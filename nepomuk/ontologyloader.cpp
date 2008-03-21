/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "ontologyloader.h"
#include "ontologymanagermodel.h"

#include <Soprano/Global>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/PluginManager>
#include <Soprano/StatementIterator>
#include <Soprano/Parser>

#include <KDesktopFile>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

#include <QtCore/QFileInfo>


using namespace Soprano;


class Nepomuk::OntologyLoader::Private
{
public:
    Private( OntologyLoader* p )
        : q( p ) {
    }

    OntologyManagerModel* model;

    void updateOntology( const QString& filename );

private:
    OntologyLoader* q;
};


Nepomuk::OntologyLoader::OntologyLoader( Soprano::Model* model, QObject* parent )
    : QObject( parent ),
      d( new Private(this) )
{
    d->model = new OntologyManagerModel( model, this );
}


Nepomuk::OntologyLoader::~OntologyLoader()
{
    delete d;
}


void Nepomuk::OntologyLoader::update()
{
    if ( !d->model ) {
        kDebug() << "No Nepomuk Model. Cannot update ontologies.";
        return;
    }

    // update all installed ontologies
    QStringList ontoFiles = KGlobal::dirs()->findAllResources( "data", "nepomuk/ontologies/*.desktop" );
    foreach( QString file, ontoFiles ) {
        d->updateOntology( file );
    }

    // FIXME: install watches for changed or newly installed ontologies
//    QStringList dirs = KGlobal::dirs()->findDirs( "data", "nepomuk/ontologies" );
}


void Nepomuk::OntologyLoader::Private::updateOntology( const QString& filename )
{
    KDesktopFile df( filename );

    // only update if the modification date of the ontology file changed (not the desktop file).
    // ------------------------------------
    QFileInfo ontoFileInf( df.readPath() );
    QDateTime ontoLastModified = model->ontoModificationDate( df.readUrl() );
    if ( ontoLastModified < ontoFileInf.lastModified() ) {

        kDebug() << "Ontology" << df.readUrl() << "needs updating.";

        QString mimeType = df.desktopGroup().readEntry( "MimeType", QString() );

        const Soprano::Parser* parser
            = Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::mimeTypeToSerialization( mimeType ),
                                                                                  mimeType );
        if ( !parser ) {
            kDebug() << "No parser to handle" << df.readName() << "(" << mimeType << ")";
            return;
        }

        kDebug() << "Parsing" << df.readPath();

        model->updateOntology( parser->parseFile( df.readPath(),
                                                  df.readUrl(),
                                                  Soprano::mimeTypeToSerialization( mimeType ),
                                                  mimeType ),
                                  df.readUrl() );
    }
    else {
        kDebug() << "Ontology" << df.readUrl() << "up to date.";
    }
}

#include "ontologyloader.moc"
