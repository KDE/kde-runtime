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

#include <QtCore/QPointer>

#include <Soprano/Node>

#include <Nepomuk/Resource>

#include <KDebug>

class Nepomuk::Query::SearchCore::Private
{
public:
    Private()
        : cutOffScore( 0.0 ), // TODO: make this configurable through the service API
          active( false ),
          canceled( false ) {
    }

    double cutOffScore;
    QHash<QUrl, Nepomuk::Query::Result> lastResults;
    SearchThread* searchThread;

    bool active;
    bool canceled;
};


Nepomuk::Query::SearchCore::SearchCore( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->searchThread = new SearchThread( this );
    connect( d->searchThread, SIGNAL( newResult( const Nepomuk::Query::Result& ) ),
             this, SLOT( slotNewResult( const Nepomuk::Query::Result& ) ) );
    connect( d->searchThread, SIGNAL( finished() ),
             this, SLOT( slotFinished() ) );
}


Nepomuk::Query::SearchCore::~SearchCore()
{
    d->searchThread->cancel();
    delete d;
}


bool Nepomuk::Query::SearchCore::isActive() const
{
    return d->active;
}


void Nepomuk::Query::SearchCore::query( const QString& query, const Nepomuk::Query::RequestPropertyMap& requestProps )
{
    d->lastResults.clear();
    d->canceled = false;
    d->active = true;
    d->searchThread->query( query, requestProps, cutOffScore() );
}


void Nepomuk::Query::SearchCore::cancel()
{
    d->canceled = true;
    d->active = false;
    d->searchThread->cancel();
}


void Nepomuk::Query::SearchCore::setCutOffScore( double score )
{
    d->cutOffScore = qMin( 1.0, qMax( score, 0.0 ) );
}


double Nepomuk::Query::SearchCore::cutOffScore() const
{
    return d->cutOffScore;
}


QList<Nepomuk::Query::Result> Nepomuk::Query::SearchCore::lastResults() const
{
    return d->lastResults.values();
}


void Nepomuk::Query::SearchCore::slotNewResult( const Nepomuk::Query::Result& result )
{
    if ( !d->canceled ) {
        QHash<QUrl, Result>::iterator it = d->lastResults.find( result.resource().resourceUri() );
        if ( it == d->lastResults.end() ) {
            d->lastResults.insert( result.resource().resourceUri(), result );
            emit newResult( result );
        }
        else {
            // FIXME: normalizing scores anyone??
            it.value().setScore( it.value().score() + result.score() );
            emit scoreChanged( it.value() );
        }
    }
}


void Nepomuk::Query::SearchCore::slotFinished()
{
    kDebug();
    d->active = false;
    emit finished();
}

#include "searchcore.moc"
