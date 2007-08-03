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

#include "dbusbackend.h"
#include "dbusbackendinterface.h"
#include "core.h"
#include "service.h"

#include <nepomuk/message.h>
#include <nepomuk/result.h>
#include <nepomuk/dbustools.h>

#include <QtDBus>

#include <kdebug.h>


Nepomuk::Middleware::Registry::DBus::Backend::Backend( Nepomuk::Middleware::Registry::Core* parent )
  : QObject( parent ),
    Nepomuk::Middleware::Registry::Backend(),
    m_core( parent )
{
  // register our ServiceDesc structure for usage with QtDBus
  qDBusRegisterMetaType<Nepomuk::Middleware::ServiceDesc>();
  qDBusRegisterMetaType<QList<Nepomuk::Middleware::ServiceDesc> >();

  m_registryInterface = new BackendInterface( this );
  QDBusConnection::sessionBus().registerService( "org.semanticdesktop.nepomuk.ServiceRegistry" );
  QDBusConnection::sessionBus().registerObject( "/org/semanticdesktop/nepomuk/ServiceRegistry", this );

  connect( QDBusConnection::sessionBus().interface(),
	   SIGNAL(serviceOwnerChanged(const QString&, const QString&, const QString&)),
	   this,
	   SLOT(slotServiceOwnerChanged(const QString&, const QString&, const QString&)) );
}


Nepomuk::Middleware::Registry::DBus::Backend::~Backend()
{
}


Nepomuk::Middleware::Result Nepomuk::Middleware::Registry::DBus::Backend::methodCall( const Nepomuk::Middleware::Message& message )
{
  QDBusInterface serviceProxy( Middleware::DBus::dbusServiceFromUrl( message.service() ),
			       Middleware::DBus::dbusObjectFromType( message.service() ),
			       Middleware::DBus::dbusInterfaceFromType( message.service() ),
			       QDBusConnection::sessionBus(),
			       this );

  if( !serviceProxy.isValid() ) {
    // FIXME: define proper error codes
    return Result::createSimpleResult( -1 );
  }

  QDBusMessage dbusReply = serviceProxy.callWithArgumentList( QDBus::Block,
							      message.method(),
							      message.arguments() );

  if( dbusReply.type() != QDBusMessage::ReplyMessage ) {
    // FIXME: define proper error codes
    return Result::createSimpleResult( -1 );
  }

  // FIXME: we silently assume that the method has only a single return value
  Result res = Result();
  res.setValue( dbusReply.arguments()[0] );
  return Result( res );
}


void Nepomuk::Middleware::Registry::DBus::Backend::slotServiceOwnerChanged( const QString& name,
									  const QString& oldOwner,
									  const QString& newOwner )
{
  Q_UNUSED( oldOwner );

  //
  // See if there is a registered service by that name
  // if the old owner lost the name (i.e. no new owner is set)
  //
  if( newOwner.isEmpty() ) {
    const Service* s = findServiceByDBusName( name );
    if( s ) {
      kDebug(300003) << "(Nepomuk::Middleware::Registry::DBus::Backend) Looks as if service " << name << " crashed.";
      m_core->unregisterService( s->url(), this );
    }
  }
}


const Nepomuk::Middleware::Registry::Service*
Nepomuk::Middleware::Registry::DBus::Backend::findServiceByDBusName( const QString& name ) const
{
  const QList<const Service*> l = m_core->allServices();
  for( QList<const Service*>::const_iterator it = l.begin(); it != l.end(); it++ )
    if( Middleware::DBus::dbusServiceFromUrl( (*it)->url() ) == name )
      return *it;
  return 0;
}

#include "dbusbackend.moc"
