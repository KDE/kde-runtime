/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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


#ifndef _SERVICESTUB_ADAPTOR_H_
#define _SERVICESTUB_ADAPTOR_H_

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

#include <Nepomuk/Service>


class ServiceControlAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.nepomuk.ServiceControl")
    Q_CLASSINFO("D-Bus Introspection", ""
                "  <interface name=\"org.kde.nepomuk.ServiceControl\" >\n"
                "    <method name=\"shutdown\" />\n"
                "    <method name=\"isInitialized\">\n"
                "      <arg name=\"state\" type=\"b\" direction=\"out\" />\n"
                "    </method>\n"
                "    <signal name=\"serviceInitialized\">\n"
                "      <arg name=\"success\" type=\"b\" direction=\"out\" />\n"
                "    </signal>\n"
                "  </interface>\n"
                "")

public:
    ServiceControlAdaptor( Nepomuk::Service *parent );
    virtual ~ServiceControlAdaptor();

    inline Nepomuk::Service *parent() const
    { return static_cast<Nepomuk::Service *>(QObject::parent()); }

public Q_SLOTS:
    bool isInitialized() const;

Q_SIGNALS:
    void serviceInitialized( bool success );

public Q_SLOTS:
    void shutdown();
};

#endif
