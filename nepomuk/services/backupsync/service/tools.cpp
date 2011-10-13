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
#include "identificationset.h"


int Nepomuk::saveBackupChangeLog(const QUrl& url, QSet<QUrl> & uniqueUris )
{
    const int step = 1000;
    const QString query = QString::fromLatin1("select ?r ?p ?o ?g where { "
                                              "graph ?g { ?r ?p ?o. } "
                                              "?g a nrl:InstanceBase . "
                                              "FILTER(!bif:exists( ( select (1) where { ?g a nrl:DiscardableInstanceBase . } ) )) ."
                                              "FILTER(regex(str(?r), '^nepomuk:/res/')). "
                                              "}");

    Soprano::Model * model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator iter= model->executeQuery( query, Soprano::Query::QueryLanguageSparql );

    int totalNumRecords = 0;
    int i = 0;
    ChangeLog changeLog;
    while( iter.next() ) {
        Soprano::Statement st( iter["r"], iter["p"], iter["o"], iter["g"] );

        changeLog += ChangeLogRecord( st );
        totalNumRecords++;

        uniqueUris.insert( st.subject().uri() );
        if( st.object().isResource() && st.object().uri().scheme() == QLatin1String("nepomuk") )
            uniqueUris.insert( st.object().uri() );

        if( ++i >= step ) {
            kDebug() << "Saving .. " << changeLog.size();
            changeLog.save( url );
            changeLog.clear();
            i = 0;
        }
    }

    changeLog.save( url );
    kDebug() << "Total Records : " << totalNumRecords;
    return totalNumRecords;
}

bool Nepomuk::saveBackupSyncFile(const QUrl& url)
{
    kDebug() << url;
    KTemporaryFile logFile;
    logFile.open();

    QSet<QUrl> uniqueUris;
    saveBackupChangeLog( logFile.fileName(), uniqueUris );

    KTemporaryFile identificationFile;
    identificationFile.open();
    const QUrl identUrl( identificationFile.fileName() );

    IdentificationSet::createIdentificationSet( uniqueUris, identUrl );
    return SyncFile::createSyncFile( logFile.fileName(), identUrl, url );
}
