/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef IDENTIFIERMODELTREE_H
#define IDENTIFIERMODELTREE_H

#include "filesystemtree.h"

namespace Soprano {
    class Statement;
}

namespace Nepomuk {
    
    struct IdentificationData {
        int id;
        bool identified;
        QUrl identifiedUrl;
        QList<Soprano::Statement> statements;
    };

    class IdentifierModelTreeItem : public FileSystemTreeItem<IdentificationData> {
    public:
        IdentifierModelTreeItem( const QUrl & url, bool isFolder, const IdentificationData & data );

        static IdentifierModelTreeItem* fromData(const IdentificationData & data );
        static IdentifierModelTreeItem* fromStatementList( int id, const QList<Soprano::Statement> & stList );

        int id() const;
        QUrl resourceUri() const;

        QUrl type() const;
        bool identified() const;

        void setUnidentified();
        void setIdentified( const QUrl & newUri );
    };

    
    class IdentifierModelTree : public FileSystemTree<IdentificationData>
    {
    public:
        IdentifierModelTree();

        virtual void add(FileSystemTreeItem<IdentificationData>* item);
        virtual void remove(const QString& resUri);
    private:
        QHash<QString, QUrl> m_resUrlHash;
    };

}

#endif // IDENTIFIERMODELTREE_H
