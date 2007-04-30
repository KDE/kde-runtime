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

#include "service.h"
#include "backend.h"

#include <knepomuk/message.h>


Nepomuk::Backbone::Registry::Service::Service()
{
}


Nepomuk::Backbone::Registry::Service::~Service()
{
}


const Nepomuk::Backbone::ServiceDesc&
Nepomuk::Backbone::Registry::Service::desc() const
{
    return m_desc;
}


const QString& Nepomuk::Backbone::Registry::Service::name() const
{
    return m_desc.name;
}


const QString& Nepomuk::Backbone::Registry::Service::url() const
{
    return m_desc.url;
}


const QString& Nepomuk::Backbone::Registry::Service::type() const
{
    return m_desc.type;
}


Nepomuk::Backbone::Registry::Service*
Nepomuk::Backbone::Registry::Service::createFromDefinition( const Nepomuk::Backbone::ServiceDesc& desc,
							    Nepomuk::Backbone::Registry::Backend* backend )
{
    Service* s = new Service;
    s->m_backend = backend;
    s->m_desc = desc;
    return s;

//   // FIXME: define the final descriptor format
//   Message reply = backend->methodCall( Message( desc.url, "org.semanticdesktop.nepomuk.Service", "identificationDescriptor" ) );
//   if( reply.isValid() && reply.arguments().count() == 1 && reply.arguments()[0].type() == QVariant::String ) {
//     QDomDocument doc;
//     int line, col;
//     QString msg;
//     if( doc.setContent( reply.arguments()[0].toString(), false, &msg, &line, &col ) ) {
//       QDomElement docElem = doc.documentElement();
//       if( docElem.nodeName() == "nepomuk:service" ) {
// 	Service* s = new Service;
// 	s->m_backend = backend;
// 	s->m_desc = desc;

// 	QDomNode n = docElem.firstChild();
// 	while( !n.isNull() ) {
// 	  if( n.nodeName() == "nepomuk:uri" )
// 	    s->m_desc.url = n.toElement().text();
// 	  else if( n.nodeName() == "nepomuk:type" )
// 	    s->m_desc.type = n.toElement().text();
// 	  n = n.nextSibling();
// 	}

// 	return s;
//       }
//       else {
// 	kDebug(300003) << "(Nepomuk::Backbone::Registry::Service) Could not find main service element" << endl;
//       }
//     }
//     else {
//       kDebug(300003) << "(Nepomuk::Backbone::Registry::Service) Parse error in line " << line << " at column " << col << ": " << msg << endl;;
//     }
//   }
//   else {
//     kDebug(300003) << "(Nepomuk::Backbone::Registry::Service) Not a valid Nepomuk-KDE service: " << desc.url << endl;
//   }

//   return 0;
}
