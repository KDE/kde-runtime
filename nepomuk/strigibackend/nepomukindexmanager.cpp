/*
   Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

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

#include "nepomukindexmanager.h"
#include "nepomukindexwriter.h"
#include "nepomukindexreader.h"

#include <strigi/strigiconfig.h>

#include <Soprano/Client/DBusClient>
#include <Soprano/Util/MutexModel>
#include <Soprano/Client/DBusModel>
#include <Soprano/Client/LocalSocketClient>

#include <QtCore/QDir>
#include <QtCore/QString>

#include <Nepomuk/ResourceManager>

#include <KDebug>


class Strigi::NepomukIndexManager::Private
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
        return new Strigi::NepomukIndexManager();
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

Strigi::NepomukIndexManager::NepomukIndexManager()
{
    d = new Private;
}


Strigi::NepomukIndexManager::~NepomukIndexManager()
{
    kDebug();
    delete d->reader;
    delete d->writer;
    delete d;
}


Strigi::IndexReader* Strigi::NepomukIndexManager::indexReader()
{
    if ( !d->reader ) {
        kDebug() << "creating IndexReader";
        d->reader = new Strigi::NepomukIndexReader( Nepomuk::ResourceManager::instance()->mainModel() );
    }

    return d->reader;
}


Strigi::IndexWriter* Strigi::NepomukIndexManager::indexWriter()
{
    if ( !d->writer ) {
        kDebug() << "creating IndexWriter";
        d->writer = new Strigi::NepomukIndexWriter( Nepomuk::ResourceManager::instance()->mainModel() );
    }

    return d->writer;
}
