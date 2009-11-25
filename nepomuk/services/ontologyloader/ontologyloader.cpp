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
#include "ontologymanageradaptor.h"
#include "graphretriever.h"

#include <Soprano/Global>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/PluginManager>
#include <Soprano/StatementIterator>
#include <Soprano/Parser>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>
#include <KLocale>
#include <KDirWatch>

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::OntologyLoader, "nepomukontologyloader")


using namespace Soprano;

class Nepomuk::OntologyLoader::Private
{
public:
    Private( OntologyLoader* p )
        : forceOntologyUpdate( false ),
          q( p ) {
    }

    OntologyManagerModel* model;

    QTimer updateTimer;
    bool forceOntologyUpdate;
    QStringList desktopFilesToUpdate;

    void updateOntology( const QString& filename );

private:
    OntologyLoader* q;
};


void Nepomuk::OntologyLoader::Private::updateOntology( const QString& filename )
{
    KConfig ontologyDescFile( filename );
    KConfigGroup df( &ontologyDescFile, QLatin1String( "Ontology" ) );

    // only update if the modification date of the ontology file changed (not the desktop file).
    // ------------------------------------
    QFileInfo ontoFileInf( df.readEntry( QLatin1String("Path") ) );
    QString ontoNamespace = df.readEntry( QLatin1String("Namespace") );
    QDateTime ontoLastModified = model->ontoModificationDate( ontoNamespace );
    bool update = false;

    if ( ontoLastModified < ontoFileInf.lastModified() ) {
        kDebug() << "Ontology" << ontoNamespace << "needs updating.";
        update = true;
    }
    else {
        kDebug() << "Ontology" << ontoNamespace << "up to date.";
    }

    if( !update && forceOntologyUpdate ) {
        kDebug() << "Ontology update forced.";
        update = true;
    }

    if( update ) {
        QString mimeType = df.readEntry( "MimeType", QString() );

        const Soprano::Parser* parser
            = Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::mimeTypeToSerialization( mimeType ),
                                                                                  mimeType );
        if ( !parser ) {
            kDebug() << "No parser to handle" << df.readEntry( QLatin1String( "Name" ) ) << "(" << mimeType << ")";
            return;
        }

        kDebug() << "Parsing" << ontoFileInf.filePath();

        Soprano::StatementIterator it = parser->parseFile( ontoFileInf.filePath(),
                                                           ontoNamespace,
                                                           Soprano::mimeTypeToSerialization( mimeType ),
                                                           mimeType );
        if ( !parser->lastError() ) {
            model->updateOntology( it, ontoNamespace );
            emit q->ontologyUpdated( ontoNamespace );
        }
        else {
            emit q->ontologyUpdateFailed( ontoNamespace, i18n( "Parsing of file %1 failed (%2)", ontoFileInf.filePath(), parser->lastError().message() ) );
        }
    }
}



Nepomuk::OntologyLoader::OntologyLoader( QObject* parent, const QList<QVariant>& )
    : Service( parent ),
      d( new Private(this) )
{
    ( void )new OntologyManagerAdaptor( this );

    d->model = new OntologyManagerModel( mainModel(), this );
    connect( &d->updateTimer, SIGNAL(timeout()), this, SLOT(updateNextOntology()) );

    // only update changed ontologies
    updateLocalOntologies();

    // watch both the global and local ontology folder for changes
    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty(QString) ),
             this, SLOT( updateLocalOntologies() ) );
    connect( dirWatch, SIGNAL( created(QString) ),
             this, SLOT( updateLocalOntologies() ) );
    foreach( const QString& dir, KGlobal::dirs()->findDirs( "data", QLatin1String("../") ) ) {
        // we only add the suffix here to make sure to also watch the non-existing local dir
        kDebug() << "watching" << ( dir + "ontology/" );
        dirWatch->addDir( dir + "ontology/", KDirWatch::WatchFiles|KDirWatch::WatchSubDirs );
    }
}


Nepomuk::OntologyLoader::~OntologyLoader()
{
    delete d;
}


void Nepomuk::OntologyLoader::updateLocalOntologies()
{
    d->desktopFilesToUpdate = KGlobal::dirs()->findAllResources( "data", "../ontology/*.ontology", KStandardDirs::Recursive|KStandardDirs::NoDuplicates );
    d->updateTimer.start(0);
}


void Nepomuk::OntologyLoader::updateAllLocalOntologies()
{
    d->forceOntologyUpdate = true;
    updateLocalOntologies();
}


void Nepomuk::OntologyLoader::updateNextOntology()
{
    if( !d->desktopFilesToUpdate.isEmpty() ) {
        d->updateOntology( d->desktopFilesToUpdate.takeFirst() );
    }
    else {
        d->forceOntologyUpdate = false;
        d->updateTimer.stop();
    }
}


QString Nepomuk::OntologyLoader::findOntologyContext( const QString& uri )
{
    return QString::fromAscii( d->model->findOntologyContext( QUrl::fromEncoded( uri.toAscii() ) ).toEncoded() );
}


void Nepomuk::OntologyLoader::importOntology( const QString& url )
{
    connect( GraphRetriever::retrieve( url ), SIGNAL( result( KJob* ) ),
             this, SLOT( slotGraphRetrieverResult( KJob* ) ) );
}


void Nepomuk::OntologyLoader::slotGraphRetrieverResult( KJob* job )
{
    GraphRetriever* graphRetriever = static_cast<GraphRetriever*>( job );
    if ( job->error() ) {
        emit ontologyUpdateFailed( QString::fromAscii( graphRetriever->url().toEncoded() ), graphRetriever->errorString() );
    }
    else {
        // TODO: find a way to check if the imported version of the ontology
        // is newer than the already installed one
        if ( d->model->updateOntology( graphRetriever->statements(), QUrl()/*graphRetriever->url()*/ ) ) {
            emit ontologyUpdated( QString::fromAscii( graphRetriever->url().toEncoded() ) );
        }
        else {
            emit ontologyUpdateFailed( QString::fromAscii( graphRetriever->url().toEncoded() ), d->model->lastError().message() );
        }
    }
}

#include "ontologyloader.moc"
