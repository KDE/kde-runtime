/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "phononserver.h"
#include "audiodevice.h"

#include <kconfiggroup.h>
#include <klocale.h>
#include <kdebug.h>
#include <KPluginFactory>
#include <KPluginLoader>
#include <QtCore/QRegExp>
#include <QtCore/QTimerEvent>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>
#include <QtCore/QVariant>
#include <Solid/AudioInterface>
#include <Solid/GenericInterface>
#include <Solid/Device>
#include <Solid/DeviceNotifier>

#ifdef HAVE_PULSEAUDIO
#include <pulse/pulseaudio.h>
#endif // HAVE_PULSEAUDIO

#include <../config-alsa.h>
#ifdef HAVE_LIBASOUND2
#include <alsa/asoundlib.h>
#endif // HAVE_LIBASOUND2

K_PLUGIN_FACTORY(PhononServerFactory,
        registerPlugin<PhononServer>();
        )
K_EXPORT_PLUGIN(PhononServerFactory("phononserver"))

typedef QList<QPair<QByteArray, QString> > PhononDeviceAccessList;
Q_DECLARE_METATYPE(PhononDeviceAccessList)

PhononServer::PhononServer(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent),
    m_config(KSharedConfig::openConfig("phonondevicesrc", KConfig::NoGlobals))
{
    findDevices();
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString &)), SLOT(deviceAdded(const QString &)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString &)), SLOT(deviceRemoved(const QString &)));
    qRegisterMetaType<PhononDeviceAccessList>();
    qRegisterMetaTypeStreamOperators<PhononDeviceAccessList>("PhononDeviceAccessList");
}

PhononServer::~PhononServer()
{
}

