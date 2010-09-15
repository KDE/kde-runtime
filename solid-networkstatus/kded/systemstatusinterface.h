/*  This file is part of the KDE project

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin.ottens@kdab.com>

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

#ifndef NETWORKSTATUS_SYSTEMSTATUSINTERFACE_H
#define NETWORKSTATUS_SYSTEMSTATUSINTERFACE_H

#include <solid/networking.h>

class SystemStatusInterface : public QObject
{
    Q_OBJECT
protected:
    SystemStatusInterface( QObject *parent = 0 );

public:
    virtual Solid::Networking::Status status() const = 0;
    virtual bool isSupported() const = 0;

signals:
    void statusChanged( Solid::Networking::Status status );
};

#endif

