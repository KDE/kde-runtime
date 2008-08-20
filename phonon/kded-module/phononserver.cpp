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
#include <kdebug.h>
#include <KPluginFactory>
#include <KPluginLoader>
#include <QtCore/QVariant>
#include <Solid/AudioInterface>
#include <Solid/GenericInterface>
#include <Solid/Device>
#include <Solid/DeviceNotifier>

K_PLUGIN_FACTORY(PhononServerFactory,
        registerPlugin<PhononServer>();
        )
K_EXPORT_PLUGIN(PhononServerFactory("phononserver"))

Q_DECLARE_METATYPE(QList<int>)
typedef QHash<QByteArray, QVariant> ObjectDescriptionHash;
Q_DECLARE_METATYPE(ObjectDescriptionHash)

PhononServer::PhononServer(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent),
    m_config(KSharedConfig::openConfig("phonondevicesrc", KConfig::NoGlobals))
{
    findDevices();
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString &)), SLOT(deviceAdded(const QString &)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString &)), SLOT(deviceRemoved(const QString &)));
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

void PhononServer::findDevices()
{
    QHash<PS::AudioDeviceKey, PS::AudioDevice> playbackDevices;
    QHash<PS::AudioDeviceKey, PS::AudioDevice> captureDevices;
    bool haveAlsaDevices = false;

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

        const PS::AudioDeviceAccess devAccess(deviceIds, accessPreference, driver, capture,
                playback);
        int initialPreference = 36 - deviceNum;

        const QString uniqueIdPrefix = uniqueId(hwDevice, deviceNum);
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
                dev.setCardName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
        if (!needNewCaptureDevice && capture) {
            PS::AudioDevice &dev = captureDevices[ckey];
            if (preferCardName) {
                dev.setCardName(audioIface->name());
            }
            dev.addAccess(devAccess);
        }
    }
    //kDebug(601) << playbackDevices;
    //kDebug(601) << captureDevices;

    m_audioOutputDevices = playbackDevices.values();
    qSort(m_audioOutputDevices);
    m_audioCaptureDevices = captureDevices.values();
    qSort(m_audioCaptureDevices);

    if (haveAlsaDevices) {
        // go through the lists and check for devices that have only OSS and remove them since
        // they're very likely bogus (Solid tells us a device can do capture and playback, even
        // though it doesn't actually know that).
        QMutableListIterator<PS::AudioDevice> it(m_audioOutputDevices);
        while (it.hasNext()) {
            const PS::AudioDevice &dev = it.next();
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
        it = m_audioCaptureDevices;
        while (it.hasNext()) {
            const PS::AudioDevice &dev = it.next();
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

    kDebug(601) << m_audioOutputDevices;
    kDebug(601) << m_audioCaptureDevices;
}

QDBusVariant PhononServer::audioDevicesIndexes(int type)
{
    kDebug(601);
    QVariant *v;
    switch (type) {
    case Phonon::AudioOutputDeviceType:
        v = &m_audioOutputDevicesIndexesCache;
        break;
    case Phonon::AudioCaptureDeviceType:
        v = &m_audioCaptureDevicesIndexesCache;
        break;
    default:
        return QDBusVariant();
    }
    if (!v->isValid()) {
        updateAudioDevicesCache();
    }
    return QDBusVariant(*v);
}

QDBusVariant PhononServer::audioDevicesProperties(int type, int index)
{
    kDebug(601);
    QVariant *v;
    QHash<int, QVariant> *h;
    switch (type) {
    case Phonon::AudioOutputDeviceType:
        v = &m_audioOutputDevicesIndexesCache;
        h = &m_audioOutputDevicesPropertiesCache;
        break;
    case Phonon::AudioCaptureDeviceType:
        v = &m_audioCaptureDevicesIndexesCache;
        h = &m_audioCaptureDevicesPropertiesCache;
        break;
    default:
        return QDBusVariant();
    }
    if (!v->isValid()) {
        updateAudioDevicesCache();
    }
    return QDBusVariant(h->value(index));
}

void PhononServer::updateAudioDevicesCache()
{

    QMultiMap<int, int> sortedOutputIndexes;
    QMultiMap<int, int> sortedInputIndexes;
    QHash<int, QHash<QByteArray, QVariant> > outputInfos;
    QHash<int, QHash<QByteArray, QVariant> > inputInfos;

    m_audioOutputDevicesIndexesCache = QVariant::fromValue(sortedOutputIndexes.values());
    m_audioCaptureDevicesIndexesCache = QVariant::fromValue(sortedInputIndexes.values());
}

void PhononServer::deviceAdded(const QString &udi)
{
    Solid::Device device(udi);
    Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
    if (!audiohw || 0 == (audiohw->deviceType() & (Solid::AudioInterface::AudioInput | Solid::AudioInterface::AudioOutput))) {
        return;
    }
    m_audioOutputDevicesIndexesCache = QVariant();
    m_audioCaptureDevicesIndexesCache = QVariant();
}

void PhononServer::deviceRemoved(const QString &udi)
{
    Solid::Device device(udi);
    Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
    if (!audiohw || 0 == (audiohw->deviceType() & (Solid::AudioInterface::AudioInput | Solid::AudioInterface::AudioOutput))) {
        return;
    }
    m_audioOutputDevicesIndexesCache = QVariant();
    m_audioCaptureDevicesIndexesCache = QVariant();
}
