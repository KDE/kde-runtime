/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "xineengine.h"

#include "mediaobject.h"
#include <QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <phonon/audiodeviceenumerator.h>
#include <phonon/audiodevice.h>
#include <QList>
#include <kconfiggroup.h>
#include <kicon.h>
#include "videowidget.h"
#include <klocale.h>
#include "xineengine_p.h"
#include "backend.h"
#include "events.h"
#include "xinethread.h"

namespace Phonon
{
namespace Xine
{
    XineEnginePrivate::XineEnginePrivate()
    {
        signalTimer.setSingleShot(true);
        connect(&signalTimer, SIGNAL(timeout()), SLOT(emitAudioDeviceChange()));
        QDBusConnection::sessionBus().registerObject("/internal/PhononXine", this, QDBusConnection::ExportScriptableSlots);
    }

    void XineEnginePrivate::emitAudioDeviceChange()
    {
        kDebug(610) ;
        emit objectDescriptionChanged(AudioOutputDeviceType);
    }

    static XineEngine *s_instance = 0;

    XineEngine::XineEngine(const KSharedConfigPtr &_config)
        : m_xine(xine_new()),
        m_config(_config),
        m_useOss(XineEngine::Unknown),
        m_inShutdown(false),
        d(new XineEnginePrivate),
        m_nullPort(0),
        m_nullVideoPort(0),
        m_thread(0)
    {
        Q_ASSERT(s_instance == 0);
        s_instance = this;
        KConfigGroup cg(m_config, "Settings");

        m_deinterlaceDVD = cg.readEntry("deinterlaceDVD", true);
        m_deinterlaceVCD = cg.readEntry("deinterlaceVCD", false);
        m_deinterlaceFile = cg.readEntry("deinterlaceFile", false);
        m_deinterlaceMethod = cg.readEntry("deinterlaceMethod", 0);
    }

    XineEngine::~XineEngine()
    {
        m_inShutdown = true;
        /*if (m_thread) {
            m_thread->stopAllStreams();
        }*/
        if (m_thread) {
            m_thread->quit();
            if (!m_thread->wait(10000)) {
                // timed out
                // assuming a deadlock, we better create a backtrace than block something important
                kFatal(610) << "Xine Thread took longer than 4s to quit. Assuming a deadlock. Please report a useful backtrace (including all threads) to bugs.kde.org";
                // if somebody made kFatal non-fatal
                m_thread->terminate();
            }
            delete m_thread;
        }

        QList<QObject *> cleanupObjects(m_cleanupObjects);
#if 0
        qDeleteAll(cleanupObjects);
#else
        const QList<QObject *>::Iterator end = cleanupObjects.end();
        QList<QObject *>::Iterator it = cleanupObjects.begin();
        while (it != end) {
            kDebug(610) << "delete" << (*it)->metaObject()->className();
            delete *it;
            ++it;
        }
#endif

        //kDebug(610) ;
        if (m_nullPort) {
            xine_close_audio_driver(m_xine, m_nullPort);
        }
        if (m_nullVideoPort) {
            xine_close_video_driver(m_xine, m_nullVideoPort);
        }
        xine_exit(m_xine);
        m_xine = 0;
        s_instance = 0;
        delete d;
    }

    XineEngine *XineEngine::self()
    {
        Q_ASSERT(s_instance);
        return s_instance;
    }

    const QObject *XineEngine::sender()
    {
        return self()->d;
    }

    xine_t *XineEngine::xine()
    {
        return self()->m_xine;
    }

    XineThread *XineEngine::thread()
    {
        XineEngine *const e = self();
        if (!e->m_thread) {
            e->m_thread = new XineThread;
            e->m_thread->moveToThread(e->m_thread);
            e->m_thread->start();
            e->m_thread->waitForEventLoop();
        }
        return e->m_thread;
    }

    bool XineEngine::deinterlaceDVD()
    {
        return s_instance->m_deinterlaceDVD;
    }

    bool XineEngine::deinterlaceVCD()
    {
        return s_instance->m_deinterlaceVCD;
    }

    bool XineEngine::deinterlaceFile()
    {
        return s_instance->m_deinterlaceFile;
    }

