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

namespace PS
{
    struct AudioDeviceKey;
    struct AudioDevice;
} // namespace PS
unsigned int qHash(const PS::AudioDeviceKey &k);
unsigned int qHash(const PS::AudioDevice &a);

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include "audiodeviceaccess.h"
#include <ksharedconfig.h>

namespace PS
{
class AudioDeviceAccess;
struct AudioDeviceKey
{
    QString uniqueId;
    int cardNumber;
    int deviceNumber;
    inline bool operator==(const AudioDeviceKey &rhs) const {
        if (uniqueId.isNull() || rhs.uniqueId.isNull()) {
            return cardNumber == rhs.cardNumber && deviceNumber == rhs.deviceNumber;
        }
        return uniqueId == rhs.uniqueId && cardNumber == rhs.cardNumber && deviceNumber == rhs.deviceNumber;
    }
};
class AudioDevice
{
    public:
        AudioDevice();
        AudioDevice(const QString &cardName, const QString &icon, const AudioDeviceKey &key,
                int pref, bool adv);
        void addAccess(const PS::AudioDeviceAccess &access);

        inline bool operator<(const AudioDevice &rhs) const
        { return m_initialPreference > rhs.m_initialPreference; }
        inline bool operator==(const AudioDevice &rhs) const { return m_key == rhs.m_key; }

        /**
         * Returns the user visible name of the device
         */
        inline const QString &name() const { return m_cardName; }

        inline void setPreferredName(const QString &name) { if (!m_dbNameOverrideFound) { m_cardName = name; } }

        inline void setUseCache(bool c) { m_useCache = c; }

        /**
         * Valid indexes are negative
         */
        inline int index() const { return m_index; }

        /**
         * UDI to identify which device was removed
         */
        //inline const QString &udi() const { return m_udi; }

        /**
         * User visible string to describe the device in detail
         */
        const QString description() const;

        const QString &icon() const { return m_icon; }

        inline bool isAvailable() const { return m_isAvailable; }
        inline bool isAdvanced() const { return m_isAdvanced; }
        inline int initialPreference() const { return m_initialPreference; }
        inline int deviceNumber() const { return m_key.deviceNumber; }
        inline const QList<AudioDeviceAccess> &accessList() const { return m_accessList; }

        void syncWithCache(const KSharedConfigPtr &config);

        inline const AudioDeviceKey &key() const { return m_key; }

    private:
        void applyHardwareDatabaseOverrides();
        friend uint qHash(const AudioDevice &);
        friend QDebug operator<<(QDebug &, const AudioDevice &);

        QList<AudioDeviceAccess> m_accessList;
        QString m_cardName;
        QString m_icon;

        AudioDeviceKey m_key;
        int m_index;
        int m_initialPreference;
        bool m_isAvailable : 1;
        bool m_isAdvanced : 1;
        bool m_dbNameOverrideFound : 1;
        bool m_useCache : 1;
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

inline uint qHash(const PS::AudioDeviceKey &k)
{
    return PS::qHash(k);
}

inline uint qHash(const PS::AudioDevice &a)
{
    return PS::qHash(a);
}

#endif // AUDIODEVICE_H
