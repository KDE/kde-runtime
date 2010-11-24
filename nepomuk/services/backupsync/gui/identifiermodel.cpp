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


#include "identifiermodel.h"
#include "identifiermodeltree.h"

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Soprano/Vocabulary/RDF>

#include <Soprano/QueryResultIterator>
#include <Soprano/Model>
#include <Soprano/Statement>

#include <Nepomuk/ResourceManager>

#include <KUrl>
#include <KDebug>
#include <KRandom>

Nepomuk::IdentifierModel::IdentifierModel(QObject* parent): QAbstractItemModel(parent)
{
    m_tree = new IdentifierModelTree();
}


Nepomuk::IdentifierModel::~IdentifierModel()
{
    delete m_tree;
}


QVariant Nepomuk::IdentifierModel::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() )
        return QVariant();
    
    IdentifierModelTreeItem *item = static_cast<IdentifierModelTreeItem *>(index.internalPointer());

    switch(role) {
        case LabelRole:
        case Qt::DisplayRole:
            return  item->toString();
            
        case SizeRole:
            //FIXME: How am I supposed to get the size?
            return KRandom::random();
            
        case TypeRole:
            return item->type();
            
        case ResourceRole:
            return item->resourceUri();
            
        case IdentifiedResourceRole:
            return item->identifiedUri();
            
        case DiscardedRole:
            return item->discarded();
    }
    
    return QVariant();
}


int Nepomuk::IdentifierModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED( parent );
    // It is always 1 for now
    return 1;
}


int Nepomuk::IdentifierModel::rowCount(const QModelIndex& parent) const
{
    if( parent.column() > 0 )
        return 0;
    
    if( !parent.isValid() ) {
        return ( m_tree->isEmpty() ? 0 : m_tree->rootNodes().size() );
    }
    
    IdentifierModelTreeItem * parentItem = static_cast<IdentifierModelTreeItem *>(parent.internalPointer());

    return parentItem->numChildren();
}


QModelIndex Nepomuk::IdentifierModel::parent(const QModelIndex& index) const
{
    if( !index.isValid() )
        return QModelIndex();

    IdentifierModelTreeItem *child = static_cast<IdentifierModelTreeItem*>( index.internalPointer() );
    IdentifierModelTreeItem *parent = static_cast< IdentifierModelTreeItem*>( child->parent() );
    if( parent == 0 )
        return QModelIndex();

    if( parent->parent() == 0 ) {
        // Its parent is one of the root nodes
        int rootPos = m_tree->rootNodes().indexOf( parent );
        if( rootPos == -1 )
            return QModelIndex();

        return createIndex( rootPos, 0, parent );
    }

    return createIndex( parent->parentRowNum(), 0, parent );
}


QModelIndex Nepomuk::IdentifierModel::index(int row, int column, const QModelIndex& parent) const
{
    if( column > 0 )
        return QModelIndex();
    
    if( !parent.isValid() ) {
        // One of the root nodes
        if( m_tree->isEmpty() || row >= m_tree->rootNodes().size() )
            return QModelIndex();
        
        return createIndex( row, column, m_tree->rootNodes().at( row ) );
    }
    
    IdentifierModelTreeItem *parentItem = static_cast<IdentifierModelTreeItem *>( parent.internalPointer() );
    if( row >= parentItem->numChildren() )
        return QModelIndex();
    
    return createIndex(row, column, parentItem->child( row ) );
}


void Nepomuk::IdentifierModel::identified(int id, const QString& oldUri, const QString& newUri)
{
    Q_UNUSED( id );
    //kDebug() << oldUri << " -----> " << newUri;
    
    emit layoutAboutToBeChanged();
 
    IdentifierModelTreeItem* item = m_tree->findByUri( QUrl(oldUri) );
    if( item ) {
        kDebug() << item->url() << " -----> " << newUri;
        item->setIdentified( QUrl(newUri) );
    }
    
    emit layoutChanged();
}


void Nepomuk::IdentifierModel::notIdentified(int id, const QList< Soprano::Statement >& sts)
{
    if( sts.isEmpty() )
        return;
    
    //kDebug();
    emit layoutAboutToBeChanged();

    // If already exists - Remove it
    QUrl resUri = sts.first().subject().uri();
    IdentifierModelTreeItem* it = m_tree->findByUri( resUri );
    if( it ) {
        kDebug() << "ALREADY EXISTS!! REMOVING!!";
        m_tree->remove( it );
    }

    // Insert into the tree
    IdentifierModelTreeItem* item = IdentifierModelTreeItem::fromStatementList( sts );
    item->setUnidentified();
    m_tree->add( item );

    emit layoutChanged();
}

Qt::ItemFlags Nepomuk::IdentifierModel::flags(const QModelIndex& index) const
{
    Q_UNUSED( index );
    //return QAbstractItemModel::flags(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

#include "identifiermodel.moc"