    int XineEngine::deinterlaceMethod()
    {
        return s_instance->m_deinterlaceMethod;
    }

    void XineEngine::xineEventListener(void *p, const xine_event_t *xineEvent)
    {
        if (!p || !xineEvent) {
            return;
        }
        //kDebug(610) << "Xine event: " << xineEvent->type << QByteArray((char *)xineEvent->data, xineEvent->data_length);

        XineStream *xs = static_cast<XineStream *>(p);

        switch (xineEvent->type) {
        case XINE_EVENT_UI_SET_TITLE: /* request title display change in ui */
            QCoreApplication::postEvent(xs, new QEVENT(NewMetaData));
            break;
        case XINE_EVENT_UI_PLAYBACK_FINISHED: /* frontend can e.g. move on to next playlist entry */
            QCoreApplication::postEvent(xs, new QEVENT(MediaFinished));
            break;
        case XINE_EVENT_PROGRESS: /* index creation/network connections */
            {
                xine_progress_data_t *progress = static_cast<xine_progress_data_t *>(xineEvent->data);
                QCoreApplication::postEvent(xs, new ProgressEvent(QString::fromUtf8(progress->description), progress->percent));
            }
            break;
        case XINE_EVENT_SPU_BUTTON: // the mouse pointer enter/leave a button, used to change the cursor
            {
                xine_spu_button_t *button = static_cast<xine_spu_button_t *>(xineEvent->data);
                if (button->direction == 1) { // enter a button
                    xs->handleDownstreamEvent(new QEVENT(NavButtonIn));
                } else {
                    xs->handleDownstreamEvent(new QEVENT(NavButtonOut));
                }
            }
            break;
        case XINE_EVENT_UI_CHANNELS_CHANGED:    /* inform ui that new channel info is available */
            kDebug(610) << "XINE_EVENT_UI_CHANNELS_CHANGED";
            {
                QCoreApplication::postEvent(xs, new QEVENT(UiChannelsChanged));
            }
            break;
        case XINE_EVENT_UI_MESSAGE:             /* message (dialog) for the ui to display */
            {
                kDebug(610) << "XINE_EVENT_UI_MESSAGE";
                const xine_ui_message_data_t *message = static_cast<xine_ui_message_data_t *>(xineEvent->data);
                if (message->type == XINE_MSG_AUDIO_OUT_UNAVAILABLE) {
                    kDebug(610) << "XINE_MSG_AUDIO_OUT_UNAVAILABLE";
                    // we don't know for sure which AudioOutput failed. but the one without any
                    // capabilities must be the guilty one
                    xs->handleDownstreamEvent(new QEVENT(AudioDeviceFailed));
                }
            }
            break;
        case XINE_EVENT_FRAME_FORMAT_CHANGE:    /* e.g. aspect ratio change during dvd playback */
            kDebug(610) << "XINE_EVENT_FRAME_FORMAT_CHANGE";
            {
                xine_format_change_data_t *data = static_cast<xine_format_change_data_t *>(xineEvent->data);
                xs->handleDownstreamEvent(new FrameFormatChangeEvent(data->width, data->height, data->aspect, data->pan_scan));
            }
            break;
        case XINE_EVENT_AUDIO_LEVEL:            /* report current audio level (l/r/mute) */
            kDebug(610) << "XINE_EVENT_AUDIO_LEVEL";
            break;
        case XINE_EVENT_QUIT:                   /* last event sent when stream is disposed */
            kDebug(610) << "XINE_EVENT_QUIT";
            break;
        case XINE_EVENT_UI_NUM_BUTTONS:         /* number of buttons for interactive menus */
            kDebug(610) << "XINE_EVENT_UI_NUM_BUTTONS";
            break;
        case XINE_EVENT_DROPPED_FRAMES:         /* number of dropped frames is too high */
            kDebug(610) << "XINE_EVENT_DROPPED_FRAMES";
            break;
        case XINE_EVENT_MRL_REFERENCE_EXT:      /* demuxer->frontend: MRL reference(s) for the real stream */
            {
                xine_mrl_reference_data_ext_t *reference = static_cast<xine_mrl_reference_data_ext_t *>(xineEvent->data);
                kDebug(610) << "XINE_EVENT_MRL_REFERENCE_EXT: " << reference->alternative
                    << ", " << reference->start_time
                    << ", " << reference->duration
                    << ", " << reference->mrl
                    << ", " << (reference->mrl + strlen(reference->mrl) + 1)
                    ;
                QCoreApplication::postEvent(xs, new ReferenceEvent(reference->alternative, reference->mrl));
            }
            break;
    }
    }

