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

#ifndef SOPRANO_NEPOMUK_TRIPLE_SERVICE_CACHE_H
#define SOPRANO_NEPOMUK_TRIPLE_SERVICE_CACHE_H

#include <QObject>
#include <QTimer>

#include <soprano/statementiterator.h>
#include <soprano/resultset.h>

namespace Soprano {
    class Model;
}

class Cache : public QObject
{
    Q_OBJECT

 public:
    Cache(); 
    ~Cache();  
 
    int insert( const Soprano::StatementIterator&, int timeout = 100000 ); 
    int insert( const Soprano::ResultSet&, int timeout = 100000 ); 
 
    Soprano::StatementIterator getStatements( int listId ); 
    Soprano::ResultSet getResultSet( int listId );

    bool contains( int listId ) const;

 public Q_SLOTS:
    void remove( int listId );
 
 private:
    int getUniqueId() const;

    class Private;
    Private *d;
};

class CacheObject : public QObject
{
    Q_OBJECT

 public:
    CacheObject( int listId, const Soprano::StatementIterator&, int timeout );
    CacheObject( int listId, const Soprano::ResultSet&, int timeout );
    ~CacheObject(); 

    Soprano::StatementIterator iterator();
    Soprano::ResultSet resultSet();

 signals:
    void timeout( int listId );

 private Q_SLOTS:
    void privateTimeout();

 private:
    int m_listId;
    Soprano::StatementIterator m_sti;
    Soprano::ResultSet m_set;
    Soprano::Model* m_model;

    QTimer m_timer;
};

#endif // SOPRANO_NEPOMUK_TRIPLE_SERVICE_CACHE_H
