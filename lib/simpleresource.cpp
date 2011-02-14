/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#include "simpleresource.h"

#include <QtCore/QHashIterator>

#include <Nepomuk/Variant>

Nepomuk::SimpleResource::SimpleResource()
{

}

Nepomuk::SimpleResource::~SimpleResource()
{

}

QUrl Nepomuk::SimpleResource::uri() const
{
    return m_uri;
}

void Nepomuk::SimpleResource::setUri(const QUrl& uri)
{
    m_uri = uri;
}

namespace {
    Soprano::Node convertIfBlankNode( const Soprano::Node & n ) {
        if( n.isResource() && n.uri().toString().startsWith("_:") ) {
            return Soprano::Node( n.uri().toString().mid(2) ); // "_:" take 2 characters
        }
        return n;
    }
}

QList< Soprano::Statement > Nepomuk::SimpleResource::toStatementList() const
{
    QList<Soprano::Statement> list;
    QHashIterator<QUrl, QVariant> it( m_properties );
    while( it.hasNext() ) {
        it.next();
        
        Soprano::Node object = Nepomuk::Variant( it.value() ).toNode();
        list << Soprano::Statement( convertIfBlankNode( m_uri ),
                                    it.key(),
                                    convertIfBlankNode( object ) );
    }
    return list;
}

bool Nepomuk::SimpleResource::isValid() const
{
    // We donot check if m_uri.isValid() as a blank uri of the form "_:daf" would be invalid
    return !m_uri.isEmpty() && ( !m_properties.isEmpty() );
}

bool Nepomuk::SimpleResource::operator ==(const Nepomuk::SimpleResource &other) const
{
    return m_uri == other.m_uri && m_properties == other.m_properties;
}

uint Nepomuk::qHash(const SimpleResource& res)
{
    return qHash(res.uri());
}
