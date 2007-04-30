/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _DBUS_SERVICE_WAITER_H_
#define _DBUS_SERVICE_WAITER_H_

#include <QObject>
#include <QStringList>

namespace Nepomuk {
    namespace Backbone {
	namespace Registry {

	    class Core;

	    class ServiceWaiter : public QObject
		{
		    Q_OBJECT

		public:
		    ServiceWaiter( Core* );
		    ~ServiceWaiter();

		    /**
		     * Wait for a service.
		     * \param service the Nepomuk URL of the service.
		     * \param msecs The timeout in milliseconds. Set to 0 to disable
		     * the timeout and wait forever.
		     *
		     * \return true if the service was found and false if the
		     * timeout was reached.
		     */
		    bool waitForService( const QString& service, int msecs = 0 );

		    /**
		     * Wait for a set of services.
		     * \param services the Nepomuk URLs of the services.
		     * \param msecs The timeout in milliseconds. Set to 0 to disable
		     * the timeout and wait forever.
		     *
		     * \return true if the services were found and false if the
		     * timeout was reached before all services were found (which 
		     * means that some might have been found).
		     */
		    bool waitForServices( const QStringList& services, int msecs = 0 );

		signals:
		    void serviceStarted( const QString& service );
		    void timeout();

		private Q_SLOTS:
		    void slotServiceRegistered( const QString& uri );
		    void slotTimeout();

		private:
		    class Private;
		    Private* d;
		};
	}
    }
}

#endif
