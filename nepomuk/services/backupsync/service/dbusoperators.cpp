/*
 * This file is part of Soprano Project.
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "dbusoperators.h"


QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::Node& node )
{
    arg.beginStructure();
    arg << ( int )node.type();
    if ( node.type() == Soprano::Node::ResourceNode ) {
        arg << QString::fromAscii( node.uri().toEncoded() );
    }
    else {
        arg << node.toString();
    }
    arg << node.language() << QString::fromAscii( node.dataType().toEncoded() );
    arg.endStructure();
    return arg;
}


const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::Node& node )
{
    arg.beginStructure();
    int type;
    QString value, language, dataTypeUri;
    arg >> type >> value >> language >> dataTypeUri;
    if ( type == Soprano::Node::LiteralNode ) {
        if ( dataTypeUri.isEmpty() )
            node = Soprano::Node( Soprano::LiteralValue::createPlainLiteral( value, language ) );
        else
            node = Soprano::Node( Soprano::LiteralValue::fromString( value, QUrl::fromEncoded( dataTypeUri.toAscii() ) ) );
    }
    else if ( type == Soprano::Node::ResourceNode ) {
        node = Soprano::Node( QUrl::fromEncoded( value.toAscii() ) );
    }
    else if ( type == Soprano::Node::BlankNode ) {
        node = Soprano::Node( value );
    }
    else {
        node = Soprano::Node();
    }
    arg.endStructure();
    return arg;
}


QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::Statement& statement )
{
    arg.beginStructure();
    arg << statement.subject() << statement.predicate() << statement.object() << statement.context();
    arg.endStructure();
    return arg;
}


const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::Statement& statement )
{
    arg.beginStructure();
    Soprano::Node subject, predicate, object, context;
    arg >> subject >> predicate >> object >> context;
    statement = Soprano::Statement( subject, predicate, object, context );
    arg.endStructure();
    return arg;
}


QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::BindingSet& set )
{
    arg.beginStructure();
    arg.beginMap( QVariant::String, qMetaTypeId<Soprano::Node>() );
    QStringList names = set.bindingNames();
    for ( int i = 0; i < names.count(); ++i ) {
        arg.beginMapEntry();
        arg << names[i] << set[ names[i] ];
        arg.endMapEntry();
    }
    arg.endMap();
    arg.endStructure();
    return arg;
}


const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::BindingSet& set )
{
    arg.beginStructure();
    arg.beginMap();
    while ( !arg.atEnd() ) {
        QString name;
        Soprano::Node val;
        arg.beginMapEntry();
        arg >> name >> val;
        arg.endMapEntry();
        set.insert( name, val );
    }

    arg.endMap();
    arg.endStructure();
    return arg;
}

void registerMetaTypes()
{
    qRegisterMetaType<Soprano::Statement>("Soprano::Statement");
    qRegisterMetaType<QList<Soprano::Statement> >("QList<Soprano::Statement>");
}
