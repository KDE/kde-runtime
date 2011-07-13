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



#ifndef CHANGELOGMERGER_H
#define CHANGELOGMERGER_H

#include "resourcemerger.h"
#include "syncresource.h"
#include "changelog.h"
#include "resourcelog.h"

#include <QtCore/QDateTime>

namespace Nepomuk {
    class ChangeLogMerger : public Sync::ResourceMerger
    {
    public:
        ChangeLogMerger( ChangeLog log );

        int id();
        void mergeChangeLog();

        /**
         * Converts the contents of the LogFile into the MergeHash
         * The LogFile can get rather large, so this could be a time consuming
         * process.
         */
        void load();

        QList<Soprano::Statement> multipleMergers() const;
        
    private:
        ChangeLog m_logFile;
        QDateTime m_minDateTime;
        
        QList<Sync::SyncResource> m_jobs;
        QList<Soprano::Statement> m_multipleMergers;
        
        /// Contains all the records from the LogFile
        ResourceLogMap m_hash;
        
        static int NextId;
        int m_id;

        KUrl m_theGraph;

        /**
         * Handles the case when the Resource's metadata has been deleted from
         * the LogFile.
         * \return true if resUri was deleted, false otherwsie
         */
        bool handleResourceDeletion( const KUrl & resUri );

        /**
         * Resolve conflicts for properties with multiple cardinalities. It works by locally
         * making all the changes in the SyncResource.
         */
        void resolveMultipleCardinality( const QList<ChangeLogRecord >& theirRecords, const QList<ChangeLogRecord >& ownRecords );
        
        /**
         * Same as multiple Cardinality resolution, but a lot faster. It figures out which statement
         * should be present by looking at the max time stamp.
         */
        void resolveSingleCardinality( const QList< Nepomuk::ChangeLogRecord >& theirRecords, const QList< Nepomuk::ChangeLogRecord >& ownRecords );
    };

}
#endif // CHANGELOGMERGER_H
