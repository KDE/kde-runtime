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

#ifndef _NEPOMUK_DBUS_BACKEND_H_
#define _NEPOMUK_DBUS_BACKEND_H_

#include <QtCore/QObject>
#include <QtCore/QString>

#include "backend.h"
#include <nepomuk/result.h>


namespace Nepomuk {
    namespace Middleware {
	namespace Registry {

	    class Core;
	    class Service;

	    namespace DBus {

		class BackendInterface;

		class Backend : public QObject, public Nepomuk::Middleware::Registry::Backend
		    {
			Q_OBJECT
	
		    public:
			Backend( Core* parent = 0 );
			~Backend();
	
			Core* core() const { return m_core; }

			Result methodCall( const Message& message );

			const Service* findServiceByDBusName( const QString& name ) const;

		    private Q_SLOTS:
			void slotServiceOwnerChanged( const QString& name,
						      const QString& oldOwner,
						      const QString& newOwner );
	
		    private:
			Core* m_core;
			BackendInterface* m_registryInterface;
		    };
	    }
	}
    }
}

#endif
