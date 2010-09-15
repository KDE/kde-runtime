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

#ifndef NETWORKSTATUS_NETWORK_H
#define NETWORKSTATUS_NETWORK_H

#include <solid/networking.h>

class Network
{
public:
	Network( const QString & name, int status, const QString & serviceName );
	/**
	 * Update the status of this network
	 */
	void setStatus( Solid::Networking::Status status );
	/**
	 * The connection status of this network
	 */
	Solid::Networking::Status status() const;
	/**
	 * The name of this network
	 */
	QString name() const;
	void setName( const QString& name );
	/**
	 * Returns the service owning this network
	 */
	QString service() const;
	void setService( const QString& service );

private:
	Network( const Network & );
	QString m_name;
	Solid::Networking::Status m_status;
	QString m_service;
};

#endif
// vim: sw=4 ts=4
