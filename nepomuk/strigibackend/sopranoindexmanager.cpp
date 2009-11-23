/*
   Copyright (C) 2007-2008 Sebastian Trueg <trueg@kde.org>

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

#include <Soprano/Client/DBusClient>
#include <Soprano/Util/MutexModel>
#include <Soprano/Client/DBusModel>
#include <Soprano/Client/LocalSocketClient>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <Nepomuk/ResourceManager>


class Strigi::Soprano::IndexManager::Private
{
public:
    Private()
        : writer( 0 ),
          reader( 0 ) {
    }

    IndexWriter* writer;
    IndexReader* reader;
};


extern "C" {
// we do not use REGISTER_STRIGI_INDEXMANAGER as we do have to perform some additional checks
STRIGI_EXPORT Strigi::IndexManager* createIndexManager( const char* )
{
    if( Nepomuk::ResourceManager::instance()->init() == 0 ) {
        return new Strigi::Soprano::IndexManager();
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

Strigi::Soprano::IndexManager::IndexManager()
{
    d = new Private;
}


Strigi::Soprano::IndexManager::~IndexManager()
{
    qDebug() << "Cleaning up SopranoIndexManager";
    delete d->reader;
    delete d->writer;
    delete d;
}


Strigi::IndexReader* Strigi::Soprano::IndexManager::indexReader()
{
    if ( !d->reader ) {
        qDebug() << "(Soprano::IndexManager) creating IndexReader";
        d->reader = new Strigi::Soprano::IndexReader( Nepomuk::ResourceManager::instance()->mainModel() );
    }

    return d->reader;
}


Strigi::IndexWriter* Strigi::Soprano::IndexManager::indexWriter()
{
    if ( !d->writer ) {
        qDebug() << "(Soprano::IndexManager) creating IndexWriter";
        d->writer = new Strigi::Soprano::IndexWriter( Nepomuk::ResourceManager::instance()->mainModel() );
    }

    return d->writer;
}
