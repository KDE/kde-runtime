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

#ifndef _NEPOMUK_CORE_H_
#define _NEPOMUK_CORE_H_

#include <QObject>
#include <QList>


namespace Nepomuk {
    namespace Middleware {

	class ServiceDesc;

	namespace Registry {

	    class Service;
	    class Backend;

	    /**
	     * The Core is not much more than a map of
	     * services which allows to register new services,
	     * unregister services, and query for services.
	     */
	    class Core : public QObject
	    {
		Q_OBJECT

	    public:
		Core( QObject* parent = 0 );
		~Core();

		const QList<const Service*> allServices();

		const Service* findServiceByUrl( const QString& url );

		/*
		 * \return The first found service that provides the requested type
		 */
		const Service* findServiceByType( const QString& type );

		const QList<const Service*> findServicesByName( const QString& name );

		const QList<const Service*> findServicesByType( const QString& type );

		/**
		 * These are the error codes returned by the registerService method.
		 * These error codes should be synced with the general Nepomuk registration
		 * error codes.
		 */
		enum Errors {
		    SERVICE_REGISTERED = 0,
		    INVALID_SERVICE = -1,
		    URI_ALREDAY_REGISTERED = -2
		};

		int registerService( const ServiceDesc& desc,
				     Backend* );

		int unregisterService( const ServiceDesc& desc,
				       Backend* );

		int unregisterService( const QString& url,
				       Backend* );

	    Q_SIGNALS:
		void serviceRegistered( const QString& uri );
		void serviceUnregistered( const QString& uri );
		void serviceRegistered( const Service* );
		void serviceUnregistered( const Service* );
      
	    private:
		Service* findServiceByUrl_internal( const QString& uri );
		const Service* findServiceByType_internal( const QString& type );
		void autoStartServices();

		/**
		 * Searches for non-loaded services with the specified service
		 * type which have their X-Nepomuk-LoadOnDemand field set.
		 *
		 * \return true if a service was found and started, false otherwise.
		 */
		bool loadServiceOnDemand( const QString& typeUri );

		class Private;
		Private* d;
	    };
	}
    }
}

#endif
