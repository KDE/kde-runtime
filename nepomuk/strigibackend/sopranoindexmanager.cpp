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
#include "nepomukmainmodel.h"

#include <strigi/strigiconfig.h>

#include <Soprano/Client/DBusClient>
#include <Soprano/Index/IndexFilterModel>
#include <Soprano/Index/CLuceneIndex>
#include <Soprano/Util/MutexModel>
#include <Soprano/Client/DBusModel>
#include <Soprano/Client/LocalSocketClient>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QString>


class Strigi::Soprano::IndexManager::Private
{
public:
    Private()
        : repository( 0 ),
          writer( 0 ),
          reader( 0 ) {
    }

    ::Soprano::Model* repository;
    IndexWriter* writer;
    IndexReader* reader;
};


extern "C" {
// we do not use REGISTER_STRIGI_INDEXMANAGER as we do have to perform some additional checks
STRIGI_EXPORT Strigi::IndexManager* createIndexManager( const char* )
{
    Nepomuk::MainModel* model = new Nepomuk::MainModel();
    if( model->isValid() ) {
        return new Strigi::Soprano::IndexManager( model );
    }
    else {
        delete model;
        return 0;
    }
}

STRIGI_EXPORT void deleteIndexManager( Strigi::IndexManager* m )
{
    delete m;
}
}

Strigi::Soprano::IndexManager::IndexManager( ::Soprano::Model* model )
{
    d = new Private;
    d->repository = model;
}


Strigi::Soprano::IndexManager::~IndexManager()
{
    qDebug() << "Cleaning up SopranoIndexManager";
    delete d->reader;
    delete d->writer;
    delete d->repository;
    delete d;
}


Strigi::IndexReader* Strigi::Soprano::IndexManager::indexReader()
{
    if ( !d->reader ) {
        qDebug() << "(Soprano::IndexManager) creating IndexReader";
        d->reader = new Strigi::Soprano::IndexReader( d->repository );
    }

    return d->reader;
}


Strigi::IndexWriter* Strigi::Soprano::IndexManager::indexWriter()
{
    if ( !d->writer ) {
        qDebug() << "(Soprano::IndexManager) creating IndexWriter";
        d->writer = new Strigi::Soprano::IndexWriter( d->repository );
    }

    return d->writer;
}
