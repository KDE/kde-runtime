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

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include "deviceaccess.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include <ksharedconfig.h>

namespace PS
{

struct DeviceKey
{
    QString uniqueId;
    int cardNumber;
    int deviceNumber;

    bool operator==(const DeviceKey &rhs) const;
};

class DeviceInfo
{
    public:
        DeviceInfo();
        DeviceInfo(const QString &cardName, const QString &icon, const DeviceKey &key,
                int pref, bool adv);

        void addAccess(const PS::DeviceAccess &access);

        bool operator<(const DeviceInfo &rhs) const;
        bool operator==(const DeviceInfo &rhs) const;

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
        const QList<DeviceAccess> &accessList() const;
        const DeviceKey &key() const;

        void removeFromCache(const KSharedConfigPtr &config) const;
        void syncWithCache(const KSharedConfigPtr &config);

    private:
        friend uint qHash(const DeviceInfo &);
        friend QDebug operator<<(QDebug &, const DeviceInfo &);

        void applyHardwareDatabaseOverrides();

    private:
        QString m_cardName;
        QString m_icon;

        QList<DeviceAccess> m_accessList;
        DeviceKey m_key;

        int m_index;
        int m_initialPreference;
        bool m_isAvailable : 1;
        bool m_isAdvanced : 1;
        bool m_dbNameOverrideFound : 1;
};

inline QDebug operator<<(QDebug &s, const PS::DeviceKey &k)
{
    s.nospace() << "\n    uniqueId: " << k.uniqueId
        << ", card: " << k.cardNumber
        << ", device: " << k.deviceNumber;
    return s;
}

inline QDebug operator<<(QDebug &s, const PS::DeviceInfo &a)
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

inline uint qHash(const DeviceKey &k)
{
    return ::qHash(k.uniqueId) + k.cardNumber + 101 * k.deviceNumber;
}

inline uint qHash(const DeviceInfo &a)
{
    return qHash(a.m_key);
}

} // namespace PS


#endif // DEVICEINFO_H

