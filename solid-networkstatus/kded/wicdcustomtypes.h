/*  This file is part of the KDE project
    Copyright (C) 2008 Dario Freddi <drf54321@gmail.com>

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

#ifndef WICDCUSTOMTYPES_H
#define WICDCUSTOMTYPES_H

#include <QtCore/QMetaType>
#include <QtCore/QStringList>
#include <QtDBus/QDBusArgument>

struct WicdConnectionInfo {
    int status;
    QStringList info;
};

Q_DECLARE_METATYPE(WicdConnectionInfo)

QDBusArgument &operator<<(QDBusArgument &argument, const WicdConnectionInfo &mystruct);
const QDBusArgument &operator>>(const QDBusArgument &argument, WicdConnectionInfo &mystruct);

#endif /* WICDCUSTOMTYPES_H */
