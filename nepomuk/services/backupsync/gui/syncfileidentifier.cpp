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

#include "syncfileidentifier.h"
#include "changelogrecord.h"
#include "syncresource.h"

#include <QtCore/QFile>
#include <QtCore/QDir>

#include <KDebug>

#include <Nepomuk/Resource>
#include <Nepomuk/Variant>
#include <Nepomuk/Vocabulary/NIE>

int Nepomuk::SyncFileIdentifier::NextId = 0;

Nepomuk::SyncFileIdentifier::SyncFileIdentifier(const Nepomuk::SyncFile& sf)
    : ResourceIdentifier()
{
    m_id = NextId++;

    m_changeLog = sf.changeLog();
    m_identificationSet = sf.identificationSet();

    //kDebug() << "Identification Set - ";
    //kDebug() << m_identificationSet.toList();
}

Nepomuk::SyncFileIdentifier::~SyncFileIdentifier()
{
    // What do we do over here?
}

namespace {

    //
    // Removes the old home directory and replaces it with the current one
    // TODO: Make it OS independent
    //
    QUrl translateHomeUri( const QUrl & uri ) {
        QString uriString = uri.toString();

        QRegExp regEx("^file://(/home/[^/]*)(/.*)$");
        if( regEx.exactMatch( uriString ) ) {
            QString newUriString = "file://" + QDir::homePath() + regEx.cap(2);

            uriString.replace( regEx, newUriString );
            return QUrl( newUriString );
        }
        return uri;
    }
}

void Nepomuk::SyncFileIdentifier::load()
{
    if( unidentified().size() > 0 )
        return;

    //
    // Translate all file:// uri so that the home dir is correct.
    //
    QList<Soprano::Statement> identList = m_identificationSet.toList();
    QMutableListIterator<Soprano::Statement> it( identList );
    while( it.hasNext() ) {
        it.next();

        Soprano::Statement & st = it.value();
        if( st.object().isResource() && st.object().uri().scheme() == QLatin1String("file") )
            st.setObject( Soprano::Node( translateHomeUri( st.object().uri() ) ) );
    }

    //kDebug() << "After translation : ";
    //kDebug() << identList;

    addStatements( identList );
}


Nepomuk::ChangeLog Nepomuk::SyncFileIdentifier::convertedChangeLog()
{
    QList<ChangeLogRecord> masterLogRecords = m_changeLog.toList();
    kDebug() << "masterLogRecords : " << masterLogRecords.size();

    QList<ChangeLogRecord> identifiedRecords;
    QMutableListIterator<ChangeLogRecord> it( masterLogRecords );

    while( it.hasNext() ) {
        ChangeLogRecord r = it.next();

        // Identify Subject
        KUrl subUri = r.st().subject().uri();
        if( subUri.scheme() == QLatin1String("nepomuk") ) {
            KUrl newUri = mappedUri( subUri );
            if( newUri.isEmpty() )
                continue;

            r.setSubject( newUri );
        }

        // Identify object
        if( r.st().object().isResource() ) {
            KUrl objUri = r.st().object().uri();
            if( objUri.scheme() == QLatin1String("nepomuk") ) {
                KUrl newUri = mappedUri( objUri );
                if( newUri.isEmpty() )
                    continue;

                r.setObject( newUri );
            }
        }

        identifiedRecords.push_back( r );

        // Remove the statement from the masterchangerecords
        it.remove();
    }

    // Update the master change log
    m_changeLog = ChangeLog::fromList( masterLogRecords );

    return ChangeLog::fromList( identifiedRecords );
}

void Nepomuk::SyncFileIdentifier::identifyAll()
{
    Nepomuk::Sync::ResourceIdentifier::identifyAll();
}

int Nepomuk::SyncFileIdentifier::id()
{
    return m_id;
}


Nepomuk::Resource Nepomuk::SyncFileIdentifier::createNewResource(const Sync::SyncResource & simpleRes) const
{
    kDebug();
    Nepomuk::Resource res;

    if( simpleRes.isFileDataObject() ) {
        res = Nepomuk::Resource( simpleRes.nieUrl() );
        if( res.exists() ) {
            // If the resource already exists. We should not create it. This is to avoid the bug where
            // a different file with the same nie:url exists. If it was the same file, identification
            // should have found it. If it hasn't, well tough luck. No other option but to manually
            // identify
            return Resource();
        }
    }

    const QList<KUrl> & keys = simpleRes.uniqueKeys();
    foreach( const KUrl & prop, keys ) {
        //kDebug() << "Prop " << prop;

        const QList<Soprano::Node> nodeList = simpleRes.values( prop );
        res.setProperty( prop, Nepomuk::Variant::fromNodeList( nodeList ) );
    }
    return res.resourceUri();
}

bool Nepomuk::SyncFileIdentifier::runIdentification(const KUrl& uri)
{
    if( Nepomuk::Sync::ResourceIdentifier::runIdentification(uri) )
        return true;

    const Sync::SyncResource res = simpleResource( uri );

    // Add the resource if ( it is NOT a FileDataObject ) or ( if is a FileDataObject and
    // exists in the filesystem at the nie:url )
    bool shouldAdd = !res.isFileDataObject();
    if( res.isFileDataObject() ) {
        if( QFile::exists( res.nieUrl().toLocalFile() ) )
            shouldAdd = true;
    }

    if( shouldAdd ) {
        Nepomuk::Resource newRes = createNewResource( res );
        if( newRes.isValid() ) {
            forceResource( uri, newRes );
            return true;
        }
    }

    return false;
}
