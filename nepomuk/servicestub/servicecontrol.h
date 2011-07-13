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

#ifndef _NEPOMUK_SERVICE_CONTROL_H_
#define _NEPOMUK_SERVICE_CONTROL_H_

#include <QtCore/QObject>

#include <KService>

namespace Nepomuk {
    class ServiceControl : public QObject
    {
        Q_OBJECT

    public:
        ServiceControl( const QString& serviceName, const KService::Ptr& service, QObject* parent = 0 );
        ~ServiceControl();

        static QString dbusServiceName( const QString& serviceName );

        enum Errors {
            ErrorUnknownServiceName = -9,
            ErrorServiceAlreadyRunning = -10,
            ErrorFailedToStart = -11,
            ErrorMissingDependency = -12
        };

    Q_SIGNALS:
        void serviceInitialized( bool success );

    public Q_SLOTS:
        void start();
        void setServiceInitialized( bool success );
        bool isInitialized() const;
        void shutdown();
	QString description() const;
	QString name() const;

    private:
        QString m_serviceName;
	QString m_description;
	QString m_readableName;
        KService::Ptr m_service;
        bool m_initialized;
    };
}

#endif
