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

#include "dbusbackend.h"
#include "dbusbackendinterface.h"
#include "core.h"
#include "service.h"

#include <knepomuk/message.h>
#include <knepomuk/result.h>
#include <knepomuk/dbustools.h>

#include <QtDBus>

#include <kdebug.h>


Nepomuk::Backbone::Registry::DBus::Backend::Backend( Nepomuk::Backbone::Registry::Core* parent )
  : QObject( parent ),
    Nepomuk::Backbone::Registry::Backend(),
    m_core( parent )
{
  // register our ServiceDesc structure for usage with QtDBus
  qDBusRegisterMetaType<Nepomuk::Backbone::ServiceDesc>();
  qDBusRegisterMetaType<QList<Nepomuk::Backbone::ServiceDesc> >();

  m_registryInterface = new BackendInterface( this );
  QDBusConnection::sessionBus().registerService( "org.semanticdesktop.nepomuk.ServiceRegistry" );
  QDBusConnection::sessionBus().registerObject( "/org/semanticdesktop/nepomuk/ServiceRegistry", this );

  connect( QDBusConnection::sessionBus().interface(), 
	   SIGNAL(serviceOwnerChanged(const QString&, const QString&, const QString&)),
	   this, 
	   SLOT(slotServiceOwnerChanged(const QString&, const QString&, const QString&)) );
}


Nepomuk::Backbone::Registry::DBus::Backend::~Backend()
{
}


Nepomuk::Backbone::Result Nepomuk::Backbone::Registry::DBus::Backend::methodCall( const Nepomuk::Backbone::Message& message )
{
  QDBusInterface serviceProxy( Backbone::DBus::dbusServiceFromUrl( message.service() ), 
			       Backbone::DBus::dbusObjectFromType( message.service() ),
			       Backbone::DBus::dbusInterfaceFromType( message.service() ),
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


void Nepomuk::Backbone::Registry::DBus::Backend::slotServiceOwnerChanged( const QString& name,
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
      kDebug(300003) << "(Nepomuk::Backbone::Registry::DBus::Backend) Looks as if service " << name << " crashed." << endl;
      m_core->unregisterService( s->url(), this );
    }
  }
}


const Nepomuk::Backbone::Registry::Service* 
Nepomuk::Backbone::Registry::DBus::Backend::findServiceByDBusName( const QString& name ) const
{
  const QList<const Service*> l = m_core->allServices();
  for( QList<const Service*>::const_iterator it = l.begin(); it != l.end(); it++ )
    if( Backbone::DBus::dbusServiceFromUrl( (*it)->url() ) == name )
      return *it;
  return 0;
}

#include "dbusbackend.moc"
