/*
 * This file is part of Soprano Project.
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _SOPRANO_SERVER_DBUS_OPERATORS_H_
#define _SOPRANO_SERVER_DBUS_OPERATORS_H_


#include <Soprano/Node>
#include <Soprano/Statement>
#include <Soprano/BindingSet>

#include <QtDBus/QDBusArgument>

Q_DECLARE_METATYPE(Soprano::Statement)
Q_DECLARE_METATYPE(Soprano::Node)
Q_DECLARE_METATYPE(Soprano::BindingSet)


QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::Node& );
const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::Node& );

QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::Statement& );
const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::Statement& );

QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::BindingSet& );
const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::BindingSet& );

void registerMetaTypes();

#endif
