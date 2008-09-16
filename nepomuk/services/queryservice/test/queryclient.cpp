/*
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include "queryclient.h"
#include "dbusoperators.h"
#include "result.h"
#include "query.h"
#include "queryparser.h"
#include "queryserviceclient.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>

#include <QDebug>
#include <QtCore/QList>



QueryClient::QueryClient( const QString& query, const QStringList& rps )
    : QObject()
{
    m_client = new Nepomuk::Search::QueryServiceClient( this );
    connect( m_client, SIGNAL( newEntries( const QList<Nepomuk::Search::Result>& ) ),
             this, SLOT( slotNewEntries( const QList<Nepomuk::Search::Result>& ) ) );
    connect( m_client, SIGNAL( entriesRemoved( const QList<QUrl>& ) ),
             this, SLOT( slotEntriesRemoved( const QList<QUrl>& ) ) );
    connect( m_client, SIGNAL( finishedListing() ),
             this, SLOT( slotFinishedListing() ) );

    Nepomuk::Search::Query q = Nepomuk::Search::QueryParser::parseQuery( query );
    foreach( const QString& rp, rps ) {
        q.addRequestProperty( QUrl( rp ) );
    }
    qDebug() << "parsed query:" << q;

    m_client->query( q );
//     QDBusInterface iface( "org.kde.nepomuk.services.nepomukqueryservice", "/nepomukqueryservice", "org.kde.nepomuk.Search" );
//     QDBusReply<QDBusObjectPath> pathReply = iface.call( "query", QVariant::fromValue( q ) );
//     QString path = pathReply.value().path();
//     qDebug() << "Connecting to query object" << path;
//     Q_ASSERT( QDBusConnection::sessionBus().connect( "org.kde.nepomuk.services.nepomukqueryservice",
//                                                      path,
//                                                      "org.kde.nepomuk.Query",
//                                                      "entriesRemoved",
//                                                      this,
//                                                      SLOT( slotEntriesRemoved( const QStringList& ) ) ) );
//     Q_ASSERT( QDBusConnection::sessionBus().connect( "org.kde.nepomuk.services.nepomukqueryservice",
//                                                      path,
//                                                      "org.kde.nepomuk.Query",
//                                                      "finishedListing",
//                                                      this,
//                                                      SLOT( slotFinishedListing() ) ) );
//     Q_ASSERT( QDBusConnection::sessionBus().connect( "org.kde.nepomuk.services.nepomukqueryservice",
//                                                      path,
//                                                      "org.kde.nepomuk.Query",
//                                                      "newEntries",
//                                                      this,
//                                                      SLOT( slotNewEntries( const QList<Nepomuk::Search::Result>& ) ) ) );

//     QDBusInterface( "org.kde.nepomuk.services.nepomukqueryservice", path, "org.kde.nepomuk.Query" ).call( "list" );
}


QueryClient::~QueryClient()
{
}


void QueryClient::slotNewEntries( const QList<Nepomuk::Search::Result>& rl )
{
    QTextStream s( stdout );
    s << "New entries: " << endl;
    foreach( const Nepomuk::Search::Result& r, rl ) {
        s << "   " << r.resourceUri().toString() << " (Score: " << r.score() << ")";
        QHash<QUrl, Soprano::Node> rps = r.requestProperties();
        s << " (";
        for ( QHash<QUrl, Soprano::Node>::const_iterator it = rps.constBegin();
              it != rps.constEnd(); ++it ) {
            s << it.key() << ": " << it.value() << "; ";
        }
        s << ")" << endl;
    }
}


void QueryClient::slotEntriesRemoved( const QList<QUrl>& l )
{
    QTextStream s( stdout );
    s << "Entries removed: ";
    foreach( const QUrl& url, l ) {
        s << url.toString();
    }
    s << endl;
}


void QueryClient::slotFinishedListing()
{
    QTextStream s( stdout );
    s << "Finished listing" << endl;
}

#include "queryclient.moc"
