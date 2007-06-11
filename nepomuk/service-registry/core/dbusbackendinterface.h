/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _NEPOMUK_DBUS_BACKEND_INTERFACE_H_
#define _NEPOMUK_DBUS_BACKEND_INTERFACE_H_

#include <QtDBus>

namespace Nepomuk {
    namespace Middleware {

	class ServiceDesc;

	namespace Registry {

	    class Service;

	    namespace DBus {

		class Backend;

		/**
		 * The BackendInterface is the DBus interface of the Service Registry.
		 * It provides methods for service registration and discovery.
		 *
		 * The methods that return service identifiers (discoverService, discoverAllServices, and
		 * allServices) return DBus service names. This is completely sufficient since each Nepomuk
		 * DBus service provides the org.semanticdektop.nepomuk.Service interface which includes
		 * a method to retrieve the Nepomuk service URI. Also no specific DBus interface or DBus object
		 * name are necessary since both are predefined by the service type.
		 *
		 * HINT: We need to use fully qualified names in the DBus interface, i.e. Nepomuk::Middleware::ServiceDesc
		 */
		class BackendInterface : public QDBusAbstractAdaptor
		{
		    Q_OBJECT
		    Q_CLASSINFO("D-Bus Interface", "org.semanticdesktop.nepomuk.ServiceRegistry")

		public:
		    BackendInterface( Backend* parent );
		    ~BackendInterface();

		public Q_SLOTS:
		    int registerService( const Nepomuk::Middleware::ServiceDesc& desc );
		    int unregisterService( const Nepomuk::Middleware::ServiceDesc& desc );
		    int unregisterService( const QString& uri );

		    Nepomuk::Middleware::ServiceDesc discoverServiceByUrl( const QString& url );
		    Nepomuk::Middleware::ServiceDesc discoverServiceByType( const QString& type );
		    QList<Nepomuk::Middleware::ServiceDesc> discoverServicesByName( const QString& name );
		    QList<Nepomuk::Middleware::ServiceDesc> discoverServicesByType( const QString& type );
		    QList<Nepomuk::Middleware::ServiceDesc> discoverAllServices();

		    // FIXME: find a better solution: either always communicate D-Bus names instead of Nepomuk URLs
		    //        or add the dbus name to the ServiceDesc (in that case we need another ServiceDesc for
		    //        the federation.
		    QString discoverDBusServiceByUrl( const QString& url );

		Q_SIGNALS:
		    void serviceRegistered( const Nepomuk::Middleware::ServiceDesc& uri );
		    void serviceUnregistered( const Nepomuk::Middleware::ServiceDesc& uri );

		    private Q_SLOTS:
		    void slotServiceRegistered( const Service* );
		    void slotServiceUnregistered( const Service* );

		private:
		    Backend* m_backend;
		};

	    }
	}
    }
}

#endif