static QString uniqueId(const Solid::Device &device, int deviceNumber)
{
    const Solid::GenericInterface *genericIface = device.as<Solid::GenericInterface>();
    Q_ASSERT(genericIface);
    const QString &subsystem = genericIface->propertyExists(QLatin1String("info.subsystem")) ?
        genericIface->property(QLatin1String("info.subsystem")).toString() :
        genericIface->property(QLatin1String("linux.subsystem")).toString();
    if (subsystem == "pci") {
        const QVariant vendor_id = genericIface->property("pci.vendor_id");
        if (vendor_id.isValid()) {
            const QVariant product_id = genericIface->property("pci.product_id");
            if (product_id.isValid()) {
                const QVariant subsys_vendor_id = genericIface->property("pci.subsys_vendor_id");
                if (subsys_vendor_id.isValid()) {
                    const QVariant subsys_product_id = genericIface->property("pci.subsys_product_id");
                    if (subsys_product_id.isValid()) {
                        return QString("pci:%1:%2:%3:%4:%5")
                            .arg(vendor_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(product_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(subsys_vendor_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(subsys_product_id.toInt(), 4, 16, QLatin1Char('0'))
                            .arg(deviceNumber);
                    }
                }
            }
        }
    } else if (subsystem == "usb" || subsystem == "usb_device") {
        const QVariant vendor_id = genericIface->property("usb.vendor_id");
        if (vendor_id.isValid()) {
            const QVariant product_id = genericIface->property("usb.product_id");
            if (product_id.isValid()) {
                return QString("usb:%1:%2:%3")
                    .arg(vendor_id.toInt(), 4, 16, QLatin1Char('0'))
                    .arg(product_id.toInt(), 4, 16, QLatin1Char('0'))
                    .arg(deviceNumber);
            }
        }
    } else {
        // not the right device, need to look at the parent (but not at the top-level device in the
        // device tree - that would be too far up the hierarchy)
        if (device.parent().isValid() && device.parent().parent().isValid()) {
            return uniqueId(device.parent(), deviceNumber);
        }
    }
    return QString();
}

static void renameDevices(QList<PS::AudioDevice> *devicelist)
{
    QHash<QString, int> cardNames;
    foreach (const PS::AudioDevice &dev, *devicelist) {
        ++cardNames[dev.name()];
    }

    // Now look for duplicate names and rename those by appending the device number
    QMutableListIterator<PS::AudioDevice> it(*devicelist);
    while (it.hasNext()) {
        PS::AudioDevice &dev = it.next();
        if (dev.deviceNumber() > 0 && cardNames.value(dev.name()) > 1) {
            dev.setPreferredName(dev.name() + QLatin1String(" #") + QString::number(dev.deviceNumber()));
        }
    }
}

struct DeviceHint
{
    QString name;
    QString description;
};

void PhononServer::findVirtualDevices()
{
#ifdef HAVE_LIBASOUND2
    QList<DeviceHint> deviceHints;

    void **hints;
    //snd_config_update();
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
        kDebug(603) << "snd_device_name_hint failed for 'pcm'";
        return;
    }

    for (void **cStrings = hints; *cStrings; ++cStrings) {
        DeviceHint nextHint;
        char *x = snd_device_name_get_hint(*cStrings, "NAME");
        nextHint.name = QString::fromUtf8(x);
        free(x);

        if (nextHint.name.startsWith("front:") ||
                /*nextHint.name.startsWith("rear:") ||
                nextHint.name.startsWith("center_lfe:") ||*/
                nextHint.name.startsWith("surround40:") ||
                nextHint.name.startsWith("surround41:") ||
                nextHint.name.startsWith("surround50:") ||
                nextHint.name.startsWith("surround51:") ||
                nextHint.name.startsWith("surround71:") ||
                nextHint.name.startsWith("default:") ||
                nextHint.name == "null"
                ) {
            continue;
        }

        x = snd_device_name_get_hint(*cStrings, "DESC");
        nextHint.description = QString::fromUtf8(x);
        free(x);

        deviceHints << nextHint;
    }
    snd_device_name_free_hint(hints);

    snd_config_update_free_global();
    snd_config_update();
    Q_ASSERT(snd_config);
    foreach (const DeviceHint &deviceHint, deviceHints) {
        const QString &alsaDeviceName = deviceHint.name;
        const QString &description = deviceHint.description;
        const QString &uniqueId = description;
        //const QString &udi = alsaDeviceName;
        const QStringList &lines = description.split("\n");
        bool isAdvanced = false;
        QString cardName = lines.first();
        if (lines.size() > 1) {
            cardName = i18n("%1 (%2)", cardName, lines[1]);
        }
        if (alsaDeviceName.startsWith("front:") ||
                alsaDeviceName.startsWith("rear:") ||
                alsaDeviceName.startsWith("center_lfe:") ||
                alsaDeviceName.startsWith("surround40:") ||
                alsaDeviceName.startsWith("surround41:") ||
                alsaDeviceName.startsWith("surround50:") ||
                alsaDeviceName.startsWith("surround51:") ||
                alsaDeviceName.startsWith("surround71:") ||
                alsaDeviceName.startsWith("iec958:")) {
            isAdvanced = true;
        }

        bool available = false;
        bool playbackDevice = false;
        bool captureDevice = false;
        {
            snd_pcm_t *pcm;
            const QByteArray &deviceNameEnc = alsaDeviceName.toUtf8();
            if (0 == snd_pcm_open(&pcm, deviceNameEnc.constData(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK /*open mode: non-blocking, sync */)) {
                available = true;
                playbackDevice = true;
                snd_pcm_close(pcm);
            }
            if (0 == snd_pcm_open(&pcm, deviceNameEnc.constData(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK /*open mode: non-blocking, sync */)) {
                available = true;
                captureDevice = true;
                snd_pcm_close(pcm);
            }
        }

        QString iconName(QLatin1String("audio-card"));
        int initialPreference = 30;
        if (description.contains("headset", Qt::CaseInsensitive) ||
                description.contains("headphone", Qt::CaseInsensitive)) {
            // it's a headset
            if (description.contains("usb", Qt::CaseInsensitive)) {
                iconName = QLatin1String("audio-headset-usb");
                initialPreference -= 10;
            } else {
                iconName = QLatin1String("audio-headset");
                initialPreference -= 10;
            }
        } else {
            if (description.contains("usb", Qt::CaseInsensitive)) {
                // it's an external USB device
                iconName = QLatin1String("audio-card-usb");
                initialPreference -= 10;
            }
        }

        const PS::AudioDeviceAccess access(QStringList(alsaDeviceName), 0, PS::AudioDeviceAccess::AlsaDriver,
                captureDevice, playbackDevice);
        //dev.setUseCache(false);
        if (playbackDevice) {
            const PS::AudioDeviceKey key = { uniqueId + QLatin1String("_playback"), -1, -1 };
            PS::AudioDevice dev(cardName, iconName, key, initialPreference, isAdvanced);
            dev.addAccess(access);
            m_audioOutputDevices << dev;
        }
        if (captureDevice) {
            const PS::AudioDeviceKey key = { uniqueId + QLatin1String("_capture"), -1, -1 };
            PS::AudioDevice dev(cardName, iconName, key, initialPreference, isAdvanced);
            dev.addAccess(access);
            m_audioCaptureDevices << dev;
        } else {
            if (!playbackDevice) {
                kDebug(601) << deviceHint.name << " doesn't work.";
            }
        }
    }
#endif // HAVE_LIBASOUND2
}

static void removeOssOnlyDevices(QList<PS::AudioDevice> *list)
{
    QMutableListIterator<PS::AudioDevice> it(*list);
    while (it.hasNext()) {
        const PS::AudioDevice &dev = it.next();
        if (dev.isAvailable()) {
            bool onlyOss = true;
            foreach (const PS::AudioDeviceAccess &a, dev.accessList()) {
                if (a.driver() != PS::AudioDeviceAccess::OssDriver) {
                    onlyOss = false;
                    break;
                }
            }
            if (onlyOss) {
                it.remove();
            }
        }
    }
}

#ifdef HAVE_PULSEAUDIO
class PulseDetectionUserData
{
    public:
        inline PulseDetectionUserData(PhononServer *p, pa_mainloop_api *api)
            : phononServer(p), mainloopApi(api), ready(2),
            alsaHandleMatches(QLatin1String(".*\\s(hw|front|surround\\d\\d):(\\d+)\\s.*")),
            captureNameMatches(QLatin1String(".*_sound_card_(\\d+)_.*_(?:playback|capture)_(\\d+)(\\.monitor)?")),
            playbackNameMatches(QLatin1String(".*_sound_card_(\\d+)_.*_playback_(\\d+)"))
        {}

        PhononServer *const phononServer;
        QList<QPair<PS::AudioDeviceKey, PS::AudioDeviceAccess> > sinks;
        QList<QPair<PS::AudioDeviceKey, PS::AudioDeviceAccess> > sources;

        inline void eol() { if (--ready == 0) { quit(); } }
        inline void quit() { mainloopApi->quit(mainloopApi, 0); }
    private:
        pa_mainloop_api *const mainloopApi;
        int ready;
    public:
        QRegExp alsaHandleMatches;
        QRegExp captureNameMatches;
        QRegExp playbackNameMatches;
};

static void pulseSinkInfoListCallback(pa_context *context, const pa_sink_info *i, int eol, void *userdata)
{
    PulseDetectionUserData *d = reinterpret_cast<PulseDetectionUserData *>(userdata);
    if (eol) {
        d->eol();
        return;
    }
    Q_ASSERT(i);
    kDebug(601).nospace()
        << "name: " << i->name
        << ", index: " << i->index
        << ", description: " << i->description
        << ", sample_spec: " << i->sample_spec.format << i->sample_spec.rate << i->sample_spec.channels
        << ", channel_map: " << i->channel_map.channels << i->channel_map.map
        << ", owner_module: " << i->owner_module
        //<< ", volume: " << i->volume
        << ", mute: " << i->mute
        << ", monitor_source: " << i->monitor_source
        << ", latency: " << i->latency
        << ", driver: " << i->driver
        << ", flags: " << i->flags;
    const QString &handle = QString::fromUtf8(i->name);
    if (d->playbackNameMatches.exactMatch(handle)) {
        const QString &description = QString::fromUtf8(i->description);
        const bool m = d->alsaHandleMatches.exactMatch(description);
        const int cardNumber = m ? d->alsaHandleMatches.cap(2).toInt() : d->playbackNameMatches.cap(1).toInt();
        const int deviceNumber = d->playbackNameMatches.cap(2).toInt();
        const PS::AudioDeviceKey key = { QString(), cardNumber, deviceNumber };
        const PS::AudioDeviceAccess access(QStringList(QString::fromUtf8(pa_context_get_server(context)) + QLatin1Char(':') + handle), 30, PS::AudioDeviceAccess::PulseAudioDriver, false, true);
        d->sinks << QPair<PS::AudioDeviceKey, PS::AudioDeviceAccess>(key, access);
    }
}

static void pulseSourceInfoListCallback(pa_context *context, const pa_source_info *i, int eol, void *userdata)
{
    PulseDetectionUserData *d = reinterpret_cast<PulseDetectionUserData *>(userdata);
    if (eol) {
        d->eol();
        return;
    }
    Q_ASSERT(i);
    kDebug(601).nospace()
        << "name: " << i->name
        << ", index: " << i->index
        << ", description: " << i->description
        << ", sample_spec: " << i->sample_spec.format << i->sample_spec.rate << i->sample_spec.channels
        << ", channel_map: " << i->channel_map.channels << i->channel_map.map
        << ", owner_module: " << i->owner_module
        //<< ", volume: " << i->volume
        << ", mute: " << i->mute
        << ", monitor_of_sink: " << i->monitor_of_sink
        << ", monitor_of_sink_name: " << i->monitor_of_sink_name
        << ", latency: " << i->latency
        << ", driver: " << i->driver
        << ", flags: " << i->flags;
    const QString &handle = QString::fromUtf8(i->name);
    if (d->captureNameMatches.exactMatch(handle)) {
        if (d->captureNameMatches.cap(3).isEmpty()) {
            const QString &description = QString::fromUtf8(i->description);
            const bool m = d->alsaHandleMatches.exactMatch(description);
            const int cardNumber = m ? d->alsaHandleMatches.cap(2).toInt() : d->captureNameMatches.cap(1).toInt();
            const int deviceNumber = d->captureNameMatches.cap(2).toInt();
            const PS::AudioDeviceKey key = {
                d->captureNameMatches.cap(3).isEmpty() ? QString() : handle, cardNumber, deviceNumber
            };
            const PS::AudioDeviceAccess access(QStringList(QString::fromUtf8(pa_context_get_server(context)) + QLatin1Char(':') + handle), 30, PS::AudioDeviceAccess::PulseAudioDriver, true, false);
            d->sources << QPair<PS::AudioDeviceKey, PS::AudioDeviceAccess>(key, access);
        } else {
            const PS::AudioDeviceKey key = {
                QString::fromUtf8(i->description), -2, -2
            };
            const PS::AudioDeviceAccess access(QStringList(QString::fromUtf8(pa_context_get_server(context)) + QLatin1Char(':') + handle), 30, PS::AudioDeviceAccess::PulseAudioDriver, true, false);
            d->sources << QPair<PS::AudioDeviceKey, PS::AudioDeviceAccess>(key, access);
        }
    }
}

static void pulseContextStateCallback(pa_context *context, void *userdata)
{
    switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
        /*pa_operation *op1 =*/ pa_context_get_sink_info_list(context, &pulseSinkInfoListCallback, userdata);
        /*pa_operation *op2 =*/ pa_context_get_source_info_list(context, &pulseSourceInfoListCallback, userdata);
        break;
    case PA_CONTEXT_FAILED:
        {
            PulseDetectionUserData *d = reinterpret_cast<PulseDetectionUserData *>(userdata);
            d->quit();
        }
        break;
    default:
        break;
    }
}
#endif // HAVE_PULSEAUDIO

void PhononServer::findDevices()
{
    QHash<PS::AudioDeviceKey, PS::AudioDevice> playbackDevices;
    QHash<PS::AudioDeviceKey, PS::AudioDevice> captureDevices;
    bool haveAlsaDevices = false;
    QHash<QString, QList<int> > listOfCardNumsPerUniqueId;

    KConfigGroup globalConfigGroup(m_config, "Globals");
    //const int cacheVersion = globalConfigGroup.readEntry("CacheVersion", 0);
    // cacheVersion 1 is KDE 4.1, 0 is KDE 4.0

    const QList<Solid::Device> &allHwDevices =
        Solid::Device::listFromQuery("AudioInterface.deviceType & 'AudioInput|AudioOutput'");
    foreach (const Solid::Device &hwDevice, allHwDevices) {
        const Solid::AudioInterface *audioIface = hwDevice.as<Solid::AudioInterface>();

        QStringList deviceIds;
        int accessPreference = 0;
        PS::AudioDeviceAccess::AudioDriver driver = PS::AudioDeviceAccess::InvalidDriver;
        bool capture = audioIface->deviceType() & Solid::AudioInterface::AudioInput;
        bool playback = audioIface->deviceType() & Solid::AudioInterface::AudioOutput;
        bool valid = true;
        bool isAdvanced = false;
        bool preferCardName = false;
        int cardNum = -1;
        int deviceNum = -1;

        switch (audioIface->driver()) {
        case Solid::AudioInterface::UnknownAudioDriver:
            valid = false;
            break;
        case Solid::AudioInterface::Alsa:
            if (audioIface->driverHandle().type() != QVariant::List) {
                valid = false;
            } else {
                haveAlsaDevices = true;
                // ALSA has better naming of the device than the corresponding OSS entry in HAL
                preferCardName = true;
                const QList<QVariant> handles = audioIface->driverHandle().toList();
                if (handles.size() < 1) {
                    valid = false;
                } else {
                    driver = PS::AudioDeviceAccess::AlsaDriver;
                    accessPreference += 10;

                    bool ok;
                    cardNum = handles.first().toInt(&ok);
                    if (!ok) {
                        cardNum = -1;
                    }
                    const QString &cardStr = handles.first().toString();
                    // the first is either an int (card number) or a QString (card id)
                    QString x_phononId = QLatin1String("x-phonon:CARD=") + cardStr;
                    QString fallbackId = QLatin1String("plughw:CARD=") + cardStr;
                    if (handles.size() > 1 && handles.at(1).isValid()) {
                        deviceNum = handles.at(1).toInt();
                        const QString deviceStr = handles.at(1).toString();
                        if (deviceNum == 0) {
                            // prefer DEV=0 devices over DEV>0
                            accessPreference += 1;
                        } else {
                            isAdvanced = true;
                        }
                        x_phononId += ",DEV=" + deviceStr;
                        fallbackId += ",DEV=" + deviceStr;
                        if (handles.size() > 2 && handles.at(2).isValid()) {
                            x_phononId += ",SUBDEV=" + handles.at(2).toString();
                            fallbackId += ",SUBDEV=" + handles.at(2).toString();
                        }
                    }
                    deviceIds << x_phononId << fallbackId;
                }
            }
            break;
        case Solid::AudioInterface::OpenSoundSystem:
            if (audioIface->driverHandle().type() != QVariant::String) {
                valid = false;
            } else {
                const Solid::GenericInterface *genericIface =
                    hwDevice.as<Solid::GenericInterface>();
                Q_ASSERT(genericIface);
                cardNum = genericIface->property("oss.card").toInt();
                deviceNum = genericIface->property("oss.device").toInt();
                driver = PS::AudioDeviceAccess::OssDriver;
                deviceIds << audioIface->driverHandle().toString();
            }
            break;
        }

        if (!valid || audioIface->soundcardType() == Solid::AudioInterface::Modem) {
            continue;
        }

        m_udisOfAudioDevices.append(hwDevice.udi());

        const PS::AudioDeviceAccess devAccess(deviceIds, accessPreference, driver, capture,
                playback);
        int initialPreference = 36 - deviceNum;

        QString uniqueIdPrefix = uniqueId(hwDevice, deviceNum);
        // "fix" cards that have the same identifiers, i.e. there's no way for the computer to tell
        // them apart.
        // We see that there's a problematic case if the same uniqueIdPrefix has been used for a
        // different cardNum before. In that case we need to append another number to the
        // uniqueIdPrefix. The first different cardNum gets a :i1, the second :i2, and so on.
        QList<int> &cardsForUniqueId = listOfCardNumsPerUniqueId[uniqueIdPrefix];
        if (cardsForUniqueId.isEmpty()) {
            cardsForUniqueId << cardNum;
        } else if (!cardsForUniqueId.contains(cardNum)) {
            cardsForUniqueId << cardNum;
            uniqueIdPrefix += QString(":i%1").arg(cardsForUniqueId.size() - 1);
        } else if (cardsForUniqueId.size() > 1) {
            const int listIndex = cardsForUniqueId.indexOf(cardNum);
            if (listIndex > 0) {
                uniqueIdPrefix += QString(":i%1").arg(listIndex);
            }
        }
        const PS::AudioDeviceKey pkey = {
            uniqueIdPrefix + QLatin1String(":playback"), cardNum, deviceNum
        };
        const bool needNewPlaybackDevice = playback && !playbackDevices.contains(pkey);
        const PS::AudioDeviceKey ckey = {
            uniqueIdPrefix + QLatin1String(":capture"), cardNum, deviceNum
        };
        const bool needNewCaptureDevice = capture && !captureDevices.contains(ckey);
        if (needNewPlaybackDevice || needNewCaptureDevice) {
            const QString &icon = hwDevice.icon();
            switch (audioIface->soundcardType()) {
            case Solid::AudioInterface::InternalSoundcard:
                break;
            case Solid::AudioInterface::UsbSoundcard:
                initialPreference -= 10;
                break;
            case Solid::AudioInterface::FirewireSoundcard:
                initialPreference -= 15;
                break;
            case Solid::AudioInterface::Headset:
                initialPreference -= 10;
                break;
            case Solid::AudioInterface::Modem:
                initialPreference -= 1000;
                kWarning(601) << "Modem devices should never show up!";
                break;
            }
            if (needNewPlaybackDevice) {
                PS::AudioDevice dev(audioIface->name(), icon, pkey, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                playbackDevices.insert(pkey, dev);
            }
            if (needNewCaptureDevice) {
                PS::AudioDevice dev(audioIface->name(), icon, ckey, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                captureDevices.insert(ckey, dev);
            }
        }
        if (!needNewPlaybackDevice && playback) {
            PS::AudioDevice &dev = playbackDevices[pkey];
            if (preferCardName) {
                dev.setPreferredName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
        if (!needNewCaptureDevice && capture) {
            PS::AudioDevice &dev = captureDevices[ckey];
            if (preferCardName) {
                dev.setPreferredName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
    }

    m_audioOutputDevices = playbackDevices.values();
    m_audioCaptureDevices = captureDevices.values();

#ifdef HAVE_PULSEAUDIO
    {
        pa_mainloop *mainloop = pa_mainloop_new();
        Q_ASSERT(mainloop);
        pa_mainloop_api *mainloopApi = pa_mainloop_get_api(mainloop);
        PulseDetectionUserData userData(this, mainloopApi);
        // XXX I don't want to show up in the client list. All I want to know is the list of sources
        // and sinks...
        pa_context *context = pa_context_new(mainloopApi, "KDE");
        // XXX stupid cast. report a bug about a missing enum value
        pa_context_connect(context, NULL, static_cast<pa_context_flags_t>(0), 0);
        pa_context_set_state_callback(context, &pulseContextStateCallback, &userData);
        pa_mainloop_run(mainloop, NULL);
        pa_context_disconnect(context);
        pa_mainloop_free(mainloop);
        kDebug(601) << userData.sources;
        kDebug(601) << userData.sinks;
        QMutableListIterator<PS::AudioDevice> it(m_audioOutputDevices);
        typedef QPair<PS::AudioDeviceKey, PS::AudioDeviceAccess> MyPair;
        static int uniqueDeviceNumber = -2;
        foreach (const MyPair &pair, userData.sinks) {
            it.toFront();
            bool needNewDeviceObject = true;
            while (it.hasNext()) {
                PS::AudioDevice &dev = it.next();
                if (dev.key() == pair.first) {
                    dev.addAccess(pair.second);
                    needNewDeviceObject = false;
                    continue;
                }
            }
            if (needNewDeviceObject) {
                const PS::AudioDeviceKey key = {
                    pair.second.deviceIds().first() + QLatin1String("playback"),
                    -1, --uniqueDeviceNumber
                };
                PS::AudioDevice dev(pair.first.uniqueId, QLatin1String("audio-backend-pulseaudio"), key, 0, true);
                dev.addAccess(pair.second);
                m_audioOutputDevices.append(dev);
            }
        }
        it = m_audioCaptureDevices;
        foreach (const MyPair &pair, userData.sources) {
            it.toFront();
            bool needNewDeviceObject = true;
            while (it.hasNext()) {
                PS::AudioDevice &dev = it.next();
                if (dev.key() == pair.first) {
                    dev.addAccess(pair.second);
                    needNewDeviceObject = false;
                    continue;
                }
            }
            if (needNewDeviceObject) {
                const PS::AudioDeviceKey key = {
                    pair.second.deviceIds().first() + QLatin1String("capture"),
                    -1, --uniqueDeviceNumber
                };
                PS::AudioDevice dev(pair.first.uniqueId, QLatin1String("audio-backend-pulseaudio"), key, 0, true);
                dev.addAccess(pair.second);
                m_audioCaptureDevices.append(dev);
            }
        }
    }
#endif // HAVE_PULSEAUDIO

    if (haveAlsaDevices) {
        // go through the lists and check for devices that have only OSS and remove them since
        // they're very likely bogus (Solid tells us a device can do capture and playback, even
        // though it doesn't actually know that).
        removeOssOnlyDevices(&m_audioOutputDevices);
        removeOssOnlyDevices(&m_audioCaptureDevices);
    }

    QSet<QString> alreadyFoundCards;
    foreach (const PS::AudioDevice &dev, m_audioOutputDevices) {
        alreadyFoundCards.insert(QLatin1String("AudioDevice_") + dev.key().uniqueId);
    }
    foreach (const PS::AudioDevice &dev, m_audioCaptureDevices) {
        alreadyFoundCards.insert(QLatin1String("AudioDevice_") + dev.key().uniqueId);
    }
    // now look in the config file for disconnected devices
    const QStringList &groupList = m_config->groupList();
    foreach (const QString &groupName, groupList) {
        if (alreadyFoundCards.contains(groupName) || !groupName.startsWith(QLatin1String("AudioDevice_"))) {
            continue;
        }

        const KConfigGroup cGroup(m_config, groupName);
        if (cGroup.readEntry("deleted", false)) {
            continue;
        }
        const QString &cardName = cGroup.readEntry("cardName", QString());
        const QString &iconName = cGroup.readEntry("iconName", QString());
        const int initialPreference = cGroup.readEntry("initialPreference", 0);
        const int isAdvanced = cGroup.readEntry("isAdvanced", true);
        const int deviceNumber = cGroup.readEntry("deviceNumber", -1);
        const PS::AudioDeviceKey key = { groupName.mid(12), -1, deviceNumber };
        const PS::AudioDevice dev(cardName, iconName, key, initialPreference, isAdvanced);
        if (groupName.endsWith(QLatin1String("playback"))) {
            m_audioOutputDevices << dev;
        } else {
            Q_ASSERT(groupName.endsWith(QLatin1String("capture")));
            m_audioCaptureDevices << dev;
        }
        alreadyFoundCards.insert(groupName);
    }

    // now that we know about the hardware let's see what virtual devices we can find in
    // ~/.asoundrc and /etc/asound.conf
    findVirtualDevices();

    renameDevices(&m_audioOutputDevices);
    renameDevices(&m_audioCaptureDevices);

    qSort(m_audioOutputDevices);
    qSort(m_audioCaptureDevices);

    QMutableListIterator<PS::AudioDevice> it(m_audioOutputDevices);
    while (it.hasNext()) {
        it.next().syncWithCache(m_config);
    }
    it = m_audioCaptureDevices;
    while (it.hasNext()) {
        it.next().syncWithCache(m_config);
    }

    m_config->sync();

    kDebug(601) << "Playback Devices:" << m_audioOutputDevices;
    kDebug(601) << "Capture Devices:" << m_audioCaptureDevices;
}

QByteArray PhononServer::audioDevicesIndexes(int type)
{
    QByteArray *v;
    switch (type) {
    case Phonon::AudioOutputDeviceType:
        v = &m_audioOutputDevicesIndexesCache;
        break;
    case Phonon::AudioCaptureDeviceType:
        v = &m_audioCaptureDevicesIndexesCache;
        break;
    default:
        return QByteArray();
    }
    if (v->isEmpty()) {
        updateAudioDevicesCache();
    }
    return *v;
}

QByteArray PhononServer::audioDevicesProperties(int index)
{
    if (m_audioOutputDevicesIndexesCache.isEmpty() || m_audioCaptureDevicesIndexesCache.isEmpty()) {
        updateAudioDevicesCache();
    }
    if (m_audioDevicesPropertiesCache.contains(index)) {
        return m_audioDevicesPropertiesCache.value(index);
    }
    return QByteArray();
}

bool PhononServer::isAudioDeviceRemovable(int index) const
{
    if (!m_audioDevicesPropertiesCache.contains(index)) {
        return false;
    }
    const QList<PS::AudioDevice> &deviceList = m_audioOutputDevices + m_audioCaptureDevices;
    foreach (const PS::AudioDevice &dev, deviceList) {
        if (dev.index() == index) {
            return !dev.isAvailable();
        }
    }
    return false;
}

void PhononServer::removeAudioDevices(const QList<int> &indexes)
{
    const QList<PS::AudioDevice> &deviceList = m_audioOutputDevices + m_audioCaptureDevices;
    foreach (int index, indexes) {
        foreach (const PS::AudioDevice &dev, deviceList) {
            if (dev.index() == index) {
                if (!dev.isAvailable()) {
                    dev.removeFromCache(m_config);
                }
                break;
            }
        }
    }
    m_config->sync();
    m_updateDeviceListing.start(50, this);
}

static inline QByteArray nameForDriver(PS::AudioDeviceAccess::AudioDriver d)
{
    switch (d) {
    case PS::AudioDeviceAccess::AlsaDriver:
        return "alsa";
    case PS::AudioDeviceAccess::OssDriver:
        return "oss";
    case PS::AudioDeviceAccess::PulseAudioDriver:
        return "pulseaudio";
    case PS::AudioDeviceAccess::JackdDriver:
        return "jackd";
    case PS::AudioDeviceAccess::EsdDriver:
        return "esd";
    case PS::AudioDeviceAccess::ArtsDriver:
        return "arts";
    case PS::AudioDeviceAccess::InvalidDriver:
        break;
    }
    Q_ASSERT_X(false, "nameForDriver", "unknown driver");
    return "";
}

template<class T>
inline static QByteArray streamToByteArray(const T &data)
{
    QByteArray r;
    QDataStream stream(&r, QIODevice::WriteOnly);
    stream << data;
    return r;
}

void PhononServer::updateAudioDevicesCache()
{
    QList<int> indexList;
    foreach (const PS::AudioDevice &dev, m_audioOutputDevices) {
        QHash<QByteArray, QVariant> properties;
        properties.insert("name", dev.name());
        properties.insert("description", dev.description());
        properties.insert("available", dev.isAvailable());
        properties.insert("initialPreference", dev.initialPreference());
        properties.insert("isAdvanced", dev.isAdvanced());
        properties.insert("icon", dev.icon());
        PhononDeviceAccessList deviceAccessList;
        bool first = true;
        QStringList oldDeviceIds;
        PS::AudioDeviceAccess::AudioDriver driverId = PS::AudioDeviceAccess::InvalidDriver;
        foreach (const PS::AudioDeviceAccess &access, dev.accessList()) {
            const QByteArray &driver = nameForDriver(access.driver());
            if (first) {
                driverId = access.driver();
                // Phonon 4.2 compatibility
                properties.insert("driver", driver);
                first = false;
            }
            foreach (const QString &deviceId, access.deviceIds()) {
                if (access.driver() == driverId) {
                    oldDeviceIds << deviceId;
                }
                deviceAccessList << QPair<QByteArray, QString>(driver, deviceId);
            }
        }
        properties.insert("deviceAccessList", QVariant::fromValue(deviceAccessList));

        // Phonon 4.2 compatibility
        properties.insert("deviceIds", oldDeviceIds);

        indexList << dev.index();
        m_audioDevicesPropertiesCache.insert(dev.index(), streamToByteArray(properties));
    }
    m_audioOutputDevicesIndexesCache = streamToByteArray(indexList);

    indexList.clear();
    foreach (const PS::AudioDevice &dev, m_audioCaptureDevices) {
        QHash<QByteArray, QVariant> properties;
        properties.insert("name", dev.name());
        properties.insert("description", dev.description());
        properties.insert("available", dev.isAvailable());
        properties.insert("initialPreference", dev.initialPreference());
        properties.insert("isAdvanced", dev.isAdvanced());
        properties.insert("icon", dev.icon());
        PhononDeviceAccessList deviceAccessList;
        foreach (const PS::AudioDeviceAccess &access, dev.accessList()) {
            const QByteArray &driver = nameForDriver(access.driver());
            foreach (const QString &deviceId, access.deviceIds()) {
                deviceAccessList << QPair<QByteArray, QString>(driver, deviceId);
            }
        }
        properties.insert("deviceAccessList", QVariant::fromValue(deviceAccessList));
        // no Phonon 4.2 compatibility for capture devices necessary as 4.2 never really supported
        // capture

        indexList << dev.index();
        m_audioDevicesPropertiesCache.insert(dev.index(), streamToByteArray(properties));
    }
    m_audioCaptureDevicesIndexesCache = streamToByteArray(indexList);
}

void PhononServer::deviceAdded(const QString &udi)
{
    Solid::Device device(udi);
    Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
    if (!audiohw || 0 == (audiohw->deviceType() & (Solid::AudioInterface::AudioInput | Solid::AudioInterface::AudioOutput))) {
        return;
    }
    m_updateDeviceListing.start(50, this);
}

void PhononServer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_updateDeviceListing.timerId()) {
        m_updateDeviceListing.stop();
        m_audioOutputDevices.clear();
        m_audioCaptureDevices.clear();
        m_udisOfAudioDevices.clear();
        findDevices();
        m_audioOutputDevicesIndexesCache.clear();
        m_audioCaptureDevicesIndexesCache.clear();

        QDBusMessage signal = QDBusMessage::createSignal("/modules/phononserver", "org.kde.PhononServer", "audioDevicesChanged");
        QDBusConnection::sessionBus().send(signal);
    }
}

void PhononServer::deviceRemoved(const QString &udi)
{
    if (m_udisOfAudioDevices.contains(udi)) {
        m_updateDeviceListing.start(50, this);
    }
}
