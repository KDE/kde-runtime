/*
   $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $

   This file is part of the Strigi project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "util.h"
#include "variant.h"

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>
#include <QtCore/QDebug>

#include <Soprano/Index/CLuceneIndex>
#include <Soprano/Model>


#define STRIGI_NS "http://www.strigi.org/data#"

QUrl Strigi::Soprano::Util::fieldUri( const std::string& s )
{
    QString qKey = QString::fromUtf8( s.c_str() );
    QUrl url;

    // very naive test for proper URI
    if ( qKey.contains( ":/" ) ) {
        url = qKey;
    }
    else {
        url = STRIGI_NS + qKey;
    }

    // just to be sure
    if ( url.isRelative() ) {
        url.setScheme( "http" );
    }

    return url;
}


QUrl Strigi::Soprano::Util::fileUrl( const std::string& filename )
{
    QUrl url = QUrl::fromLocalFile( QFileInfo( QString::fromUtf8( filename.c_str() ) ).absoluteFilePath() );
    url.setScheme( "file" );
    return url;
}


std::string Strigi::Soprano::Util::fieldName( const QUrl& uri )
{
    QString s = uri.toString();
    if ( s.startsWith( STRIGI_NS ) ) {
        s = s.mid( strlen( STRIGI_NS ) );
    }
    return s.toUtf8().data();
}


TString Strigi::Soprano::Util::convertSearchField( const std::string& field )
{
    if ( QString::fromUtf8( field.c_str() ) == ::Soprano::Index::CLuceneIndex::defaultSearchField() ) {
        return TString::fromUtf8( field.c_str() );
    }
    else {
        return fieldUri( field ).toString();
    }
}


QUrl Strigi::Soprano::Util::uniqueUri( const QString& ns, ::Soprano::Model* model )
{
    QUrl uri;
    do {
        QString uid = QUuid::createUuid().toString();
        uri = ( ns + uid.mid( 1, uid.length()-2 ) );
    } while ( model->containsAnyStatement( ::Soprano::Statement( uri, ::Soprano::Node(), ::Soprano::Node() ) ) );
    return uri;
}


Strigi::Variant Strigi::Soprano::Util::nodeToVariant( const ::Soprano::Node& node )
{
    if ( node.isLiteral() ) {
        switch( node.literal().type() ) {
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:  // FIXME: no perfect conversion :(
        case QVariant::ULongLong:
            return Strigi::Variant( node.literal().toInt() );

        case QVariant::Bool:
            return Strigi::Variant( node.literal().toBool() );

        default:
            return Strigi::Variant( node.literal().toString().toUtf8().data() );
        }
    }
    else {
        qWarning() << "(Soprano::Util::nodeToVariant) cannot convert non-literal node to variant.";
        return Strigi::Variant();
    }
}
