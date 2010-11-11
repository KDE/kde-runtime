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


#include "identifiermodeltree.h"

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Soprano/Vocabulary/RDF>

#include <Soprano/Statement>

Nepomuk::IdentifierModelTreeItem::IdentifierModelTreeItem(const QUrl& url, bool isFolder, const Nepomuk::IdentificationData& data)
    : FileSystemTreeItem<Nepomuk::IdentificationData>( url, isFolder, data )
{
}

Nepomuk::IdentifierModelTreeItem* Nepomuk::IdentifierModelTreeItem::fromData(const Nepomuk::IdentificationData& data)
{
    const QUrl nieUrl = Nepomuk::Vocabulary::NIE::url();
    const QUrl rdfType = Soprano::Vocabulary::RDF::type();
    const QUrl nfoFolder = Nepomuk::Vocabulary::NFO::Folder();
    
    bool isFolder = false;
    QUrl url;
    foreach( const Soprano::Statement & st, data.statements ) {
        if( st.predicate().uri() == nieUrl ) {
            url = st.object().uri();
            //if( m_nieUrl.startsWith("file://") )
            //    m_nieUrl = m_nieUrl.mid( 7 );
        }
        else if( st.predicate().uri() == rdfType ) {
            if( st.object().uri() == nfoFolder )
                isFolder = true;
        }
    }

    return new IdentifierModelTreeItem( url, isFolder, data );
}

QUrl Nepomuk::IdentifierModelTreeItem::type() const
{
    //TODO: Implement me!
    return Nepomuk::Vocabulary::NFO::Audio();
}


Nepomuk::IdentifierModelTreeItem* Nepomuk::IdentifierModelTreeItem::fromStatementList(int id, const QList< Soprano::Statement >& stList)
{
    IdentificationData data;
    data.id = id;
    data.statements = stList;

    return fromData( data );
}

QUrl Nepomuk::IdentifierModelTreeItem::resourceUri() const
{
    if( data().statements.isEmpty() )
        return QUrl();
    
    return data().statements.first().subject().uri();
}

int Nepomuk::IdentifierModelTreeItem::id() const
{
    return data().id;
}


bool Nepomuk::IdentifierModelTreeItem::identified() const
{
    return data().identified;
}

void Nepomuk::IdentifierModelTreeItem::setIdentified(const QUrl& newUri)
{
    data().identifiedUrl = newUri;
    data().identified = true;
}

void Nepomuk::IdentifierModelTreeItem::setUnidentified()
{
    data().identified = false;
}


//
// IdentifierModelTree
//

Nepomuk::IdentifierModelTree::IdentifierModelTree()
    : FileSystemTree<IdentificationData>()
{
}

void Nepomuk::IdentifierModelTree::add(FileSystemTreeItem< Nepomuk::IdentificationData >* item)
{
    QString resUri;
    if( !item->data().statements.isEmpty() )
        resUri = item->data().statements.first().subject().toString();
    
    m_resUrlHash.insert( resUri, item->url() );
    
    FileSystemTree< Nepomuk::IdentificationData >::add(item);
}


void Nepomuk::IdentifierModelTree::remove(const QString& resUri)
{

    QHash< QString, QUrl >::const_iterator it = m_resUrlHash.constFind( resUri );
    if( it != m_resUrlHash.constEnd() ) {
        FileSystemTree<Nepomuk::IdentificationData>::remove( it.value().toString() );
    }
}


