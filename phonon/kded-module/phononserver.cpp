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
#include "deviceinfo.h"

#include <kconfiggroup.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>
#include <KPluginFactory>
#include <KPluginLoader>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QRegExp>
#include <QtCore/QSettings>
#include <QtCore/QTimerEvent>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>
#include <QtCore/QVariant>
#include <Solid/AudioInterface>
#include <Solid/GenericInterface>
#include <Solid/Device>
#include <Solid/DeviceNotifier>

#include <../config-alsa.h>
#ifdef HAVE_LIBASOUND2
#include <alsa/asoundlib.h>
#endif // HAVE_LIBASOUND2

K_PLUGIN_FACTORY(PhononServerFactory,
        registerPlugin<PhononServer>();
        )
K_EXPORT_PLUGIN(PhononServerFactory("phononserver"))

PhononServer::PhononServer(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent),
    m_config(KSharedConfig::openConfig("phonondevicesrc", KConfig::SimpleConfig))
{
    findDevices();
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString &)), SLOT(deviceAdded(const QString &)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString &)), SLOT(deviceRemoved(const QString &)));

    Phonon::registerMetaTypes();
}

PhononServer::~PhononServer()
{
}

static QString uniqueId(const Solid::Device &device, int deviceNumber)
{
    Q_UNUSED(deviceNumber);
    return device.udi();
#if 0
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
#endif
}

