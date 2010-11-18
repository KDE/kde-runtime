/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "tools.h"
#include "changelogrecord.h"
#include "syncfile.h"
#include "changelog.h"

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/StatementIterator>

#include <Nepomuk/ResourceManager>

#include <KTemporaryFile>
#include <KDebug>


void Nepomuk::saveBackupChangeLog(const QUrl& url)
{
    const int step = 100;
    
    ChangeLog changeLog;
    
    const QString query = QString::fromLatin1("select ?r ?p ?o ?g where { graph ?g { ?r ?p ?o. } ?g a nrl:InstanceBase . FILTER(!bif:exists( ( select (1) where { ?g a nrl:DiscardableInstanceBase . } ) )) . }");
    
    Soprano::Model * model = Nepomuk::ResourceManager::instance()->mainModel();
    
    Soprano::QueryResultIterator iter= model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    
    int i=0;
    while( iter.next() ) {
        Soprano::Statement st( iter["r"], iter["p"], iter["o"], iter["g"] );
         //kDebug() << st;
        changeLog += ChangeLogRecord( st );

        if( ++i >= step ) {
            //kDebug() << "Saving .. " << changeLog.size();
            changeLog.save( url );
            changeLog.clear();
            i = 0;
        }
    }

    changeLog.save( url );
}

//TODO: This doesn't really solve the problem that the backup maybe huge and
//      large parts of the memory may get used. The proper solution would be
//      to re-implement ChangeLog and IdentSet so that they don't load everything
//      in one go.

bool Nepomuk::saveBackupSyncFile(const QUrl& url)
{
    KTemporaryFile file;
    file.open();

    saveBackupChangeLog( file.fileName() );
    ChangeLog log = ChangeLog::fromUrl( file.fileName() );
    kDebug() << "Log size: " << log.size();

    if( log.empty() ) {
        kDebug() << "Nothing to save..";
        return false;
    }
    
    SyncFile syncFile( log );
    return syncFile.save( url );
}
