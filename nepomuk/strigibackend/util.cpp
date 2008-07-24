/*
   Copyright (C) 2007-2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

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

#include <strigi/variant.h>
#include <strigi/fieldtypes.h>

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>
#include <QtCore/QDebug>

#include <Soprano/Index/CLuceneIndex>
#include <Soprano/Model>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/XMLSchema>


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
    else if ( QString( field.c_str() ) == ::Soprano::Vocabulary::RDF::type().toString() ) {
        // see sopranoindexwriter:addValue for details on this conversion
        static TString strigiType( "http://strigi.sourceforge.net/fields#rdf-string-type" );
        return strigiType;
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


void Strigi::Soprano::Util::storeStrigiMiniOntology( ::Soprano::Model* model )
{
    // we use some nice URI here although we still have the STRIGI_NS for backwards comp
    // at some point (if parentUrl will not be moved to xesam) we should move to a proper onto

    QUrl graph( "http://nepomuk.kde.org/ontologies/2008/07/24/strigi/metadata" );
    ::Soprano::Statement parentUrlProp( fieldUri( FieldRegister::parentLocationFieldName ),
                                        ::Soprano::Vocabulary::RDF::type(),
                                        ::Soprano::Vocabulary::RDF::Property(),
                                        graph );
    ::Soprano::Statement parentUrlRange( parentUrlProp.subject(),
                                         ::Soprano::Vocabulary::RDFS::range(),
                                         ::Soprano::Vocabulary::XMLSchema::string(),
                                         graph );
    ::Soprano::Statement parentUrlDomain( parentUrlProp.subject(),
                                          ::Soprano::Vocabulary::RDFS::domain(),
                                          ::Soprano::Vocabulary::Xesam::File(),
                                          graph );
    ::Soprano::Statement metaDataType( graph,
                                       ::Soprano::Vocabulary::RDF::type(),
                                       ::Soprano::Vocabulary::NRL::Ontology(),
                                       graph );

    if ( !model->containsStatement( parentUrlProp ) ) {
        model->addStatement( parentUrlProp );
    }
    if ( !model->containsStatement( parentUrlRange ) ) {
        model->addStatement( parentUrlRange );
    }
    if ( !model->containsStatement( parentUrlDomain ) ) {
        model->addStatement( parentUrlDomain );
    }
    if ( !model->containsStatement( metaDataType ) ) {
        model->addStatement( metaDataType );
    }
}
