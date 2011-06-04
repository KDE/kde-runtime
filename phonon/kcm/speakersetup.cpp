/*  This file is part of the KDE project
    Copyright (C) 2010 Colin Guthrie <cguthrie@mandriva.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "speakersetup.h"
#include "testspeakerwidget.h"
#include <kconfiggroup.h>
#include <kaboutdata.h>
#include <kicon.h>

#include <glib.h>
#include <pulse/xmalloc.h>
#include <pulse/glib-mainloop.h>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

#define SS_DEFAULT_ICON "audio-card"


static pa_glib_mainloop *s_mainloop = NULL;
static pa_context *s_context = NULL;

typedef struct {
  uint32_t index;
  QString name;
  QString icon;
  QMap<uint32_t,QPair<QString,QString> > profiles;
  QString activeProfile;
} cardInfo;

QMap<uint32_t,cardInfo> s_Cards;

typedef struct {
  uint32_t index;
  uint32_t cardIndex;
  QString name;
  QString icon;
  pa_channel_map channelMap;
  QMap<uint32_t,QPair<QString,QString> > ports;
  QString activePort;
} deviceInfo;

QMap<uint32_t,deviceInfo> s_Sinks;

QMap<uint32_t,deviceInfo> s_Sources;

static int debugLevel() {
    static int level = -1;
    if (level < 1) {
        level = 0;
        QString pulseenv = qgetenv("PHONON_PULSEAUDIO_DEBUG");
        int l = pulseenv.toInt();
        if (l > 0)
            level = (l > 2 ? 2 : l);
    }
    return level;
}

static void logMessage(const QString &message, int priority = 2, QObject *obj=0);
static void logMessage(const QString &message, int priority, QObject *obj)
{
    if (debugLevel() > 0) {
        QString output;
        if (obj) {
            // Strip away namespace from className
            QString className(obj->metaObject()->className());
            int nameLength = className.length() - className.lastIndexOf(':') - 1;
            className = className.right(nameLength);
            output.sprintf("%s %s (%s %p)", message.toLatin1().constData(),
                           obj->objectName().toLatin1().constData(),
                           className.toLatin1().constData(), obj);
        }
        else {
            output = message;
        }
        if (priority <= debugLevel()) {
            qDebug() << QString("PulseSupportSS(%1): %2").arg(priority).arg(output);
        }
    }
}


static void card_cb(pa_context *c, const pa_card_info *i, int eol, void *userdata) {
    Q_ASSERT(c);
    Q_ASSERT(userdata);

    SpeakerSetup* ss = static_cast<SpeakerSetup*>(userdata);

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        logMessage(QString("Card callback failure"));
        return;
    }

    if (eol > 0) {
        ss->updateFromPulse();
        return;
    }

    Q_ASSERT(i);
    ss->updateCard(i);
}

static void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    Q_ASSERT(c);
    Q_ASSERT(userdata);

    SpeakerSetup* ss = static_cast<SpeakerSetup*>(userdata);

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        logMessage(QString("Sink callback failure"));
        return;
    }

    if (eol > 0) {
        ss->updateIndependantDevices();
        ss->updateFromPulse();
        return;
    }

    Q_ASSERT(i);
    ss->updateSink(i);
}

static void source_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    Q_ASSERT(c);
    Q_ASSERT(userdata);

    SpeakerSetup* ss = static_cast<SpeakerSetup*>(userdata);

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        logMessage(QString("Source callback failure"));
        return;
    }

    if (eol > 0) {
        ss->updateIndependantDevices();
        ss->updateFromPulse();
        return;
    }

    Q_ASSERT(i);
    ss->updateSource(i);
}

static void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
    Q_ASSERT(c);
    Q_ASSERT(userdata);

    SpeakerSetup* ss = static_cast<SpeakerSetup*>(userdata);

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_CARD:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                ss->removeCard(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_card_info_by_index(c, index, card_cb, ss))) {
                    logMessage(QString("pa_context_get_card_info_by_index() failed"));
                    return;
                }
                pa_operation_unref(o);
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SINK:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
              ss->removeSink(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_sink_info_by_index(c, index, sink_cb, ss))) {
                    logMessage(QString("pa_context_get_sink_info_by_index() failed"));
                    return;
                }
                pa_operation_unref(o);
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SOURCE:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
              ss->removeSource(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_source_info_by_index(c, index, source_cb, ss))) {
                    logMessage(QString("pa_context_get_source_info_by_index() failed"));
                    return;
                }
                pa_operation_unref(o);
            }
            break;
    }
}


static const char* statename(pa_context_state_t state)
{
    switch (state)
    {
        case PA_CONTEXT_UNCONNECTED:  return "Unconnected";
        case PA_CONTEXT_CONNECTING:   return "Connecting";
        case PA_CONTEXT_AUTHORIZING:  return "Authorizing";
        case PA_CONTEXT_SETTING_NAME: return "Setting Name";
        case PA_CONTEXT_READY:        return "Ready";
        case PA_CONTEXT_FAILED:       return "Failed";
        case PA_CONTEXT_TERMINATED:   return "Terminated";
    }

    static QString unknown;
    unknown = QString("Unknown state: %0").arg(state);
    return unknown.toAscii().constData();
}

static void context_state_callback(pa_context *c, void *userdata)
{
    Q_ASSERT(c);
    Q_ASSERT(userdata);

    SpeakerSetup* ss = static_cast<SpeakerSetup*>(userdata);

    logMessage(QString("context_state_callback %1").arg(statename(pa_context_get_state(c))));
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY) {
        // Attempt to load things up
        pa_operation *o;

        pa_context_set_subscribe_callback(c, subscribe_cb, ss);

        if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
                                          (PA_SUBSCRIPTION_MASK_CARD|
                                           PA_SUBSCRIPTION_MASK_SINK|
                                           PA_SUBSCRIPTION_MASK_SOURCE), NULL, NULL))) {
            logMessage(QString("pa_context_subscribe() failed"));
            return;
        }
        pa_operation_unref(o);

        if (!(o = pa_context_get_card_info_list(c, card_cb, ss))) {
          logMessage(QString("pa_context_get_card_info_list() failed"));
          return;
        }
        pa_operation_unref(o);

        if (!(o = pa_context_get_sink_info_list(c, sink_cb, ss))) {
          logMessage(QString("pa_context_get_sink_info_list() failed"));
          return;
        }
        pa_operation_unref(o);

        if (!(o = pa_context_get_source_info_list(c, source_cb, ss))) {
          logMessage(QString("pa_context_get_source_info_list() failed"));
          return;
        }
        pa_operation_unref(o);

        ss->load();

    } else if (!PA_CONTEXT_IS_GOOD(state)) {
        /// @todo Deal with reconnection...
        //logMessage(QString("Connection to PulseAudio lost: %1").arg(pa_strerror(pa_context_errno(c))));

        // If this is our probe phase, exit our context immediately
        if (s_context != c)
            pa_context_disconnect(c);
        else {
            pa_context_unref(s_context);
            s_context = NULL;
            //QTimer::singleShot(50, PulseSupport::getInstance(), SLOT(connectToDaemon()));
        }
    }
}

static void suspended_callback(pa_stream *s, void *userdata) {
    SpeakerSetup *ss = static_cast<SpeakerSetup*>(userdata);

    if (pa_stream_is_suspended(s))
        ss->updateVUMeter(-1);
}

static void read_callback(pa_stream *s, size_t length, void *userdata) {
    SpeakerSetup *ss = static_cast<SpeakerSetup*>(userdata);
    const void *data;
    int v;

    if (pa_stream_peek(s, &data, &length) < 0) {
        logMessage("Failed to read data from stream");
        return;
    }

    Q_ASSERT(length > 0);
    Q_ASSERT(length % sizeof(float) == 0);

    v = ((const float*) data)[length / sizeof(float) -1] * 100;

    pa_stream_drop(s);

    if (v < 0)
        v = 0;
    if (v > 100)
        v = 100;

    ss->updateVUMeter(v);
}


SpeakerSetup::SpeakerSetup(QWidget *parent)
    : QWidget(parent)
    , m_OutstandingRequests(3)
    , m_Canberra(NULL)
    , m_VUStream(NULL)
    , m_VURealValue(0)
{
    setupUi(this);

    cardLabel->setEnabled(false);
    cardBox->setEnabled(false);
    profileLabel->setVisible(false);
    profileBox->setVisible(false);

    deviceLabel->setEnabled(false);
    deviceBox->setEnabled(false);
    portLabel->setVisible(false);
    portBox->setVisible(false);

    for (int i=0; i<5; ++i)
        placementGrid->setColumnStretch(i, 1);
    for (int i=0; i<3; ++i)
        placementGrid->setRowStretch(i, 1);

    m_Icon = new QLabel(this);
    const QFileInfo fi(QDir(QDir::homePath()), ".face.icon");
    if (fi.exists())
        m_Icon->setPixmap(QPixmap(fi.absoluteFilePath()).scaled(KIconLoader::SizeHuge, KIconLoader::SizeHuge, Qt::KeepAspectRatio));
    else
        m_Icon->setPixmap(KIcon("system-users").pixmap(KIconLoader::SizeHuge, KIconLoader::SizeHuge));
    placementGrid->addWidget(m_Icon, 1, 2, Qt::AlignCenter);

    update();
    connect(cardBox,    SIGNAL(currentIndexChanged(int)), SLOT(cardChanged()));
    connect(profileBox, SIGNAL(currentIndexChanged(int)), SLOT(profileChanged()));
    connect(deviceBox,  SIGNAL(currentIndexChanged(int)), SLOT(deviceChanged()));
    connect(portBox,    SIGNAL(currentIndexChanged(int)), SLOT(portChanged()));

    m_VUTimer = new QTimer(this);
    m_VUTimer->setInterval(10);
    connect(m_VUTimer, SIGNAL(timeout()), this, SLOT(reallyUpdateVUMeter()));

    // We require a glib event loop
    if (QLatin1String(QAbstractEventDispatcher::instance()->metaObject()->className())
            != "QGuiEventDispatcherGlib") {
        logMessage("Disabling PulseAudio integration for lack of GLib event loop.");
        return;
    }

    s_mainloop = pa_glib_mainloop_new(NULL);
    Q_ASSERT(s_mainloop);

    pa_mainloop_api *api = pa_glib_mainloop_get_api(s_mainloop);

    s_context = pa_context_new(api, "kspeakersetup");
    int rv;
    rv = pa_context_connect(s_context, NULL, PA_CONTEXT_NOFAIL, 0);
    Q_ASSERT(rv >= 0);

    pa_context_set_state_callback(s_context, &context_state_callback, this);

    rv = ca_context_create(&m_Canberra);
    Q_ASSERT(rv >= 0);
}

SpeakerSetup::~SpeakerSetup()
{
    if (m_Canberra)
        ca_context_destroy(m_Canberra);
    if (s_context) {
        pa_context_unref(s_context);
        s_context = NULL;
    }
    if (s_mainloop) {
        pa_glib_mainloop_free(s_mainloop);
        s_mainloop = NULL;
    }
}

void SpeakerSetup::load()
{
}

void SpeakerSetup::save()
{
}

void SpeakerSetup::defaults()
{
}

void SpeakerSetup::updateCard(const pa_card_info* i)
{
    cardInfo info;
    info.index = i->index;

    const char* description = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_DESCRIPTION);
    info.name = description ? QString::fromUtf8(description) : i->name;

    const char* icon = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME);
    info.icon = icon ? icon : SS_DEFAULT_ICON;

    for (uint32_t j = 0; j < i->n_profiles; ++j)
      info.profiles[i->profiles[j].priority] = QPair<QString,QString>(i->profiles[j].name, QString::fromUtf8(i->profiles[j].description));
    if (i->active_profile)
      info.activeProfile = i->active_profile->name;


    bool bs = cardBox->blockSignals(true);
    if (s_Cards.contains(i->index)) {
      int idx = cardBox->findData(i->index);
      if (idx >= 0) {
        cardBox->setItemIcon(idx, KIcon(info.icon));
        cardBox->setItemText(idx, info.name);
      }
    }
    else
      cardBox->addItem(KIcon(info.icon), info.name, i->index);
    cardBox->blockSignals(bs);

    s_Cards[i->index] = info;

    cardChanged();

    logMessage(QString("Got info about card %1").arg(info.name));
}

void SpeakerSetup::removeCard(uint32_t index)
{
    s_Cards.remove(index);
    updateFromPulse();
    int idx = cardBox->findData(index);
    if (idx >= 0)
        cardBox->removeItem(idx);
}

void SpeakerSetup::updateSink(const pa_sink_info* i)
{
    deviceInfo info;
    info.index = i->index;
    info.cardIndex = i->card;
    info.name = QString::fromUtf8(i->description);

    const char* icon = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME);
    info.icon = icon ? icon : SS_DEFAULT_ICON;

    info.channelMap = i->channel_map;

    for (uint32_t j = 0; j < i->n_ports; ++j)
      info.ports[i->ports[j]->priority] = QPair<QString,QString>(i->ports[j]->name, QString::fromUtf8(i->ports[j]->description));
    if (i->active_port)
      info.activePort = i->active_port->name;

    s_Sinks[i->index] = info;

    // Need to update the currently displayed port if this sink is the currently displayed one.
    if (info.ports.size()) {
        int idx = deviceBox->currentIndex();
        if (idx >= 0) {
            int64_t index = deviceBox->itemData(idx).toInt();
            if (index > 0 && index == i->index) {
                bool bs = portBox->blockSignals(true);
                portBox->setCurrentIndex(portBox->findData(info.activePort));
                portBox->blockSignals(bs);
            }
        }
    }

    logMessage(QString("Got info about sink %1").arg(info.name));
}

void SpeakerSetup::removeSink(uint32_t index)
{
    s_Sinks.remove(index);
    updateIndependantDevices();
    updateFromPulse();
    int idx = deviceBox->findData(index);
    if (idx >= 0)
        deviceBox->removeItem(idx);
}

void SpeakerSetup::updateSource(const pa_source_info* i)
{
    if (i->monitor_of_sink != PA_INVALID_INDEX)
        return;

    deviceInfo info;
    info.index = i->index;
    info.cardIndex = i->card;
    info.name = QString::fromUtf8(i->description);

    const char* icon = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME);
    info.icon = icon ? icon : SS_DEFAULT_ICON;

    info.channelMap = i->channel_map;

    for (uint32_t j = 0; j < i->n_ports; ++j)
      info.ports[i->ports[j]->priority] = QPair<QString,QString>(i->ports[j]->name, QString::fromUtf8(i->ports[j]->description));
    if (i->active_port)
      info.activePort = i->active_port->name;

    s_Sources[i->index] = info;

    // Need to update the currently displayed port if this source is the currently displayed one.
    if (false && info.ports.size()) {
        int idx = deviceBox->currentIndex();
        if (idx >= 0) {
            int64_t index = deviceBox->itemData(idx).toInt();
            if (index < 0 && -1*index == i->index) {
                bool bs = portBox->blockSignals(true);
                portBox->setCurrentIndex(portBox->findData(info.activePort));
                portBox->blockSignals(bs);
            }
        }
    }

    logMessage(QString("Got info about source %1").arg(info.name));
}

void SpeakerSetup::removeSource(uint32_t index)
{
    s_Sources.remove(index);
    updateIndependantDevices();
    updateFromPulse();
    int idx = deviceBox->findData(index);
    if (false && idx >= 0)
        deviceBox->removeItem(idx);
}

void SpeakerSetup::updateFromPulse()
{
    if (m_OutstandingRequests > 0) {
        if (0 == --m_OutstandingRequests) {
            // Work out which seclector to pick by default (we want to choose a real Card if possible)
            if (s_Cards.size() != cardBox->count())
                cardBox->setCurrentIndex(1);
            emit ready();
        }
    }

    if (!m_OutstandingRequests) {
        if (!s_Cards.size() && !s_Sinks.size()) {
            cardLabel->setEnabled(false);
            cardBox->setEnabled(false);
            profileLabel->setVisible(false);
            profileBox->setVisible(false);

            deviceLabel->setEnabled(false);
            deviceBox->setEnabled(false);
            portLabel->setVisible(false);
            portBox->setVisible(false);
        }
        if (s_Cards.size() && !cardBox->isEnabled()) {
            cardLabel->setEnabled(true);
            cardBox->setEnabled(true);
            cardChanged();
        }
        if (s_Sinks.size() && !deviceBox->isEnabled()) {
            deviceLabel->setEnabled(true);
            deviceBox->setEnabled(true);
            deviceChanged();
        }
    }
}

void SpeakerSetup::cardChanged()
{
    int idx = cardBox->currentIndex();
    if (idx < 0) {
        profileLabel->setVisible(false);
        profileBox->setVisible(false);
        return;
    }

    uint32_t card_index = cardBox->itemData(idx).toUInt();
    Q_ASSERT(PA_INVALID_INDEX == card_index || s_Cards.contains(card_index));
    bool show_profiles = (PA_INVALID_INDEX != card_index && s_Cards[card_index].profiles.size());
    if (show_profiles) {
        cardInfo &card_info = s_Cards[card_index];
        bool bs = profileBox->blockSignals(true);
        profileBox->clear();
        for (QMap<uint32_t, QPair<QString,QString> >::iterator it = card_info.profiles.begin(); it != card_info.profiles.end(); ++it)
            profileBox->insertItem(0, it.value().second, it.value().first);
        profileBox->setCurrentIndex(profileBox->findData(card_info.activeProfile));
        profileBox->blockSignals(bs);
    }
    profileLabel->setVisible(show_profiles);
    profileBox->setVisible(show_profiles);


    bool bs = deviceBox->blockSignals(true);
    deviceBox->clear();
    for (QMap<uint32_t,deviceInfo>::iterator it = s_Sinks.begin(); it != s_Sinks.end(); ++it) {
        if (it->cardIndex == card_index)
            deviceBox->addItem(KIcon(it->icon), i18n("Playback (%1)", it->name), it->index);
    }
    for (QMap<uint32_t,deviceInfo>::iterator it = s_Sources.begin(); it != s_Sources.end(); ++it) {
        if (it->cardIndex == card_index)
            deviceBox->addItem(KIcon(it->icon), i18n("Capture (%1)", it->name), -1*it->index);
    }
    deviceBox->blockSignals(bs);

    deviceGroupBox->setEnabled(!!deviceBox->count());

    deviceChanged();

    logMessage(QString("Doing update %1").arg(cardBox->currentIndex()));

    emit changed();
}

void SpeakerSetup::profileChanged()
{
    uint32_t card_index = cardBox->itemData(cardBox->currentIndex()).toUInt();
    Q_ASSERT(PA_INVALID_INDEX != card_index);

    QString profile = profileBox->itemData(profileBox->currentIndex()).toString();
    logMessage(QString("Changing profile to %1").arg(profile));

    cardInfo &card_info = s_Cards[card_index];
    Q_ASSERT(card_info.profiles.size());

    pa_operation *o;
    if (!(o = pa_context_set_card_profile_by_index(s_context, card_index, profile.toAscii().constData(), NULL, NULL)))
        logMessage(QString("pa_context_set_card_profile_by_name() failed"));
    else
        pa_operation_unref(o);

    emit changed();
}

void SpeakerSetup::updateIndependantDevices()
{
    // Should we display the "Independent Devices" drop down?
    // Count all the sinks without cards
    bool showID = false;
    for (QMap<uint32_t,deviceInfo>::iterator it = s_Sinks.begin(); it != s_Sinks.end(); ++it) {
        if (PA_INVALID_INDEX == it->cardIndex) {
            showID = true;
            break;
        }
    }

    bool haveID = (PA_INVALID_INDEX == cardBox->itemData(0).toUInt());

    logMessage(QString("Want ID: %1; Have ID: %2").arg(showID?"Yes":"No").arg(haveID?"Yes":"No"));

    bool bs = cardBox->blockSignals(true);
    if (haveID && !showID)
        cardBox->removeItem(0);
    else if (!haveID && showID)
        cardBox->insertItem(0, KIcon(SS_DEFAULT_ICON), "Independent Devices", PA_INVALID_INDEX);
    cardBox->blockSignals(bs);
}

void SpeakerSetup::updateVUMeter(int vol)
{
  if (vol < 0) {
      inputLevels->setEnabled(false);
      inputLevels->setValue(0);
      m_VURealValue = 0;
  } else {
      inputLevels->setEnabled(true);
      if (vol > inputLevels->value())
          inputLevels->setValue(vol);
      m_VURealValue = vol;
  }
}

void SpeakerSetup::reallyUpdateVUMeter()
{
    int val = inputLevels->value();
    if (val > m_VURealValue)
        inputLevels->setValue(val-1);
}

static deviceInfo & getDeviceInfo(int64_t index)
{
    if (index > 0) {
      Q_ASSERT(s_Sinks.contains(index));
      return s_Sinks[index];
    }
    
    index *= -1;
    Q_ASSERT(s_Sources.contains(index));
    return s_Sources[index];
}

void SpeakerSetup::deviceChanged()
{
    int idx = deviceBox->currentIndex();
    if (idx < 0) {
        portLabel->setVisible(false);
        portBox->setVisible(false);
        _updatePlacementTester();
        return;
    }
    int64_t index = deviceBox->itemData(idx).toInt();
    deviceInfo &device_info = getDeviceInfo(index);

    logMessage(QString("Updating ports for device '%1' (%2 ports available)").arg(device_info.name).arg(device_info.ports.size()));

    bool show_ports = !!device_info.ports.size();
    if (show_ports) {
        bool bs = portBox->blockSignals(true);
        portBox->clear();
        for (QMap<uint32_t, QPair<QString,QString> >::iterator it = device_info.ports.begin(); it != device_info.ports.end(); ++it)
            portBox->insertItem(0, it.value().second, it.value().first);
        portBox->setCurrentIndex(portBox->findData(device_info.activePort));
        portBox->blockSignals(bs);
    }
    portLabel->setVisible(show_ports);
    portBox->setVisible(show_ports);

    if (deviceBox->currentIndex() >= 0) {
        if (index < 0)
            _createMonitorStreamForSource(-1*index);
        _updatePlacementTester();
    }

    emit changed();
}

void SpeakerSetup::portChanged()
{
    int64_t index = deviceBox->itemData(deviceBox->currentIndex()).toInt();

    QString port = portBox->itemData(portBox->currentIndex()).toString();
    logMessage(QString("Changing port to %1").arg(port));

    deviceInfo &device_info = getDeviceInfo(index);
    Q_ASSERT(device_info.ports.size());

    pa_operation *o;
    if (index > 0) {
        if (!(o = pa_context_set_sink_port_by_index(s_context, (uint32_t)index, port.toAscii().constData(), NULL, NULL)))
            logMessage(QString("pa_context_set_sink_port_by_index() failed"));
        else
            pa_operation_unref(o);
    } else {
        if (!(o = pa_context_set_source_port_by_index(s_context, (uint32_t)(-1*index), port.toAscii().constData(), NULL, NULL)))
            logMessage(QString("pa_context_set_source_port_by_index() failed"));
        else
            pa_operation_unref(o);
    }

    emit changed();
}

void SpeakerSetup::_updatePlacementTester()
{
    static const int position_table[] = {
        /* Position, X, Y */
        PA_CHANNEL_POSITION_FRONT_LEFT,            0, 0,
        PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,  1, 0,
        PA_CHANNEL_POSITION_FRONT_CENTER,          2, 0,
        PA_CHANNEL_POSITION_MONO,                  2, 0,
        PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER, 3, 0,
        PA_CHANNEL_POSITION_FRONT_RIGHT,           4, 0,
        PA_CHANNEL_POSITION_SIDE_LEFT,             0, 1,
        PA_CHANNEL_POSITION_SIDE_RIGHT,            4, 1,
        PA_CHANNEL_POSITION_REAR_LEFT,             0, 2,
        PA_CHANNEL_POSITION_REAR_CENTER,           2, 2,
        PA_CHANNEL_POSITION_REAR_RIGHT,            4, 2,
        PA_CHANNEL_POSITION_LFE,                   3, 2
    };

    QLayoutItem* w;
    while ((w = placementGrid->takeAt(0))) {
        if (w->widget() != m_Icon) {
            if (w->widget())
                delete w->widget();
            delete w;
        }
    }
    placementGrid->addWidget(m_Icon, 1, 2, Qt::AlignCenter);
    int idx = deviceBox->currentIndex();
    if (idx < 0)
      return;

    int64_t index = deviceBox->itemData(idx).toInt();
    deviceInfo& sink_info = getDeviceInfo(index);

    if (index < 0) {
      playbackOrCaptureStack->setCurrentIndex(1);
      m_VUTimer->start();
      return;
    }

    playbackOrCaptureStack->setCurrentIndex(0);
    m_VUTimer->stop();

    for (int i = 0; i < 36; i += 3) {
        pa_channel_position_t pos = (pa_channel_position_t)position_table[i];
        // Check to see if we have this item in our current channel map.
        bool have = false;
        for (uint32_t j = 0; j < sink_info.channelMap.channels; ++j) {
            if (sink_info.channelMap.map[j] == pos) {
                have = true;
                break;
            }
        }
        if (!have) {
            continue;
        }

        KPushButton* btn = new TestSpeakerWidget(pos, m_Canberra, this);//KPushButton(KIcon("audio-card"), (name ? name : "Unknown Channel"), this);
        placementGrid->addWidget(btn, position_table[i+2], position_table[i+1], Qt::AlignCenter);
    }

}

