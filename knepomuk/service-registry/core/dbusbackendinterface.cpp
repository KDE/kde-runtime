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

#include "dbusbackendinterface.h"
#include "dbusbackend.h"
#include "core.h"
#include "service.h"

#include <knepomuk/dbustools.h>

#include <kdebug.h>


Nepomuk::Backbone::Registry::DBus::BackendInterface::BackendInterface( Nepomuk::Backbone::Registry::DBus::Backend* parent )
    : QDBusAbstractAdaptor( parent ),
      m_backend( parent )
{
    connect( m_backend->core(), SIGNAL(serviceRegistered(const Service*)),
             this, SLOT(slotServiceRegistered(const Service*)) );
    connect( m_backend->core(), SIGNAL(serviceUnregistered(const Service*)),
             this, SLOT(slotServiceUnregistered(const Service*)) );
}


Nepomuk::Backbone::Registry::DBus::BackendInterface::~BackendInterface()
{
}


int Nepomuk::Backbone::Registry::DBus::BackendInterface::registerService( const ServiceDesc& desc )
{
    return m_backend->core()->registerService( desc, m_backend );
}


int Nepomuk::Backbone::Registry::DBus::BackendInterface::unregisterService( const ServiceDesc& desc )
{
    return m_backend->core()->unregisterService( desc, m_backend );
}


int Nepomuk::Backbone::Registry::DBus::BackendInterface::unregisterService( const QString& url )
{
    return m_backend->core()->unregisterService( url, m_backend );
}


QList<Nepomuk::Backbone::ServiceDesc>
Nepomuk::Backbone::Registry::DBus::BackendInterface::discoverServicesByName( const QString& name )
{
    const QList<const Service*> services = m_backend->core()->findServicesByName( name );
    QList<ServiceDesc> l;
    for( QList<const Service*>::const_iterator it = services.constBegin();
         it != services.constEnd(); ++it )
        l.append( (*it)->desc() );
    return l;
}


Nepomuk::Backbone::ServiceDesc
Nepomuk::Backbone::Registry::DBus::BackendInterface::discoverServiceByUrl( const QString& url )
{
    const Nepomuk::Backbone::Registry::Service* s = m_backend->core()->findServiceByUrl( url );
    if( s )
        return s->desc();
    else
        return ServiceDesc();
}


Nepomuk::Backbone::ServiceDesc
Nepomuk::Backbone::Registry::DBus::BackendInterface::discoverServiceByType( const QString& type )
{
    const Nepomuk::Backbone::Registry::Service* s = m_backend->core()->findServiceByType( type );
    if( s )
        return s->desc();
    else
        return ServiceDesc();
}


QList<Nepomuk::Backbone::ServiceDesc>
Nepomuk::Backbone::Registry::DBus::BackendInterface::discoverServicesByType( const QString& type )
{
    const QList<const Service*> services = m_backend->core()->findServicesByType( type );
    QList<ServiceDesc> l;
    for( QList<const Service*>::const_iterator it = services.constBegin();
         it != services.constEnd(); ++it )
        l.append( (*it)->desc() );
    return l;
}


QList<Nepomuk::Backbone::ServiceDesc>
Nepomuk::Backbone::Registry::DBus::BackendInterface::discoverAllServices()
{
    const QList<const Service*> services = m_backend->core()->allServices();
    QList<ServiceDesc> l;
    for( QList<const Service*>::const_iterator it = services.constBegin();
         it != services.constEnd(); ++it )
        l.append( (*it)->desc() );
    return l;
}


void Nepomuk::Backbone::Registry::DBus::BackendInterface::slotServiceRegistered( const Nepomuk::Backbone::Registry::Service* s )
{
    emit serviceRegistered( s->desc() );
}


void Nepomuk::Backbone::Registry::DBus::BackendInterface::slotServiceUnregistered( const Nepomuk::Backbone::Registry::Service* s )
{
    emit serviceUnregistered( s->desc() );
}


QString Nepomuk::Backbone::Registry::DBus::BackendInterface::discoverDBusServiceByUrl( const QString& url )
{
    return Nepomuk::Backbone::DBus::dbusServiceFromUrl( url );
}

#include "dbusbackendinterface.moc"
