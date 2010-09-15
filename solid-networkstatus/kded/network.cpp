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

#include "network.h"

#include <KDebug>

Network::Network( const QString & name, int status, const QString & serviceName )
	: m_name( name ), m_status( (Solid::Networking::Status)status ), m_service( serviceName )
{
}

void Network::setStatus( Solid::Networking::Status status )
{
	m_status = status;
}

Solid::Networking::Status Network::status() const
{
	return m_status;
}

void Network::setName( const QString& name )
{
	m_name = name;
}

QString Network::name() const
{
	return m_name;
}

QString Network::service() const
{
	return m_service;
}

void Network::setService( const QString& service )
{
	m_service = service;
}

// vim: sw=4 ts=4
