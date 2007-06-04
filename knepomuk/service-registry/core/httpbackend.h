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

#ifndef _NEPOMUK_HTTP_BACKEND_H_
#define _NEPOMUK_HTTP_BACKEND_H_

#include <QtCore/QObject>

#include <core/backend.h>
#include "knepregcore_export.h"

namespace Nepomuk {
    namespace Backbone {
	namespace Registry {
	    
	    namespace Http {

		class KNEPREGCORE_EXPORT Backend : public QObject, public Backend
		    {
			Q_OBJECT

		    public:
			Backend( Core* core );
			~Backend();

			Core* core() const { return m_core; }

			Result methodCall( const Message& message );

		    private:
			Core* m_core;	
		    };
	    }
	}
    }
}

#endif
