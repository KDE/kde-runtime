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

#ifndef _NEPOMUK_SERVICE_H_
#define _NEPOMUK_SERVICE_H_

#include <QtCore/QString>

#include <knepomuk/servicedesc.h>
#include "knepregcore_export.h"

namespace Nepomuk {
    namespace Backbone {
	namespace Registry {

	    class Backend;

	    /**
	     * @author Sebastian Trueg <sebastian@trueg.de>
	     */
	    class KNEPREGCORE_EXPORT Service
		{
		public:
		    ~Service();
      
		    const ServiceDesc& desc() const;
		    const QString& name() const;
		    const QString& url() const;
		    const QString& type() const;

		    /**
		     * The backend this service was created from.
		     *
		     * This will most likely always be the DBus backend.
		     */
		    Backend* backend() const { return m_backend; }
      
		    /**
		     * Creates a Service object from a service description.
		     *
		     * Example:
		     * <pre>
		     * <nepomuk:service>
		     *   <nepomuk:uri>http://nepomuk,semanticdesktop.org/services/storage/SesameStore</nepomuk:uri>
		     *   <nepomuk:type>org.semanticdesktop.nepomuk.services.storage</nepomuk:type>
		     * </nepomuk:service>
		     *
		     * FIXME: add fields like author, copyright, and so on...
		     */
		    static Service* createFromDefinition( const ServiceDesc&,
							  Backend* backend );
      
		private:
		    Service();

		    ServiceDesc m_desc;

		    Backend* m_backend;
		};
	}
    }
}

#endif
