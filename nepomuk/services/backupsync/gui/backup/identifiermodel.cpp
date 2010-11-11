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
    IdentifierModelTreeItem *item = static_cast<IdentifierModelTreeItem *> (m_tree->root());
    if( index.isValid() )
        item = static_cast<IdentifierModelTreeItem *>(index.internalPointer());
    
    switch(role) {
        case LabelRole:
        case Qt::DisplayRole:
            //FIXME: Make this look good
            return QString::fromLatin1("Text for item %1").arg( item->prettyString() );
            
        case SizeRole:
            //FIXME: How am I supposed to get the size?
            return KRandom::random();
            
        case TypeRole:
            return item->url();
            
        case ResourceRole:
            return QUrl( item->resourceUri() );
            
        case IdentifiedResourceRole:
            //FIXME!
            return QUrl( item->resourceUri() );
            
        case DiscardedRole:
            return true;
            //FIXME!
            //return m_discardedResource.contains(data(index,ResourceRole).toUrl());
    }
    
    return item->url();
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
        //kDebug() << "NOT VALID - Probably root";
        int r = m_tree->root()->isEmpty() ? 0 : 1;
        //kDebug() << r;
        return r;
    }
    
    IdentifierModelTreeItem * parentItem;

    //kDebug() << "Else cluase !!";
    parentItem = static_cast<IdentifierModelTreeItem *>(parent.internalPointer());
    //if( parentItem == m_tree->root() )
    //    kDebug() << "ROOT!! :-D";
    
    int r = parentItem->numChildren();
    //kDebug() << r;
    return r;
}


QModelIndex Nepomuk::IdentifierModel::parent(const QModelIndex& index) const
{
    if( !index.isValid() )
        return QModelIndex();

    IdentifierModelTreeItem *childItem = static_cast<IdentifierModelTreeItem*>( index.internalPointer() );
    if( childItem == m_tree->root() )
        return QModelIndex();

    FileSystemTreeItem<IdentificationData> *parentItem = static_cast<FileSystemTreeItem<IdentificationData>*>( childItem->parent() );
    if( !parentItem )
        return QModelIndex();

    if( parentItem == m_tree->root() )
        return createIndex( 0, 0, m_tree->root() );
    
    return createIndex( parentItem->parentRowNum(), 0, parentItem );
}


QModelIndex Nepomuk::IdentifierModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid()) {
//         IdentifierModelTreeItem * root = dynamic_cast<IdentifierModelTreeItem*>( );

        if( m_tree->root()->isEmpty() )
            return QModelIndex();

        return createIndex( row, column, m_tree->root() );
    }
    else {
        IdentifierModelTreeItem *parentItem = static_cast<IdentifierModelTreeItem *>( parent.internalPointer() );
        FileSystemTreeItem<IdentificationData> *childItem = static_cast<FileSystemTreeItem<IdentificationData>*>( parentItem->child(row) );
        return createIndex(row, column, childItem);
    }
}


void Nepomuk::IdentifierModel::identified(int id, const QString& oldUri, const QString& newUri)
{
    Q_UNUSED( id );
    kDebug() << oldUri << " -----> " << newUri;
    
    emit layoutAboutToBeChanged();
 
    m_tree->remove( oldUri );
    
    emit layoutChanged();
}


void Nepomuk::IdentifierModel::notIdentified(int id, const QList< Soprano::Statement >& sts)
{
    kDebug();
    emit layoutAboutToBeChanged();

    IdentifierModelTreeItem* item = IdentifierModelTreeItem::fromStatementList(id, sts);
    item->setUnidentified();;
    m_tree->add( item );

    emit layoutChanged();
}


void Nepomuk::IdentifierModel::debug_identified(int id, const QString& nieUrl)
{/*
    Q_UNUSED( id )
    emit layoutAboutToBeChanged();
    m_tree->remove( nieUrl );
    emit layoutChanged();*/
}


void Nepomuk::IdentifierModel::debug_notIdentified(int id, const QString& resUri, const QString& nieUrl, bool folder)
{
    
    emit layoutAboutToBeChanged();

    Soprano::Statement st( Soprano::Node( QUrl(resUri) ),
                           Soprano::Node( Nepomuk::Vocabulary::NIE::url() ),
                           Soprano::Node( QUrl(nieUrl) ) );
    QList<Soprano::Statement> stList;
    stList.append( st );
    if( folder ) {
        stList << Soprano::Statement( QUrl(resUri), Soprano::Vocabulary::RDF::type(), Nepomuk::Vocabulary::NFO::Folder() );
    }
    
    //kDebug() << m_tree->toList();
    m_tree->add( IdentifierModelTreeItem::fromStatementList( id, stList ) );
    kDebug() << m_tree->toList();
    emit layoutChanged();
}

Qt::ItemFlags Nepomuk::IdentifierModel::flags(const QModelIndex& index) const
{
    Q_UNUSED( index );
    //return QAbstractItemModel::flags(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

#include "identifiermodel.moc"