void SpeakerSetup::_createMonitorStreamForSource(uint32_t source_idx)
{
    if (m_VUStream) {
        pa_stream_disconnect(m_VUStream);
        m_VUStream = NULL;
    }
    
    char t[16];
    pa_buffer_attr attr;
    pa_sample_spec ss;

    ss.channels = 1;
    ss.format = PA_SAMPLE_FLOAT32;
    ss.rate = 25;

    memset(&attr, 0, sizeof(attr));
    attr.fragsize = sizeof(float);
    attr.maxlength = (uint32_t) -1;

    snprintf(t, sizeof(t), "%u", source_idx);

    if (!(m_VUStream = pa_stream_new(s_context, "Peak detect", &ss, NULL))) {
        logMessage("Failed to create monitoring stream");
        return;
    }

    pa_stream_set_read_callback(m_VUStream, read_callback, this);
    pa_stream_set_suspended_callback(m_VUStream, suspended_callback, this);

    if (pa_stream_connect_record(m_VUStream, t, &attr, (pa_stream_flags_t) (PA_STREAM_DONT_MOVE|PA_STREAM_PEAK_DETECT|PA_STREAM_ADJUST_LATENCY)) < 0) {
        logMessage("Failed to connect monitoring stream");
        pa_stream_unref(m_VUStream);
        m_VUStream = NULL;
    }
}

uint32_t SpeakerSetup::getCurrentSinkIndex()
{
    int idx = deviceBox->currentIndex();
    if (idx < 0)
        return PA_INVALID_INDEX;

    int64_t index = deviceBox->itemData(idx).toInt();
    if (index > 0)
        return (uint32_t)index;

    return PA_INVALID_INDEX;
}


#include "speakersetup.moc"
// vim: sw=4 sts=4 et tw=100
