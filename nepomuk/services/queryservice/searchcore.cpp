/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

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

#include "searchcore.h"
#include "searchthread.h"

#include <QtCore/QEventLoop>
#include <QtCore/QPointer>

#include <Soprano/Node>

#include <KDebug>

class Nepomuk::Search::SearchCore::Private
{
public:
    Private()
        : cutOffScore( 0.25 ),
          active( false ),
          canceled( false ) {
    }

    double cutOffScore;
    QHash<QUrl, Nepomuk::Search::Result> lastResults;
    SearchThread* searchThread;

    bool active;
    bool canceled;

    QPointer<QEventLoop> eventLoop;
};


Nepomuk::Search::SearchCore::SearchCore( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->searchThread = new SearchThread( this );
    connect( d->searchThread, SIGNAL( newResult( const Nepomuk::Search::Result& ) ),
             this, SLOT( slotNewResult( const Nepomuk::Search::Result& ) ) );
    connect( d->searchThread, SIGNAL( finished() ),
             this, SLOT( slotFinished() ) );
}


Nepomuk::Search::SearchCore::~SearchCore()
{
    d->searchThread->cancel();
    delete d;
}


bool Nepomuk::Search::SearchCore::isActive() const
{
    return d->active;
}


void Nepomuk::Search::SearchCore::query( const Query& query )
{
    d->lastResults.clear();
    d->canceled = false;
    d->active = true;
    d->searchThread->query( query, cutOffScore() );
}


void Nepomuk::Search::SearchCore::cancel()
{
    d->canceled = true;
    d->active = false;
    d->searchThread->cancel();
}


void Nepomuk::Search::SearchCore::setCutOffScore( double score )
{
    d->cutOffScore = qMin( 1.0, qMax( score, 0.0 ) );
}


double Nepomuk::Search::SearchCore::cutOffScore() const
{
    return d->cutOffScore;
}


QList<Nepomuk::Search::Result> Nepomuk::Search::SearchCore::lastResults() const
{
    return d->lastResults.values();
}


void Nepomuk::Search::SearchCore::slotNewResult( const Nepomuk::Search::Result& result )
{
    if ( !d->canceled ) {
        QHash<QUrl, Result>::iterator it = d->lastResults.find( result.resourceUri() );
        if ( it == d->lastResults.end() ) {
            d->lastResults.insert( result.resourceUri(), result );
            emit newResult( result );
        }
        else {
            // FIXME: normalizing scores anyone??
            it.value().setScore( it.value().score() + result.score() );
            emit scoreChanged( it.value() );
        }
    }
}


void Nepomuk::Search::SearchCore::slotFinished()
{
    kDebug();
    d->active = false;
    if ( d->eventLoop ) {
        d->eventLoop->exit();
    }
    emit finished();
}


QList<Nepomuk::Search::Result> Nepomuk::Search::SearchCore::blockingQuery( const Query& q )
{
    kDebug();

    // cancel previous search
    if( d->eventLoop != 0 ) {
        kDebug() << "Killing previous search";
        QEventLoop* loop = d->eventLoop;
        d->eventLoop = 0;
        d->searchThread->cancel();
        loop->exit();
    }

    QEventLoop loop;
    d->eventLoop = &loop;
    query( q );
    loop.exec();
    d->eventLoop = 0;
    kDebug() << "done";
    return lastResults();
}

#include "searchcore.moc"
