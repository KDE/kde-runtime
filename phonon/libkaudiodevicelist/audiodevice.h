/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PHONON_AUDIODEVICE_H
#define PHONON_AUDIODEVICE_H

#include "kaudiodevicelist_export.h"
#include <solid/audiointerface.h>
#include <ksharedconfig.h>
#include <QtCore/QDebug>
#include <QtCore/QSharedData>
#include <QtCore/QStringList>

class KConfigGroup;

namespace Solid
{
    class AudioInterface;
} // namespace Solid

namespace Phonon
{
    class AudioDevicePrivate;

    /** \class AudioDevice audiodevice.h Phonon/AudioDevice
     * \brief Information about a soundcard of the computer.
     *
     * This class encapsulates some useful information about a soundcard of the computer. Don't
     * instanciate this class in your code, use AudioDeviceEnumerator::availableDevices().
     *
     * \see AudioDeviceEnumerator
     * \ingroup Backend
     * \author Matthias Kretz <kretz@kde.org>
     */
    class KAUDIODEVICELIST_EXPORT AudioDevice
    {
        friend class AudioDevicePrivate;
        friend class AudioDeviceEnumerator;
        friend class AudioDeviceEnumeratorPrivate;

        public:
            //static QStringList addSoftVolumeMixerControl(const AudioDevice &device, const QStringList &mixerControlNames);

            /**
             * \internal
             * Creates an invalid and empty instance.
             */
            AudioDevice();
            /**
             * Copy constructor. The data is implicitly shared, so copying is cheap.
             */
            AudioDevice(const AudioDevice &rhs);
            /**
             * Destroys the object.
             */
            ~AudioDevice();

            /**
             * Assignment operator. The data is implicitly shared, so copying is cheap.
             */
            AudioDevice &operator=(const AudioDevice &rhs);
            /**
             * Equality operator.
             */
            bool operator==(const AudioDevice &rhs) const;
            /**
             * Inequality operator.
             */
            bool operator!=(const AudioDevice &rhs) const { return !operator==(rhs); }

            /**
             * Returns the name of the soundcard. This string
             * should be shown to the user to select from multiple soundcards.
             */
            QString cardName() const;

            /**
             * Returns a list of device identifiers that your code can use in a snd_pcm_open call.
             * If the code wants to open the soundcard identified by this object it should try all
             * the device strings from start to end (they are sorted for preference).
             */
            QStringList deviceIds() const;

            /**
             * Returns an icon name used to identify the type of soundcard. Simply use
             * \code
             * KIcon icon(audioDevice.iconName());
             * \endcode
             * to get the icon.
             */
            QString iconName() const;

            /**
             * Retrieves the audio driver that should be used to access the device.
             *
             * @return the driver needed to access the device
             * @see Solid::AudioInterface::AudioDriver
             */
            Solid::AudioInterface::AudioDriver driver() const;

            /**
             * Unique index to identify the device.
             */
            int index() const;

            /**
             * Returns the initial preference for this device.
             *
             * The higher the number the more preferred the device is.
             */
            int initialPreference() const;

            /**
             * Returns whether the device is available.
             *
             * An external device can be unavailable when it's unplugged. Every device can be
             * unavailable when its driver is not loaded.
             */
            bool isAvailable() const;

            /**
             * Removes an unavailable device from the persistent store.
             *
             * \return \c true if the device was removed
             * \return \c false if the device could not be removed
             */
            bool ceaseToExist();

            /**
             * Devices that ceased to exist are invalid.
             */
            bool isValid() const;

            /**
             * Returns whether the device is a capture device.
             */
            bool isCaptureDevice() const;

            /**
             * Returns whether the device is a playback device.
             */
            bool isPlaybackDevice() const;

            /**
             * Returns whether the device is a non-standard device mostly only interesting for
             * advanced users.
             */
            bool isAdvancedDevice() const;

        private:
            AudioDevice(Solid::Device audioHw, KSharedConfig::Ptr config);
            AudioDevice(KConfigGroup &deviceGroup);
            AudioDevice(const QString &alsaDeviceName, const QString &description, KSharedConfig::Ptr config);

            QExplicitlySharedDataPointer<AudioDevicePrivate> d;
    };
} // namespace Phonon

inline QDebug operator<<(QDebug &s, const Phonon::AudioDevice &dev)
{
    s.space() << "\n-" << dev.cardName() << dev.driver() << dev.deviceIds() << "index:" << dev.index() << "preference:" << dev.initialPreference() << "avail:" << dev.isAvailable() << "advanced:" << dev.isAdvancedDevice();
    return s.space();
}

#endif // PHONON_AUDIODEVICE_H
