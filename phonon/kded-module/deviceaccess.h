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

#ifndef DEVICEACCESS_H
#define DEVICEACCESS_H

#include <QtCore/QDebug>
#include <QtCore/QStringList>

namespace PS
{

class DeviceAccess
{
    public:
        enum AudioDriver {
            InvalidDriver = 0,
            AlsaDriver,
            OssDriver,
            JackdDriver
        };

        DeviceAccess(const QStringList &deviceIds, int accessPreference,
                AudioDriver driver, bool capture, bool playback);

        bool operator<(const DeviceAccess &rhs) const;
        bool operator==(const DeviceAccess &rhs) const;
        bool operator!=(const DeviceAccess &rhs) const;

        /**
         * UDI to identify which device was removed
         */
        const QString &udi() const;

        AudioDriver driver() const;

        const QString driverName() const;

        const QStringList &deviceIds() const;

        int accessPreference() const;

        bool isCapture() const;

        bool isPlayback() const;

    private:
        friend QDebug operator<<(QDebug &, const DeviceAccess &);

        // the udi is needed to identify a removed device
        QString m_udi;

        QStringList m_deviceIds;
        int m_accessPreference;
        AudioDriver m_driver : 16;
        bool m_capture : 8;
        bool m_playback : 8;
};

QDebug operator<<(QDebug &s, const DeviceAccess &a);

} // namespace PS

#endif // DEVICEACCESS_H
