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


#ifndef IDENTIFIERMODELTREE_H
#define IDENTIFIERMODELTREE_H

#include "filesystemtree.h"

namespace Soprano {
    class Statement;
}

namespace Nepomuk {

    class IdentifierModelTreeItem : public FileSystemTreeItem {
    public:
        IdentifierModelTreeItem( const QString & url, bool isFolder );

        static IdentifierModelTreeItem* fromStatementList( const QList<Soprano::Statement> & stList );

        QUrl resourceUri() const;

        QUrl type() const;
        bool identified() const;
        QUrl identifiedUri() const;

        void setUnidentified();
        void setIdentified( const QUrl & newUri );

        QString toString() const;

        void setDiscarded( bool status );
        bool discarded() const;

        void discardAll();
    private:
        QUrl m_identifiedUrl;
        bool m_discarded;
        QList<Soprano::Statement> m_statements;
        QUrl m_type;

        friend class IdentifierModelTree;
    };

    
    class IdentifierModelTree : public FileSystemTree
    {
    public:
        IdentifierModelTree();

        virtual void add(FileSystemTreeItem* item);
        IdentifierModelTreeItem* findByUri( const QUrl& uri );

        void discardAll();
    private:
        QHash<QUrl, QString> m_resUrlHash;
        friend class IdentifierModelTreeItem;
    };

}

#endif // IDENTIFIERMODELTREE_H
