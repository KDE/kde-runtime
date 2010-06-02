/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "searchurllistener.h"
#include "nepomuksearchurltools.h"
#include "queryserviceinterface.h"
#include "queryinterface.h"

#include <kdirnotify.h>
#include <kdebug.h>
#include <nepomuk/result.h>

#include <QtCore/QHash>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusReply>


namespace {
    /**
     * The nepomuksearch KIO slave always requests nie:url as request property, thus, we need to do the same
     * to enforce proper Result instances
     */
    QHash<QString, QString> nieUriReqProp()
    {
        QHash<QString, QString> encodedRps;
        encodedRps.insert( QLatin1String( "reqProp1" ), KUrl( Nepomuk::Vocabulary::NIE::url() ).url() );
        return encodedRps;
    }
}


Nepomuk::SearchUrlListener::SearchUrlListener( const KUrl& queryUrl )
    : QObject( 0 ),
      m_ref( 0 ),
      m_queryUrl( queryUrl ),
      m_queryInterface( 0 )
{
    kDebug() << queryUrl;

    const QString queryService = QLatin1String( "org.kde.nepomuk.services.nepomukqueryservice" );
    if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( queryService ) ) {
        createInterface();
    }
    else {
        kDebug() << "Query service down. Waiting for it to come up to begin listening.";
    }

    // listen to the query service getting initialized
    // no need to listen for it going down. In that case nothing happens
    QDBusConnection::sessionBus().connect( queryService,
                                           QLatin1String( "/servicecontrol" ),
                                           QLatin1String( "org.kde.nepomuk.ServiceControl" ),
                                           QLatin1String( "serviceInitialized" ),
                                           this,
                                           SLOT( slotQueryServiceInitialized( bool ) ) );
}


Nepomuk::SearchUrlListener::~SearchUrlListener()
{
    kDebug() << m_queryUrl;

    if ( m_queryInterface )
        m_queryInterface->close();
    delete m_queryInterface;
}


int Nepomuk::SearchUrlListener::ref()
{
    return ++m_ref;
}


int Nepomuk::SearchUrlListener::unref()
{
    return --m_ref;
}


void Nepomuk::SearchUrlListener::slotNewEntries( const QList<Nepomuk::Query::Result>& )
{
    org::kde::KDirNotify::emitFilesAdded( m_queryUrl.url() );
}


void Nepomuk::SearchUrlListener::slotEntriesRemoved( const QStringList& entries )
{
    QStringList urls;
    foreach( const QString& uri, entries ) {
        KUrl resultUrl( m_queryUrl );
        resultUrl.addPath( Nepomuk::resourceUriToUdsName( uri ) );
        urls << resultUrl.url();
    }
    org::kde::KDirNotify::emitFilesRemoved( urls );
}


void Nepomuk::SearchUrlListener::slotQueryServiceInitialized( bool success )
{
    kDebug() << m_queryUrl << success;

    if ( success ) {
        // reconnect to our query
        createInterface();

        // inform KIO that results are available
        org::kde::KDirNotify::emitFilesAdded( m_queryUrl.url() );
    }
}


void Nepomuk::SearchUrlListener::createInterface()
{
    kDebug() << m_queryUrl;

    delete m_queryInterface;

    const QString queryService = QLatin1String( "org.kde.nepomuk.services.nepomukqueryservice" );
    org::kde::nepomuk::QueryService queryServiceInterface( queryService,
                                                           "/nepomukqueryservice",
                                                           QDBusConnection::sessionBus() );

    QDBusReply<QDBusObjectPath> r = queryServiceInterface.sparqlQuery( Nepomuk::queryFromUrl( m_queryUrl ),
                                                                       nieUriReqProp() );

    if ( r.isValid() ) {
        m_queryInterface = new org::kde::nepomuk::Query( queryService,
                                                         r.value().path(),
                                                         QDBusConnection::sessionBus() );
        connect( m_queryInterface, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ),
                 this, SLOT( slotNewEntries( QList<Nepomuk::Query::Result> ) ) );
        connect( m_queryInterface, SIGNAL( entriesRemoved( QStringList ) ),
                 this, SLOT( slotEntriesRemoved( QStringList ) ) );
        m_queryInterface->listen();
    }
}

#include "searchurllistener.moc"
