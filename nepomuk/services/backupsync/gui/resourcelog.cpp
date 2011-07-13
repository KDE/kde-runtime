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

#include "resourcelog.h"
#include "changelog.h"
#include "changelogrecord.h"

#include <QtCore/QMultiHash>

#include <Nepomuk/Types/Property>
#include <algorithm>


Nepomuk::ResourceLogMap Nepomuk::ResourceLogMap::fromChangeLogRecordList(const QList<ChangeLogRecord>& records)
{
    ResourceLogMap hash;

    //
    // Organize according to resource uri
    //
    QMultiHash<KUrl, ChangeLogRecord> multiHash;
    foreach( const ChangeLogRecord &r, records ) {
        multiHash.insert( r.st().subject().uri(), r );
    }

    //
    // Convert to MergeHash
    //
    const QList<KUrl> & keys = multiHash.uniqueKeys();
    foreach( const KUrl & key, keys ) {
        ResourceLog ms;
        ms.uri = key;

        const QList<ChangeLogRecord>& records = multiHash.values( key );
        foreach( const ChangeLogRecord & r, records ) {
            const KUrl &pred = r.st().predicate().uri();
            ms.prop.insert( pred, r );
        }
        hash.insert( ms.uri, ms );
    }

    return hash;
}


Nepomuk::ResourceLogMap Nepomuk::ResourceLogMap::fromChangeLog(const Nepomuk::ChangeLog& log)
{
    return fromChangeLogRecordList( log.toList() );
}


namespace {

    Nepomuk::ChangeLogRecord maxRecord( const QList<Nepomuk::ChangeLogRecord> & records ) {
        QList<Nepomuk::ChangeLogRecord>::const_iterator it = std::max_element( records.begin(), records.end() );
        if( it != records.constEnd() )
            return *it;
        return Nepomuk::ChangeLogRecord();
    }

    typedef QHash<Soprano::Node, Nepomuk::ChangeLogRecord> OptimizeHash;

}


void Nepomuk::ResourceLogMap::optimize()
{
    QMutableHashIterator<KUrl, ResourceLog> it( *this );
    while( it.hasNext() ) {
        it.next();

        ResourceLog & log = it.value();

        const QList<KUrl> & properties = log.prop.uniqueKeys();
        foreach( const KUrl & propUri, properties ) {
            QList<ChangeLogRecord> records = log.prop.values( propUri );

            Types::Property property( propUri );
            int maxCard = property.maxCardinality();

            if( maxCard == 1 ) {
                ChangeLogRecord max = maxRecord( records );
                records.clear();
                records.append( max );
            }
            else {
                // The records have to be sorted by timeStamp in order to optimize them
                qSort( records );

                OptimizeHash hash;
                foreach( const ChangeLogRecord & record, records ) {
                    if( record.added() ) {
                        OptimizeHash::const_iterator iter = hash.constFind( record.st().object() );
                        if( iter != hash.constEnd() ){
                            if( !iter.value().added() ) {
                                hash.remove( record.st().object() );
                            }
                        }
                        else {
                            hash.insert( record.st().object(), record );
                        }
                    }
                    else {
                        OptimizeHash::const_iterator iter = hash.constFind( record.st().object() );
                        if( iter != hash.constEnd() ) {
                            if( iter.value().added() ) {
                                hash.remove( record.st().object() );
                            }
                        }
                        else
                            hash.insert( record.st().object(), record );
                    }
                }
                records = hash.values();
                qSort( records );

                // Update the log
                log.prop.remove( propUri );
                foreach( const ChangeLogRecord & record, records )
                    log.prop.insert( propUri, record );
            }
        }

    }
}
