/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "cache.h"

#include <QtCore/QMap>
#include <QtCore/QtGlobal>
#include <QtCore/QDateTime>

#include <kdebug.h>

#include <soprano/model.h>

class Cache::Private
{
public:
    Private()
    {}

    QMap<int, CacheObject *> cache;
};

Cache::Cache()
{
    d = new Private;
}

Cache::~Cache()
{
    d->cache.clear();
    delete d;
}

int Cache::getUniqueId() const
{
    static int s_counter = 1;

    int val = 0;

    do {
        val = QDateTime::currentDateTime().toTime_t() + ++s_counter;
    } while( d->cache.contains( val ) );

    return val;
}


int Cache::insert( const Soprano::StatementIterator& sti, int timeout )
{
    int id = getUniqueId();
    CacheObject *obj = new CacheObject( id, sti, timeout );
    d->cache.insert( id, obj );

    connect(obj, SIGNAL( timeout(int) ), this, SLOT( remove(int) ));

    return id;
}


int Cache::insert( const Soprano::ResultSet& set, int timeout )
{
    int id = getUniqueId();
    CacheObject *obj = new CacheObject( id, set, timeout );
    d->cache.insert( id, obj );

    connect(obj, SIGNAL( timeout(int) ), this, SLOT( remove(int) ));

    return id;
}


Soprano::StatementIterator Cache::getStatements( int listId )
{
    return d->cache.value( listId )->iterator();
}


Soprano::ResultSet Cache::getResultSet( int listId )
{
    return d->cache.value( listId )->resultSet();
}


bool Cache::contains( int listId ) const
{
    return d->cache.contains( listId );
}


void Cache::remove( int listId )
{
    if ( d->cache.contains( listId ) )
    {
        CacheObject *obj = d->cache.value ( listId );
        disconnect(obj, SIGNAL( timeout(int) ), this, SLOT( remove(int) ));

        obj->deleteLater();
        d->cache.remove( listId );
    }

    //  kDebug(300002) << "(SopranoRDFRepository::Cache) Cache size: " << d->cache.size() << endl;
}

/////////////////
// CacheObject //
/////////////////

CacheObject::CacheObject( int listId, const Soprano::StatementIterator &sti, int timeout )
    : m_listId( listId ),
      m_sti( sti ),
      m_model( 0 )
{
    if( timeout ) {
        m_timer.setSingleShot( true );
        m_timer.setInterval( timeout );
        m_timer.start();
        connect( &m_timer, SIGNAL( timeout() ), this, SLOT( privateTimeout() ) );
    }
}

CacheObject::CacheObject( int listId, const Soprano::ResultSet& set, int timeout )
    : m_listId( listId ),
      m_set( set ),
      m_model( 0 )
{
    if( timeout ) {
        m_timer.setSingleShot( true );
        m_timer.setInterval( timeout );
        m_timer.start();
        connect( &m_timer, SIGNAL( timeout() ), this, SLOT( privateTimeout() ) );
    }
}


CacheObject::~CacheObject()
{
    delete m_model;
    m_timer.stop();
}

void CacheObject::privateTimeout()
{
    kDebug(300002) << "(SopranoRDFRepository::CacheObject) Timeout of " << m_listId;
    emit timeout( m_listId );
}

Soprano::StatementIterator CacheObject::iterator()
{
    // Restart the timer
    m_timer.stop();
    m_timer.start();

    if ( !m_sti.isValid() ) {
        if ( ( m_model = m_set.model() ) ) {
            m_sti = m_model->listStatements();
        }
    }

    return m_sti;
}

Soprano::ResultSet CacheObject::resultSet()
{
    // Restart the timer
    m_timer.stop();
    m_timer.start();

    return m_set;
}

#include "cache.moc"