static void renameDevices(QList<PS::DeviceInfo> *devicelist)
{
    QHash<QString, int> cardNames;
    foreach (const PS::DeviceInfo &dev, *devicelist) {
        ++cardNames[dev.name()];
    }

    // Now look for duplicate names and rename those by appending the device number
    QMutableListIterator<PS::DeviceInfo> it(*devicelist);
    while (it.hasNext()) {
        PS::DeviceInfo &dev = it.next();
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

static inline QDebug operator<<(QDebug &d, const DeviceHint &h)
{
    d.nospace() << h.name << " (" << h.description << ")";
    return d;
}

void PhononServer::findVirtualDevices()
{
#ifdef HAVE_LIBASOUND2
    QList<DeviceHint> deviceHints;
    QHash<PS::DeviceKey, PS::DeviceInfo> playbackDevices;
    QHash<PS::DeviceKey, PS::DeviceInfo> captureDevices;

    // update config to the changes on disc
    snd_config_update_free_global();
    snd_config_update();

    void **hints;
    //snd_config_update();
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
        kDebug(601) << "snd_device_name_hint failed for 'pcm'";
        return;
    }

    for (void **cStrings = hints; *cStrings; ++cStrings) {
        DeviceHint nextHint;
        char *x = snd_device_name_get_hint(*cStrings, "NAME");
        nextHint.name = QString::fromUtf8(x);
        free(x);

        if (nextHint.name.isEmpty() || nextHint.name == "null")
            continue;

        if (nextHint.name.startsWith(QLatin1String("front:")) ||
            nextHint.name.startsWith(QLatin1String("rear:")) ||
            nextHint.name.startsWith(QLatin1String("center_lfe:")) ||
            nextHint.name.startsWith(QLatin1String("surround40:")) ||
            nextHint.name.startsWith(QLatin1String("surround41:")) ||
            nextHint.name.startsWith(QLatin1String("surround50:")) ||
            nextHint.name.startsWith(QLatin1String("surround51:")) ||
            nextHint.name.startsWith(QLatin1String("surround71:"))) {
            continue;
        }

        x = snd_device_name_get_hint(*cStrings, "DESC");
        nextHint.description = QString::fromUtf8(x);
        free(x);

        deviceHints << nextHint;
    }
    snd_device_name_free_hint(hints);
    kDebug(601) << deviceHints;

    snd_config_update_free_global();
    snd_config_update();
    Q_ASSERT(snd_config);
    foreach (const DeviceHint &deviceHint, deviceHints) {
        const QString &alsaDeviceName = deviceHint.name;
        const QString &description = deviceHint.description;
        QString uniqueId = description;
        //const QString &udi = alsaDeviceName;
        bool isAdvanced = false;

        // Try to determine the card name
        const QStringList &lines = description.split('\n');
        QString cardName = lines.first();

        if (cardName.isEmpty()) {
            int cardNameStart = alsaDeviceName.indexOf("CARD=");
            int cardNameEnd;

            if (cardNameStart >= 0) {
                cardNameStart += 5;
                cardNameEnd = alsaDeviceName.indexOf(',', cardNameStart);
                if (cardNameEnd < 0)
                    cardNameEnd = alsaDeviceName.count();

                cardName = alsaDeviceName.mid(cardNameStart, cardNameEnd - cardNameStart);
            } else {
                cardName = i18n("Unknown");
            }
        }

        // Put a non-empty unique id if the description is empty
        if (uniqueId.isEmpty()) {
            uniqueId = cardName;
        }

        // Add a description to the card name, if it exists
        if (lines.size() > 1) {
            cardName = i18nc("%1 is the sound card name, %2 is the description in case it exists", "%1 (%2)", cardName, lines[1]);
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

        if (alsaDeviceName.startsWith(QLatin1String("front:")) ||
            alsaDeviceName.startsWith(QLatin1String("rear:")) ||
            alsaDeviceName.startsWith(QLatin1String("center_lfe:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround40:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround41:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround50:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround51:")) ||
            alsaDeviceName.startsWith(QLatin1String("surround71:")) ||
            alsaDeviceName.startsWith(QLatin1String("iec958:"))) {
            isAdvanced = true;
        }

        if (alsaDeviceName.startsWith(QLatin1String("front")) ||
            alsaDeviceName.startsWith(QLatin1String("center")) ||
            alsaDeviceName.startsWith(QLatin1String("rear")) ||
            alsaDeviceName.startsWith(QLatin1String("surround"))) {
            captureDevice = false;
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

        const PS::DeviceAccess access(QStringList(alsaDeviceName), 0, PS::DeviceAccess::AlsaDriver,
                captureDevice, playbackDevice);
        //dev.setUseCache(false);
        if (playbackDevice) {
            const PS::DeviceKey key = { uniqueId + QLatin1String("_playback"), -1, -1 };

            if (playbackDevices.contains(key)) {
                playbackDevices[key].addAccess(access);
            } else {
                PS::DeviceInfo dev(cardName, iconName, key, initialPreference, isAdvanced);
                dev.addAccess(access);
                playbackDevices.insert(key, dev);
            }
        }
        if (captureDevice) {
            const PS::DeviceKey key = { uniqueId + QLatin1String("_capture"), -1, -1 };

            if (captureDevices.contains(key)) {
                captureDevices[key].addAccess(access);
            } else {
                PS::DeviceInfo dev(cardName, iconName, key, initialPreference, isAdvanced);
                dev.addAccess(access);
                captureDevices.insert(key, dev);
            }
        } else {
            if (!playbackDevice) {
                kDebug(601) << deviceHint.name << " doesn't work.";
            }
        }
    }

    m_audioOutputDevices << playbackDevices.values();
    m_audioCaptureDevices << captureDevices.values();

    const QString etcFile(QLatin1String("/etc/asound.conf"));
    const QString homeFile(QDir::homePath() + QLatin1String("/.asoundrc"));
    const bool etcExists = QFile::exists(etcFile);
    const bool homeExists = QFile::exists(homeFile);
    if (etcExists || homeExists) {
        static QFileSystemWatcher *watcher = 0;
        if (!watcher) {
            watcher = new QFileSystemWatcher(this);
            connect(watcher, SIGNAL(fileChanged(QString)), SLOT(alsaConfigChanged()));
        }
        // QFileSystemWatcher stops monitoring after a file got removed. Many editors save files by
        // writing to a temp file and moving it over the other one. QFileSystemWatcher seems to
        // interpret that as removing and stops watching a file after it got modified by an editor.
        if (etcExists && !watcher->files().contains(etcFile)) {
            kDebug(601) << "setup QFileSystemWatcher for" << etcFile;
            watcher->addPath(etcFile);
        }
        if (homeExists && !watcher->files().contains(homeFile)) {
            kDebug(601) << "setup QFileSystemWatcher for" << homeFile;
            watcher->addPath(homeFile);
        }
    }
#endif // HAVE_LIBASOUND2
}

void PhononServer::alsaConfigChanged()
{
    kDebug(601);
    m_updateDevicesTimer.start(50, this);
}

static void removeOssOnlyDevices(QList<PS::DeviceInfo> *list)
{
    QMutableListIterator<PS::DeviceInfo> it(*list);
    while (it.hasNext()) {
        const PS::DeviceInfo &dev = it.next();
        if (dev.isAvailable()) {
            bool onlyOss = true;
            foreach (const PS::DeviceAccess &a, dev.accessList()) {
                if (a.driver() != PS::DeviceAccess::OssDriver) {
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

void PhononServer::findDevices()
{
    QHash<PS::DeviceKey, PS::DeviceInfo> playbackDevices;
    QHash<PS::DeviceKey, PS::DeviceInfo> captureDevices;
    bool haveAlsaDevices = false;
    QHash<QString, QList<int> > listOfCardNumsPerUniqueId;

    KConfigGroup globalConfigGroup(m_config, "Globals");
    //const int cacheVersion = globalConfigGroup.readEntry("CacheVersion", 0);
    // cacheVersion 1 is KDE 4.1, 0 is KDE 4.0

    const QList<Solid::Device> &allHwDevices =
        Solid::Device::listFromQuery("AudioInterface.deviceType & 'AudioInput|AudioOutput'");

    kDebug(601) << "Solid gives" << allHwDevices.count() << "devices";

    foreach (const Solid::Device &hwDevice, allHwDevices) {
        const Solid::AudioInterface *audioIface = hwDevice.as<Solid::AudioInterface>();

        QStringList deviceIds;
        int accessPreference = 0;
        PS::DeviceAccess::DeviceDriverType driver = PS::DeviceAccess::InvalidDriver;
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
                    driver = PS::DeviceAccess::AlsaDriver;
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
                driver = PS::DeviceAccess::OssDriver;
                deviceIds << audioIface->driverHandle().toString();
            }
            break;
        }

        if (!valid || audioIface->soundcardType() == Solid::AudioInterface::Modem) {
            continue;
        }

        m_udisOfAudioDevices.append(hwDevice.udi());

        const PS::DeviceAccess devAccess(deviceIds, accessPreference, driver, capture,
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
        const PS::DeviceKey pkey = {
            uniqueIdPrefix + QLatin1String(":playback"), cardNum, deviceNum
        };
        const bool needNewPlaybackDevice = playback && !playbackDevices.contains(pkey);
        const PS::DeviceKey ckey = {
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
                PS::DeviceInfo dev(audioIface->name(), icon, pkey, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                playbackDevices.insert(pkey, dev);
            }
            if (needNewCaptureDevice) {
                PS::DeviceInfo dev(audioIface->name(), icon, ckey, initialPreference, isAdvanced);
                dev.addAccess(devAccess);
                captureDevices.insert(ckey, dev);
            }
        }
        if (!needNewPlaybackDevice && playback) {
            PS::DeviceInfo &dev = playbackDevices[pkey];
            if (preferCardName) {
                dev.setPreferredName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
        if (!needNewCaptureDevice && capture) {
            PS::DeviceInfo &dev = captureDevices[ckey];
            if (preferCardName) {
                dev.setPreferredName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
    }

    m_audioOutputDevices = playbackDevices.values();
    m_audioCaptureDevices = captureDevices.values();

    if (haveAlsaDevices) {
        // go through the lists and check for devices that have only OSS and remove them since
        // they're very likely bogus (Solid tells us a device can do capture and playback, even
        // though it doesn't actually know that).
        removeOssOnlyDevices(&m_audioOutputDevices);
        removeOssOnlyDevices(&m_audioCaptureDevices);
    }

    // now that we know about the hardware let's see what virtual devices we can find in
    // ~/.asoundrc and /etc/asound.conf
    findVirtualDevices();

    QSet<QString> alreadyFoundCards;
    foreach (const PS::DeviceInfo &dev, m_audioOutputDevices) {
        alreadyFoundCards.insert(QLatin1String("AudioDevice_") + dev.key().uniqueId);
    }
    foreach (const PS::DeviceInfo &dev, m_audioCaptureDevices) {
        alreadyFoundCards.insert(QLatin1String("AudioDevice_") + dev.key().uniqueId);
    }
    // now look in the config file for disconnected devices
    const QStringList &groupList = m_config->groupList();
    QStringList askToRemove;
    QList<int> askToRemoveIndexes;
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
        const bool hotpluggable = cGroup.readEntry("hotpluggable", true);
        const int initialPreference = cGroup.readEntry("initialPreference", 0);
        const int isAdvanced = cGroup.readEntry("isAdvanced", true);
        const int deviceNumber = cGroup.readEntry("deviceNumber", -1);
        const PS::DeviceKey key = { groupName.mid(12), -1, deviceNumber };
        const PS::DeviceInfo dev(cardName, iconName, key, initialPreference, isAdvanced);
        const bool isPlayback = groupName.endsWith(QLatin1String("playback"));
        if (!hotpluggable) {
            const QSettings phononSettings(QLatin1String("kde.org"), QLatin1String("libphonon"));
            if (isAdvanced &&
                    phononSettings.value(QLatin1String("General/HideAdvancedDevices"), true).toBool()) {
                dev.removeFromCache(m_config);
                continue;
            } else {
                askToRemove << (isPlayback ? i18n("Output: %1", cardName) :
                        i18n("Capture: %1", cardName));
                askToRemoveIndexes << cGroup.readEntry("index", 0);
            }
        }
        if (isPlayback) {
            m_audioOutputDevices << dev;
        } else if (!groupName.endsWith(QLatin1String("capture"))) {
            // this entry shouldn't be here
            m_config->deleteGroup(groupName);
        } else {
            m_audioCaptureDevices << dev;
        }
        alreadyFoundCards.insert(groupName);
    }
    if (!askToRemove.isEmpty()) {
        qSort(askToRemove);
        QMetaObject::invokeMethod(this, "askToRemoveDevices", Qt::QueuedConnection,
                Q_ARG(QStringList, askToRemove), Q_ARG(QList<int>, askToRemoveIndexes));
    }

    renameDevices(&m_audioOutputDevices);
    renameDevices(&m_audioCaptureDevices);

    qSort(m_audioOutputDevices);
    qSort(m_audioCaptureDevices);

    QMutableListIterator<PS::DeviceInfo> it(m_audioOutputDevices);
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
    const QList<PS::DeviceInfo> &deviceList = m_audioOutputDevices + m_audioCaptureDevices;
    foreach (const PS::DeviceInfo &dev, deviceList) {
        if (dev.index() == index) {
            return !dev.isAvailable();
        }
    }
    return false;
}

void PhononServer::removeAudioDevices(const QList<int> &indexes)
{
    const QList<PS::DeviceInfo> &deviceList = m_audioOutputDevices + m_audioCaptureDevices;
    foreach (int index, indexes) {
        foreach (const PS::DeviceInfo &dev, deviceList) {
            if (dev.index() == index) {
                if (!dev.isAvailable()) {
                    dev.removeFromCache(m_config);
                }
                break;
            }
        }
    }
    m_config->sync();
    m_updateDevicesTimer.start(50, this);
}

static inline QByteArray nameForDriver(PS::DeviceAccess::DeviceDriverType d)
{
    switch (d) {
    case PS::DeviceAccess::AlsaDriver:
        return "alsa";
    case PS::DeviceAccess::OssDriver:
        return "oss";
    case PS::DeviceAccess::JackdDriver:
        return "jackd";
    case PS::DeviceAccess::InvalidDriver:
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
    foreach (const PS::DeviceInfo &dev, m_audioOutputDevices) {
        QHash<QByteArray, QVariant> properties;
        properties.insert("name", dev.name());
        properties.insert("description", dev.description());
        properties.insert("available", dev.isAvailable());
        properties.insert("initialPreference", dev.initialPreference());
        properties.insert("isAdvanced", dev.isAdvanced());
        properties.insert("icon", dev.icon());
        Phonon::DeviceAccessList deviceAccessList;
        bool first = true;
        QStringList oldDeviceIds;
        PS::DeviceAccess::DeviceDriverType driverId = PS::DeviceAccess::InvalidDriver;
        foreach (const PS::DeviceAccess &access, dev.accessList()) {
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
                deviceAccessList << Phonon::DeviceAccess(driver, deviceId);
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
    foreach (const PS::DeviceInfo &dev, m_audioCaptureDevices) {
        QHash<QByteArray, QVariant> properties;
        properties.insert("name", dev.name());
        properties.insert("description", dev.description());
        properties.insert("available", dev.isAvailable());
        properties.insert("initialPreference", dev.initialPreference());
        properties.insert("isAdvanced", dev.isAdvanced());
        properties.insert("icon", dev.icon());
        Phonon::DeviceAccessList deviceAccessList;
        foreach (const PS::DeviceAccess &access, dev.accessList()) {
            const QByteArray &driver = nameForDriver(access.driver());
            foreach (const QString &deviceId, access.deviceIds()) {
                deviceAccessList << Phonon::DeviceAccess(driver, deviceId);
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
    m_updateDevicesTimer.start(50, this);
}

void PhononServer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_updateDevicesTimer.timerId()) {
        m_updateDevicesTimer.stop();
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
        m_updateDevicesTimer.start(50, this);
    }
}

void PhononServer::askToRemoveDevices(const QStringList &devList, const QList<int> &indexes)
{
    const QString &dontAskAgainName = QLatin1String("phonon_forget_devices_") +
        devList.join(QLatin1String("_"));
    KMessageBox::ButtonCode result;
    if (!KMessageBox::shouldBeShownYesNo(dontAskAgainName, result)) {
        if (result == KMessageBox::Yes) {
            kDebug(601) << "removeAudioDevices" << indexes;
            removeAudioDevices(indexes);
        }
        return;
    }

    class MyDialog: public KDialog
    {
        public:
            MyDialog() : KDialog(0, Qt::Dialog) {}

        protected:
            virtual void slotButtonClicked(int button)
            {
                if (button == KDialog::User1) {
                    kDebug(601) << "start kcm_phonon";
                    KProcess::startDetached(QLatin1String("kcmshell4"), QStringList(QLatin1String("kcm_phonon")));
                    reject();
                } else {
                    KDialog::slotButtonClicked(button);
                }
            }
    } *dialog = new MyDialog;
    dialog->setPlainCaption(i18n("Removed Sound Devices"));
    dialog->setButtons(KDialog::Yes | KDialog::No | KDialog::User1);
    KIcon icon("audio-card");
    dialog->setWindowIcon(icon);
    dialog->setModal(false);
    KGuiItem yes(KStandardGuiItem::yes());
    yes.setToolTip(i18n("Forget about the sound devices."));
    dialog->setButtonGuiItem(KDialog::Yes, yes);
    dialog->setButtonGuiItem(KDialog::No, KStandardGuiItem::no());
    dialog->setButtonGuiItem(KDialog::User1, KGuiItem(i18nc("short string for a button, it opens "
                    "the Phonon page of System Settings", "Manage Devices"),
                KIcon("preferences-system"),
                i18n("Open the System Settings page for sound device configuration where you can "
                    "manually remove disconnected devices from the cache.")));
    dialog->setEscapeButton(KDialog::No);
    dialog->setDefaultButton(KDialog::User1);

    bool checkboxResult = false;
    int res = KMessageBox::createKMessageBox(dialog, icon,
            i18n("<html><p>KDE detected that one or more internal sound devices were removed.</p>"
                "<p><b>Do you want KDE to permanently forget about these devices?</b></p>"
                "<p>This is the list of devices KDE thinks can be removed:<ul><li>%1</li></ul></p></html>",
            devList.join(QLatin1String("</li><li>"))),
            QStringList(),
            i18n("Do not ask again for these devices"),
            &checkboxResult, KMessageBox::Notify);
    result = (res == KDialog::Yes ? KMessageBox::Yes : KMessageBox::No);
    if (result == KMessageBox::Yes) {
        kDebug(601) << "removeAudioDevices" << indexes;
        removeAudioDevices(indexes);
    }
    if (checkboxResult) {
        KMessageBox::saveDontShowAgainYesNo(dontAskAgainName, result);
    }
}
