/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Sebastian Trueg <trueg@kde.org>
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

#include <core/backend.h>
#include "knepregcore_export.h"

namespace Nepomuk {
    namespace Backbone {
	namespace Registry {

	    class Core;
	    class Service;

	    namespace DBus {

		class BackendInterface;

		class KNEPREGCORE_EXPORT Backend : public QObject, public Nepomuk::Backbone::Registry::Backend
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
