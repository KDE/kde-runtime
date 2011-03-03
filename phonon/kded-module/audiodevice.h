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

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include "audiodeviceaccess.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include <ksharedconfig.h>

namespace PS
{

struct AudioDeviceKey
{
    QString uniqueId;
    int cardNumber;
    int deviceNumber;

    bool operator==(const AudioDeviceKey &rhs) const;
};

class AudioDevice
{
    public:
        AudioDevice();
        AudioDevice(const QString &cardName, const QString &icon, const AudioDeviceKey &key,
                int pref, bool adv);

        void addAccess(const PS::AudioDeviceAccess &access);

        bool operator<(const AudioDevice &rhs) const;
        bool operator==(const AudioDevice &rhs) const;

        /**
         * Returns the user visible name of the device
         */
        const QString &name() const;

        void setPreferredName(const QString &name);

        /**
         * Valid indexes are negative
         */
        int index() const;

        /**
         * User visible string to describe the device in detail
         */
        const QString description() const;

        const QString &icon() const;

        bool isAvailable() const;
        bool isAdvanced() const;
        int initialPreference() const;
        int deviceNumber() const;
        const QList<AudioDeviceAccess> &accessList() const;
        const AudioDeviceKey &key() const;

        void removeFromCache(const KSharedConfigPtr &config) const;
        void syncWithCache(const KSharedConfigPtr &config);

    private:
        friend uint qHash(const AudioDevice &);
        friend QDebug operator<<(QDebug &, const AudioDevice &);

        void applyHardwareDatabaseOverrides();

    private:
        QString m_cardName;
        QString m_icon;

        QList<AudioDeviceAccess> m_accessList;
        AudioDeviceKey m_key;

        int m_index;
        int m_initialPreference;
        bool m_isAvailable : 1;
        bool m_isAdvanced : 1;
        bool m_dbNameOverrideFound : 1;
};

inline QDebug operator<<(QDebug &s, const PS::AudioDeviceKey &k)
{
    s.nospace() << "\n    uniqueId: " << k.uniqueId
        << ", card: " << k.cardNumber
        << ", device: " << k.deviceNumber;
    return s;
}

inline QDebug operator<<(QDebug &s, const PS::AudioDevice &a)
{
    s.nospace() << "\n- " << a.m_cardName
        << ", icon: " << a.m_icon
        << a.m_key
        << "\n  index: " << a.m_index
        << ", initialPreference: " << a.m_initialPreference
        << ", available: " << a.m_isAvailable
        << ", advanced: " << a.m_isAdvanced
        << ", DB name override: " << a.m_dbNameOverrideFound
        << "\n  description: " << a.description()
        << "\n  access: " << a.m_accessList;
    return s;
}

inline uint qHash(const AudioDeviceKey &k)
{
    return ::qHash(k.uniqueId) + k.cardNumber + 101 * k.deviceNumber;
}

inline uint qHash(const AudioDevice &a)
{
    return qHash(a.m_key);
}

} // namespace PS


#endif // AUDIODEVICE_H
