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

#include "nepomuksearchmodule.h"
#include "searchurllistener.h"
#include "dbusoperators_p.h"
#include "timelinetools.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusServiceWatcher>

#include <Nepomuk/Vocabulary/NIE>

#include <kdebug.h>
#include <kdirnotify.h>

namespace {
    inline bool isNepomukSearchUrl( const KUrl& url )
    {
        static const char s_nepProName[] = "nepomuksearch";
        return url.protocol() == QLatin1String( s_nepProName );
    }

    inline bool isTimelineUrl( const KUrl& url )
    {
        static const char s_timelineProName[] = "timeline";
        return url.protocol() == QLatin1String( s_timelineProName );
    }

    inline bool isNepomukSearchOrTimelineUrl( const KUrl& url )
    {
        return isNepomukSearchUrl( url ) || isTimelineUrl( url );
    }
}


Nepomuk::SearchModule::SearchModule( QObject* parent, const QList<QVariant>& )
    : KDEDModule( parent )
{
    kDebug();

    Nepomuk::Query::registerDBusTypes();

    //
    // connect to serviceOwnerChanged to catch crashed clients that never unregistered
    // themselves
    //
    m_watcher = new QDBusServiceWatcher( this );
    m_watcher->setConnection( QDBusConnection::sessionBus() );
    m_watcher->setWatchMode( QDBusServiceWatcher::WatchForUnregistration );
    connect( m_watcher, SIGNAL( serviceUnregistered( const QString& ) ),
             this, SLOT( slotServiceUnregistered( const QString& ) ) );

    //
    // connect to KDirLister telling us that it entered a dir
    // We cannot use the KDirNotify class since we need the dbus context!
    //
    QDBusConnection::sessionBus().connect( QString(),
                                           QString(),
                                           org::kde::KDirNotify::staticInterfaceName(),
                                           QLatin1String( "enteredDirectory" ),
                                           this,
                                           SLOT( registerSearchUrl( QString ) ) );
    QDBusConnection::sessionBus().connect( QString(),
                                           QString(),
                                           org::kde::KDirNotify::staticInterfaceName(),
                                           QLatin1String( "leftDirectory" ),
                                           this,
                                           SLOT( unregisterSearchUrl( QString ) ) );
}


Nepomuk::SearchModule::~SearchModule()
{
    kDebug();
}


void Nepomuk::SearchModule::registerSearchUrl( const QString& urlString )
{
    const KUrl url( urlString );
    KUrl queryUrl;

    if ( isNepomukSearchUrl( url ) ) {
        queryUrl = url;
    }
    else if ( isTimelineUrl( url ) ) {
        QDate date;
        if ( Nepomuk::parseTimelineUrl( url, &date ) == Nepomuk::DayFolder )
            queryUrl = Nepomuk::buildTimelineQuery( date ).toSearchUrl();
    }

    if ( queryUrl.isValid() ) {
        kDebug() << "REGISTER REGISTER REGISTER REGISTER REGISTER REGISTER" << url;
        QHash<KUrl, SearchUrlListener*>::iterator it = m_queryHash.find( url );
        if ( it == m_queryHash.end() ) {
            SearchUrlListener* listener = new SearchUrlListener( queryUrl, url );
            listener->ref();
            m_queryHash.insert( url, listener );
        }
        else {
            it.value()->ref();
        }

        if ( calledFromDBus() ) {
            m_dbusServiceUrlHash.insert( message().service(), url );
            m_watcher->addWatchedService( message().service() );
        }
    }
}


void Nepomuk::SearchModule::unregisterSearchUrl( const QString& urlString )
{
    const KUrl url( urlString );
    if ( isNepomukSearchOrTimelineUrl( url ) ) {
        kDebug() << "UNREGISTER UNREGISTER UNREGISTER UNREGISTER UNREGISTER" << url;
        unrefUrl( url );
        if ( calledFromDBus() ) {
            m_dbusServiceUrlHash.remove( message().service(), url );
            if ( !m_dbusServiceUrlHash.contains( message().service() ) )
                m_watcher->removeWatchedService( message().service() );
        }
    }
}


QStringList Nepomuk::SearchModule::watchedSearchUrls()
{
    return KUrl::List( m_queryHash.keys() ).toStringList();
}


void Nepomuk::SearchModule::slotServiceUnregistered( const QString& serviceName )
{
    QHash<QString, KUrl>::iterator it = m_dbusServiceUrlHash.find( serviceName );
    while ( it != m_dbusServiceUrlHash.end() ) {
        unrefUrl( it.value() );
        m_dbusServiceUrlHash.erase( it );
        it = m_dbusServiceUrlHash.find( serviceName );
    }
    m_watcher->removeWatchedService( serviceName );
}


void Nepomuk::SearchModule::unrefUrl( const KUrl& url )
{
    QHash<KUrl, SearchUrlListener*>::iterator it = m_queryHash.find( url );
    if ( it != m_queryHash.end() ) {
        if ( it.value()->unref() <= 0 ) {
            it.value()->deleteLater();
            m_queryHash.erase( it );
        }
    }
}

#include <kpluginfactory.h>
#include <kpluginloader.h>

K_PLUGIN_FACTORY(NepomukSearchModuleFactory,
                 registerPlugin<Nepomuk::SearchModule>();
    )
K_EXPORT_PLUGIN(NepomukSearchModuleFactory("nepomuksearchmodule"))

#include "nepomuksearchmodule.moc"
