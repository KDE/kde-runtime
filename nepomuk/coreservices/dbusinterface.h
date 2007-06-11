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

#ifndef _NEPOMUK_CORE_SERVICES_DBUS_INTERFACE_H_
#define _NEPOMUK_CORE_SERVICES_DBUS_INTERFACE_H_

#include <QtDBus>

#include <soprano/node.h>
#include <soprano/statement.h>

namespace Nepomuk {
    namespace CoreServices {

	class SopranoRDFRepository;

	class DBusInterface : public QDBusAbstractAdaptor
	    {
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "org.semanticdesktop.nepomuk.CoreServices.debug")

	    public:
		DBusInterface( QObject*, SopranoRDFRepository* rep );
		~DBusInterface();
	  
	    public Q_SLOTS:
		void dumpGraph( const QString& graph );
		void quit();
		Soprano::Statement testStatement();
		Soprano::Node testNode();

	    private:
		class Private;
		Private* d;
	    };
    }
}

#endif
