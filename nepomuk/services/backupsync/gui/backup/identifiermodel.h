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


#ifndef IDENTIFIERMODEL_H
#define IDENTIFIERMODEL_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QMap>
#include <Soprano/Statement>

namespace Nepomuk {

    class IdentifierModelTree;
    class IdentifierModelTreeItem;

    class IdentifierModel : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        
        enum CustomRoles {
            ResourceRole = 67823,
            LabelRole = 436786,
            SizeRole = 64876,
            TypeRole = 25325,
            IdentifiedResourceRole = 6784787,
            DiscardedRole = 687589
        };
        
        IdentifierModel(QObject* parent = 0);
        virtual ~IdentifierModel();

        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& child) const;
        virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const;
        
    public Q_SLOTS:
        /**
         * Used for debugging & testing.
         */
        void debug_notIdentified( int id, const QString & resUri, const QString & nieUrl, bool folder=false );

        /**
         * Used for debugging & testing.
         */
        void debug_identified( int id, const QString & nieUrl );

    public Q_SLOTS:
//         void resolveResource( const QUrl& resource, const QUrl& identified );
//         void discardResource( const QUrl& resource );
        
        void notIdentified( int id, const QList<Soprano::Statement> & sts );
        void identified( int id, const QString & oldUri, const QString & newUri );

    private:
        IdentifierModelTree * m_tree;
    };
}

#endif // IDENTIFIERMODEL_H
