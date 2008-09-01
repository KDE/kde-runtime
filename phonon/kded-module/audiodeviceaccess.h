/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) version 3.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef AUDIODEVICEACCESS_H
#define AUDIODEVICEACCESS_H

#include <QtCore/QDebug>
#include <QtCore/QStringList>

namespace PS
{
class AudioDeviceAccess
{
    public:
        enum AudioDriver {
            InvalidDriver = 0,
            AlsaDriver,
            OssDriver,
            PulseAudioDriver,
            JackdDriver,
            EsdDriver,
            ArtsDriver
        };

        inline AudioDeviceAccess(const QStringList &deviceIds, int accessPreference,
                AudioDriver driver, bool capture, bool playback)
            : m_deviceIds(deviceIds),
            m_accessPreference(accessPreference),
            m_driver(driver),
            m_capture(capture),
            m_playback(playback)
        {
        }

        inline bool operator<(const AudioDeviceAccess &rhs) const {
            return m_accessPreference > rhs.m_accessPreference; }
        inline bool operator==(const AudioDeviceAccess &rhs) const {
            return m_deviceIds == rhs.m_deviceIds && m_capture == rhs.m_capture
                && m_playback == rhs.m_playback; }
        inline bool operator!=(const AudioDeviceAccess &rhs) const { return !operator==(rhs); }

        /**
         * UDI to identify which device was removed
         */
        inline const QString &udi() const { return m_udi; }

        AudioDriver driver() const { return m_driver; }

        const QString driverName() const;

        inline const QStringList &deviceIds() const { return m_deviceIds; }

        inline int accessPreference() const { return m_accessPreference; }

        inline bool isCapture() const { return m_capture; }

        inline bool isPlayback() const { return m_playback; }

    private:
        friend QDebug operator<<(QDebug &, const AudioDeviceAccess &);

        // the udi is needed to identify a removed device
        QString m_udi;

        QStringList m_deviceIds;
        int m_accessPreference;
        AudioDriver m_driver : 16;
        bool m_capture : 8;
        bool m_playback : 8;
};

QDebug operator<<(QDebug &s, const AudioDeviceAccess &a);

} // namespace PS

#endif // AUDIODEVICEACCESS_H