    xine_audio_port_t *XineEngine::nullPort()
    {
        if (!s_instance->m_nullPort) {
            s_instance->m_nullPort = xine_open_audio_driver(s_instance->m_xine, "none", 0);
        }
        Q_ASSERT(s_instance->m_nullPort);
        return s_instance->m_nullPort;
    }

    xine_video_port_t *XineEngine::nullVideoPort()
    {
        if (!s_instance->m_nullVideoPort) {
            s_instance->m_nullVideoPort = xine_open_video_driver(s_instance->m_xine, "auto", XINE_VISUAL_TYPE_NONE, 0);
        }
        Q_ASSERT(s_instance->m_nullVideoPort);
        return s_instance->m_nullVideoPort;
    }

    QList<int> XineEngine::audioOutputIndexes()
    {
        XineEngine *that = self();
        that->checkAudioOutputs();
        QList<int> list;
        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            list << that->m_audioOutputInfos[i].index;
        }
        return list;
    }

    QHash<QByteArray, QVariant> XineEngine::audioOutputProperties(int audioDevice)
    {
        QHash<QByteArray, QVariant> ret;
        XineEngine *that = self();
        that->checkAudioOutputs();

        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            if (that->m_audioOutputInfos[i].index == audioDevice) {
                switch (that->m_useOss) {
                case XineEngine::True: // postfix
                    if (that->m_audioOutputInfos[i].driver == "oss") {
                        ret.insert("name", i18n("%1 (OSS)", that->m_audioOutputInfos[i].name));
                    } else if (that->m_audioOutputInfos[i].driver == "alsa") {
                        ret.insert("name", i18n("%1 (ALSA)", that->m_audioOutputInfos[i].name));
                    }
                    // no postfix: fall through
                case XineEngine::False: // no postfix
                case XineEngine::Unknown: // no postfix
                    ret.insert("name", that->m_audioOutputInfos[i].name);
                }
                ret.insert("description", that->m_audioOutputInfos[i].description);

                const QString iconName = that->m_audioOutputInfos[i].icon;
                if (!iconName.isEmpty()) {
                    ret.insert("icon", KIcon(iconName));
                }
                ret.insert("available", that->m_audioOutputInfos[i].available);

                if (that->m_audioOutputInfos[i].driver == "alsa") {
                    ret.insert("mixerDeviceId", that->m_audioOutputInfos[i].mixerDevice);
                }

                ret.insert("initialPreference", that->m_audioOutputInfos[i].initialPreference);
                ret.insert("isAdvanced", that->m_audioOutputInfos[i].isAdvanced);

                return ret;
            }
        }
        ret.insert("name", QString());
        ret.insert("description", QString());
        ret.insert("available", false);
        ret.insert("initialPreference", 0);
        ret.insert("isAdvanced", false);
        return ret;
    }

    QByteArray XineEngine::audioDriverFor(int audioDevice)
    {
        XineEngine *that = self();
        that->checkAudioOutputs();
        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            if (that->m_audioOutputInfos[i].index == audioDevice) {
                return that->m_audioOutputInfos[i].driver;
            }
        }
        return QByteArray();
    }

    QStringList XineEngine::alsaDevicesFor(int audioDevice)
    {
        XineEngine *that = self();
        that->checkAudioOutputs();
        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            if (that->m_audioOutputInfos[i].index == audioDevice) {
                if (that->m_audioOutputInfos[i].driver == "alsa") { // only for ALSA
                    return that->m_audioOutputInfos[i].devices;
                }
            }
        }
        return QStringList();
    }

    void XineEnginePrivate::ossSettingChanged(bool useOss)
    {
        const XineEngine::UseOss tmp = useOss ? XineEngine::True : XineEngine::False;
        if (tmp == s_instance->m_useOss) {
            return;
        }
        s_instance->m_useOss = tmp;
        if (useOss) {
            // add OSS devices if xine supports OSS output
            const char *const *outputPlugins = xine_list_audio_output_plugins(s_instance->xine());
            for (int i = 0; outputPlugins[i]; ++i) {
                if (0 == strcmp(outputPlugins[i], "oss")) {
                    QList<AudioDevice> audioDevices = AudioDeviceEnumerator::availablePlaybackDevices();
                    foreach (const AudioDevice &dev, audioDevices) {
                        if (dev.driver() == Solid::AudioInterface::OpenSoundSystem) {
                            s_instance->addAudioOutput(dev, "oss");
                        }
                    }
                    signalTimer.start();
                    return;
                }
            }
        } else {
            // remove all OSS devices
            typedef QList<XineEngine::AudioOutputInfo>::iterator Iterator;
            Iterator it = s_instance->m_audioOutputInfos.begin();
            while (it != s_instance->m_audioOutputInfos.end()) {
                if (it->driver == "oss") {
                    it = s_instance->m_audioOutputInfos.erase(it);
                } else {
                    ++it;
                }
            }
            signalTimer.start();
        }
    }

    void XineEngine::addAudioOutput(const AudioDevice &dev, const QByteArray &driver)
    {
        QString mixerDevice;
        int initialPreference = dev.initialPreference();
        if (dev.driver() == Solid::AudioInterface::Alsa) {
            initialPreference += 100;
            foreach (QString id, dev.deviceIds()) {
                const int idx = id.indexOf(QLatin1String("CARD="));
                if (idx > 0) {
                    id = id.mid(idx + 5);
                    const int commaidx = id.indexOf(QLatin1Char(','));
                    if (commaidx > 0) {
                        id = id.left(commaidx);
                    }
                    mixerDevice = QLatin1String("hw:") + id;
                    break;
                }
                mixerDevice = id;
            }
        } else if (!dev.deviceIds().isEmpty()) {
            initialPreference += 50;
            mixerDevice = dev.deviceIds().first();
        }
        const QString description = dev.deviceIds().isEmpty() ?
            i18n("<html>This device is currently not available (either it is unplugged or the "
                    "driver is not loaded).</html>") :
            i18n("<html>This will try the following devices and use the first that works: "
                    "<ol><li>%1</li></ol></html>", dev.deviceIds().join("</li><li>"));
        AudioOutputInfo info(dev.index(), initialPreference, dev.cardName(),
                description, dev.iconName(), driver, dev.deviceIds(), mixerDevice);
        info.available = dev.isAvailable();
        info.isAdvanced = dev.isAdvancedDevice();
        if (m_audioOutputInfos.contains(info)) {
            m_audioOutputInfos.removeAll(info); // the latest is more up to date wrt availability
        }
        m_audioOutputInfos << info;
    }

    void XineEngine::addAudioOutput(int index, int initialPreference, const QString &name, const QString &description,
            const QString &icon, const QByteArray &driver, const QStringList &deviceIds, const QString &mixerDevice, bool isAdvanced)
    {
        AudioOutputInfo info(index, initialPreference, name, description, icon, driver, deviceIds, mixerDevice);
        info.isAdvanced = isAdvanced;
        const int listIndex = m_audioOutputInfos.indexOf(info);
        if (listIndex == -1) {
            info.available = true;
            m_audioOutputInfos << info;
//X             KConfigGroup config(m_config, QLatin1String("AudioOutputDevice_") + QString::number(index));
//X             config.writeEntry("name", name);
//X             config.writeEntry("description", description);
//X             config.writeEntry("driver", driver);
//X             config.writeEntry("icon", icon);
//X             config.writeEntry("initialPreference", initialPreference);
        } else {
            AudioOutputInfo &infoInList = m_audioOutputInfos[listIndex];
            if (infoInList.icon != icon || infoInList.initialPreference != initialPreference) {
//X                 KConfigGroup config(m_config, QLatin1String("AudioOutputDevice_") + QString::number(infoInList.index));

//X                 config.writeEntry("icon", icon);
//X                 config.writeEntry("initialPreference", initialPreference);

                infoInList.icon = icon;
                infoInList.initialPreference = initialPreference;
            }
            infoInList.devices = deviceIds;
            infoInList.mixerDevice = mixerDevice;
            infoInList.available = true;
        }
    }

    void XineEngine::checkAudioOutputs()
    {
        if (m_audioOutputInfos.isEmpty()) {
            kDebug(610) << "isEmpty";
            QObject::connect(AudioDeviceEnumerator::self(), SIGNAL(devicePlugged(const AudioDevice &)),
                    d, SLOT(devicePlugged(const AudioDevice &)));
            QObject::connect(AudioDeviceEnumerator::self(), SIGNAL(deviceUnplugged(const AudioDevice &)),
                    d, SLOT(deviceUnplugged(const AudioDevice &)));
            int nextIndex = 10000;
//X             QStringList groups = m_config->groupList();
//X             foreach (QString group, groups) {
//X                 if (group.startsWith("AudioOutputDevice_")) {
//X                     const int index = group.right(group.size() - 18/*strlen("AudioOutputDevice_") */).toInt();
//X                     if (index >= nextIndex) {
//X                         nextIndex = index + 1;
//X                     }
//X                     KConfigGroup config(m_config, group);
//X                     m_audioOutputInfos << AudioOutputInfo(index,
//X                             config.readEntry("initialPreference", 0),
//X                             config.readEntry("name", QString()),
//X                             config.readEntry("description", QString()),
//X                             config.readEntry("icon", QString()),
//X                             config.readEntry("driver", QByteArray()),
//X                             QStringList(), QString()); // the device list can change and needs to be queried
//X                                             // from the actual hardware configuration
//X                 }
//X             }

            // This will list the audio drivers, not the actual devices.
            const char *const *outputPlugins = xine_list_audio_output_plugins(xine());
            for (int i = 0; outputPlugins[i]; ++i) {
                kDebug(610) << "outputPlugin: " << outputPlugins[i];
                if (0 == strcmp(outputPlugins[i], "alsa")) {
                    if (m_useOss == XineEngine::Unknown) {
                        m_useOss = KConfigGroup(m_config, "Settings").readEntry("showOssDevices", false) ? XineEngine::True : XineEngine::False;
                        if (m_useOss == XineEngine::False) {
                            // remove all OSS devices
                            typedef QList<AudioOutputInfo>::iterator Iterator;
                            const Iterator end = m_audioOutputInfos.end();
                            Iterator it = m_audioOutputInfos.begin();
                            while (it != end) {
                                if (it->driver == "oss") {
                                    it = m_audioOutputInfos.erase(it);
                                } else {
                                    ++it;
                                }
                            }
                        }
                    }

                    QList<AudioDevice> alsaDevices = AudioDeviceEnumerator::availablePlaybackDevices();
                    foreach (const AudioDevice &dev, alsaDevices) {
                        if (dev.driver() == Solid::AudioInterface::Alsa) {
                            addAudioOutput(dev, "alsa");
                        }
                    }
                } else if (0 == strcmp(outputPlugins[i], "none") || 0 == strcmp(outputPlugins[i], "file")) {
                    // ignore these devices
                } else if (0 == strcmp(outputPlugins[i], "oss")) {
                    if (m_useOss) {
                        QList<AudioDevice> audioDevices = AudioDeviceEnumerator::availablePlaybackDevices();
                        foreach (const AudioDevice &dev, audioDevices) {
                            if (dev.driver() == Solid::AudioInterface::OpenSoundSystem) {
                                addAudioOutput(dev, "oss");
                            }
                        }
                    }
                } else if (0 == strcmp(outputPlugins[i], "jack")) {
                    addAudioOutput(nextIndex++, 9, i18n("Jack Audio Connection Kit"),
                            i18n("<html><p>JACK is a low-latency audio server. It can connect a number "
                                "of different applications to an audio device, as well as allowing "
                                "them to share audio between themselves.</p>"
                                "<p>JACK was designed from the ground up for professional audio "
                                "work, and its design focuses on two key areas: synchronous "
                                "execution of all clients, and low latency operation.</p></html>"),
                            /*icon name */"audio-backend-jack", outputPlugins[i], QStringList(),
                            QString());
                } else if (0 == strcmp(outputPlugins[i], "arts")) {
                    addAudioOutput(nextIndex++, -100, i18n("aRts"),
                            i18n("<html><p>aRts is the old soundserver and media framework that was used "
                                "in KDE2 and KDE3. Its use is discouraged.</p></html>"),
                            /*icon name */"audio-backend-arts", outputPlugins[i], QStringList(), QString());
                } else if (0 == strcmp(outputPlugins[i], "pulseaudio")) {
                    addAudioOutput(nextIndex++, 10, i18n("PulseAudio"),
                            xine_get_audio_driver_plugin_description(xine(), outputPlugins[i]),
                            /*icon name */"audio-backend-pulseaudio", outputPlugins[i], QStringList(), QString(), true /*isAdvanced*/);
                } else if (0 == strcmp(outputPlugins[i], "esd")) {
                    addAudioOutput(nextIndex++, 8, i18n("Esound (ESD)"),
                            xine_get_audio_driver_plugin_description(xine(), outputPlugins[i]),
                            /*icon name */"audio-backend-esd", outputPlugins[i], QStringList(), QString());
                } else {
                    addAudioOutput(nextIndex++, -20, outputPlugins[i],
                            xine_get_audio_driver_plugin_description(xine(), outputPlugins[i]),
                            /*icon name */outputPlugins[i], outputPlugins[i], QStringList(),
                            QString());
                }
            }

            qSort(m_audioOutputInfos);

            // now m_audioOutputInfos holds all devices this computer has ever seen
            foreach (const AudioOutputInfo &info, m_audioOutputInfos) {
                kDebug(610) << info.index << info.name << info.driver << info.devices;
            }
        }
    }

    void XineEnginePrivate::devicePlugged(const AudioDevice &dev)
    {
        kDebug(610) << dev.cardName();
        if (!dev.isPlaybackDevice()) {
            return;
        }
        const char *const *outputPlugins = xine_list_audio_output_plugins(XineEngine::xine());
        switch (dev.driver()) {
        case Solid::AudioInterface::Alsa:
            for (int i = 0; outputPlugins[i]; ++i) {
                if (0 == strcmp(outputPlugins[i], "alsa")) {
                    s_instance->addAudioOutput(dev, "alsa");
                    signalTimer.start();
                }
            }
            qSort(s_instance->m_audioOutputInfos);
            break;
        case Solid::AudioInterface::OpenSoundSystem:
            if (s_instance->m_useOss) {
                for (int i = 0; outputPlugins[i]; ++i) {
                    if (0 == strcmp(outputPlugins[i], "oss")) {
                        s_instance->addAudioOutput(dev, "oss");
                        signalTimer.start();
                    }
                }
            }
            qSort(s_instance->m_audioOutputInfos);
            break;
        case Solid::AudioInterface::UnknownAudioDriver:
            break;
        }
    }

    void XineEnginePrivate::deviceUnplugged(const AudioDevice &dev)
    {
        kDebug(610) << dev.cardName();
        if (!dev.isPlaybackDevice()) {
            return;
        }
        QByteArray driver;
        switch (dev.driver()) {
        case Solid::AudioInterface::Alsa:
            driver = "alsa";
            break;
        case Solid::AudioInterface::OpenSoundSystem:
            driver = "oss";
            break;
        case Solid::AudioInterface::UnknownAudioDriver:
            break;
        }
        XineEngine::AudioOutputInfo info(dev.index(), 0, dev.cardName(), QString(), dev.iconName(),
                driver, dev.deviceIds(), QString());
        const int indexOfInfo = s_instance->m_audioOutputInfos.indexOf(info);
        if (indexOfInfo < 0) {
            kDebug(610) << "told to remove " << dev.cardName() <<
                " with driver " << driver << " but the device was not present in m_audioOutputInfos";
            return;
        }
        const XineEngine::AudioOutputInfo oldInfo = s_instance->m_audioOutputInfos.takeAt(indexOfInfo);
        Q_ASSERT(!s_instance->m_audioOutputInfos.contains(info));
        info.initialPreference = oldInfo.initialPreference;
        s_instance->m_audioOutputInfos << info; // now the device is listed as not available
        qSort(s_instance->m_audioOutputInfos);
        signalTimer.start();
    }
} // namespace Xine
} // namespace Phonon

#include "xineengine_p.moc"
// vim: sw=4 ts=4 tw=100 et
