/*
   $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $

   This file is part of the Strigi project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "sopranoindexmanager.h"
#include "sopranoindexwriter.h"
#include "sopranoindexreader.h"

#include <strigi/strigiconfig.h>

#include <Soprano/Soprano>
#include <Soprano/Client/DBusClient>
#include <Soprano/Index/IndexFilterModel>
#include <Soprano/Index/CLuceneIndex>
#include <Soprano/Util/MutexModel>
#include <Soprano/Client/DBusModel>
#include <Soprano/Client/LocalSocketClient>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QString>


namespace {
    ::Soprano::Client::LocalSocketClient* s_sopranoSocketClient = 0;
    ::Soprano::Client::DBusClient* s_sopranoDBusClient = 0;

    QString nepomukServerSocketPath() {
        QString kdeHome = getenv( "KDEHOME" );
        if ( kdeHome.isEmpty() ) {
            kdeHome = QDir::homePath() + "/.kde4";
        }
        return kdeHome + "/share/apps/nepomuk/socket";
    }
}

class Strigi::Soprano::IndexManager::Private
{
public:
    Private()
        : repository( 0 ),
          protectionModel( 0 ),
          index( 0 ),
          indexModel( 0 ),
          writer( 0 ),
          reader( 0 ) {
    }

    ::Soprano::Model* repository;
    ::Soprano::Util::MutexModel* protectionModel;
    ::Soprano::Index::CLuceneIndex* index;
    ::Soprano::Index::IndexFilterModel* indexModel;
    IndexWriter* writer;
    IndexReader* reader;
};


extern "C" {
// we do not use REGISTER_STRIGI_INDEXMANAGER as we do have to perform some additional checks
STRIGI_EXPORT Strigi::IndexManager* createIndexManager( const char* dir )
{
    if ( !s_sopranoSocketClient ) {
        s_sopranoSocketClient = new ::Soprano::Client::LocalSocketClient();
    }
    ::Soprano::Model* model = 0;
    if ( !s_sopranoSocketClient->isConnected() ) {
        QString socket = nepomukServerSocketPath();
        if ( s_sopranoSocketClient->connect( socket ) ) {
            model = s_sopranoSocketClient->createModel( "main" );
        }
        else {
            qDebug() << "(Strigi::Soprano::IndexManager) unable to connect to nepomuks server via local socket:" << socket;
        }
    }

    if ( !model ) {
        if ( !s_sopranoDBusClient ) {
            s_sopranoDBusClient = new ::Soprano::Client::DBusClient( "org.kde.nepomuk.services.nepomukstorage" );
        }

        if ( s_sopranoDBusClient->isValid() ) {
            qDebug() << "(Strigi::Soprano::IndexManager) found Soprano server.";
            model = s_sopranoDBusClient->createModel( "main" );
        }
    }

    if ( model ) {
        return new Strigi::Soprano::IndexManager( model, QString() );
    }
    else {
        return 0;
    }
}

STRIGI_EXPORT void deleteIndexManager( Strigi::IndexManager* m )
{
    delete m;
}
}

Strigi::Soprano::IndexManager::IndexManager( ::Soprano::Model* model, const QString& path )
{
    d = new Private;
    d->repository = model;
    if ( !path.isEmpty() ) {
        d->index = new ::Soprano::Index::CLuceneIndex();
        d->index->open( path, true );
        d->indexModel = new ::Soprano::Index::IndexFilterModel( d->index, model );
    }
    else {
        d->protectionModel = new ::Soprano::Util::MutexModel( ::Soprano::Util::MutexModel::ReadWriteMultiThreading, model );
    }
}


Strigi::Soprano::IndexManager::~IndexManager()
{
    qDebug() << "Cleaning up SopranoIndexManager";
    delete d->reader;
    delete d->writer;
    delete d->indexModel;
    delete d->index;
    delete d->protectionModel;
    delete d->repository;
    delete d;
}


Strigi::IndexReader* Strigi::Soprano::IndexManager::indexReader()
{
    if ( !d->reader ) {
        qDebug() << "(Soprano::IndexManager) creating IndexReader";
        if ( d->indexModel )
            d->reader = new Strigi::Soprano::IndexReader( d->indexModel );
        else
            d->reader = new Strigi::Soprano::IndexReader( d->protectionModel );
    }

    return d->reader;
}


Strigi::IndexWriter* Strigi::Soprano::IndexManager::indexWriter()
{
    if ( !d->writer ) {
        qDebug() << "(Soprano::IndexManager) creating IndexWriter";
        if ( d->indexModel )
            d->writer = new Strigi::Soprano::IndexWriter( d->indexModel );
        else
            d->writer = new Strigi::Soprano::IndexWriter( d->protectionModel );
    }

    return d->writer;
}
