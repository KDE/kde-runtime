/*  This file is part of kdebase/workspace/solid
    Copyright (C) 2005,2007 Will Stephenson <wstephenson@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KDED_NETWORKSTATUS_H
#define KDED_NETWORKSTATUS_H

#include <QStringList>

#include <KDEDModule>

#include "network.h"

class SystemStatusInterface;

class NetworkStatusModule : public KDEDModule
{
Q_OBJECT
Q_CLASSINFO( "D-Bus Interface", "org.kde.Solid.Networking" )
public:
    NetworkStatusModule(QObject* parent, const QList<QVariant>&);
    ~NetworkStatusModule();
    // Client interface
public Q_SLOTS:
    Q_SCRIPTABLE int status();
    // Service interface
    Q_SCRIPTABLE QStringList networks();
    Q_SCRIPTABLE void setNetworkStatus( const QString & networkName, int status );
    Q_SCRIPTABLE void registerNetwork( const QString & networkName, int status, const QString & serviceName );
    Q_SCRIPTABLE void unregisterNetwork( const QString & networkName );
Q_SIGNALS:
    // Client interface
    /**
     * A status change occurred affecting the overall connectivity
     * @param status The new status
     */
    void statusChanged( uint status );
protected Q_SLOTS:
    void serviceUnregistered( const QString & name );
    void solidNetworkingStatusChanged( Solid::Networking::Status status );
    void backendRegistered();
    void backendUnregistered();
protected:
    // set up embedded backend
    void init();
    // recalculate cached status
    void updateStatus();

private:
    QList<SystemStatusInterface*> backends;
    class Private;
    Private *d;
};

#endif
// vim: sw=4 ts=4
