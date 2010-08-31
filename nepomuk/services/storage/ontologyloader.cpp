/* This file is part of the KDE Project
   Copyright (c) 2007-2010 Sebastian Trueg <trueg@kde.org>

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
#include "kuvo.h"

#include <Soprano/Global>
#include <Soprano/Node>
#include <Soprano/LiteralValue>
#include <Soprano/Model>
#include <Soprano/PluginManager>
#include <Soprano/StatementIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Parser>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDFS>

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



Nepomuk::OntologyLoader::OntologyLoader( Soprano::Model* model, QObject* parent )
    : QObject( parent ),
      d( new Private(this) )
{
    // register ontology resource dir
    KGlobal::dirs()->addResourceType( "xdgdata-ontology", 0, "ontology" );

    // export ourselves on DBus
    ( void )new OntologyManagerAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String("/nepomukontologyloader"),
                                                  this,
                                                  QDBusConnection::ExportAdaptors );

    // be backwards compatible
    QDBusConnection::sessionBus().registerService( QLatin1String("org.kde.nepomuk.services.nepomukontologyloader") );

    d->model = new OntologyManagerModel( model, this );
    connect( &d->updateTimer, SIGNAL(timeout()), this, SLOT(updateNextOntology()) );

    // watch both the global and local ontology folder for changes
    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty(QString) ),
             this, SLOT( updateLocalOntologies() ) );
    connect( dirWatch, SIGNAL( created(QString) ),
             this, SLOT( updateLocalOntologies() ) );

    foreach( const QString& dir, KGlobal::dirs()->resourceDirs( "xdgdata-ontology" ) ) {
        kDebug() << "watching" << dir;
        dirWatch->addDir( dir, KDirWatch::WatchFiles|KDirWatch::WatchSubDirs );
    }
}


Nepomuk::OntologyLoader::~OntologyLoader()
{
    delete d;
}


void Nepomuk::OntologyLoader::updateLocalOntologies()
{
    d->desktopFilesToUpdate = KGlobal::dirs()->findAllResources( "xdgdata-ontology", "*.ontology", KStandardDirs::Recursive|KStandardDirs::NoDuplicates );
    if(d->desktopFilesToUpdate.isEmpty())
        kError() << "No ontology files found! Make sure the shared-desktop-ontologies project is installed and XDG_DATA_DIRS is set properly.";
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
        updateUserVisibility();
        emit ontologyLoadingFinished(this);
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


namespace {
    struct UserVisibleNode {
        UserVisibleNode( const QUrl& r )
            : uri( r ),
              userVisible( 0 ) {
        }

        QUrl uri;
        int userVisible;
        QHash<QUrl, UserVisibleNode*> parents;

        /**
         * Set the value of nao:userVisible and add the corresponding
         * statement to the list of statements in case the value was
         * not defined yet.
         */
        int updateUserVisibility( QList<Soprano::Statement>& statements ) {
            if ( userVisible != 0 ) {
                kDebug() << "User visibility already set" << uri.toString() << ( userVisible == 1 );
                return userVisible;
            }
            else {
                if ( !parents.isEmpty() ) {
                    for ( QHash<QUrl, UserVisibleNode*>::iterator it = parents.begin();
                          it != parents.end(); ++it ) {
                        kDebug() << uri.toString() << "Checking parent class" << it.value()->uri.toString();
                        if ( it.value()->updateUserVisibility( statements ) == -1 ) {
                            userVisible = -1;
                            break;
                        }
                    }
                }
                if ( userVisible == 0 ) {
                    // default to visible
                    userVisible = 1;
                }
                statements << Soprano::Statement( uri,
                                                  Soprano::Vocabulary::NAO::userVisible(),
                                                  Soprano::LiteralValue( userVisible == 1 ),
                                                  Nepomuk::Vocabulary::KUVO::kuvoNamespace() );
                kDebug() << "Setting nao:userVisible of" << uri.toString() << ( userVisible == 1 );
                return userVisible;
            }
        }
    };
}

void Nepomuk::OntologyLoader::updateUserVisibility()
{
    //
    // build a tree of all classes and properties
    //
    QString query
        = QString::fromLatin1( "select distinct ?r ?p ?v where { "
                               "{ ?r a rdfs:Class . "
                               "OPTIONAL { ?r rdfs:subClassOf ?p . ?p a rdfs:Class . } . } "
                               "UNION "
                               "{ ?r a rdf:Property . "
                               "OPTIONAL { ?r rdfs:subPropertyOf ?p . ?p a rdf:Property . } . } "
                               "OPTIONAL { ?r %1 ?v . } . "
                               "FILTER(?r!=rdfs:Resource) . "
                               "}" )
        .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::NAO::userVisible() ) );
    kDebug() << query;
    QHash<QUrl, UserVisibleNode*> all;
    {
        Soprano::QueryResultIterator it
            = d->model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        while( it.next() ) {
            const QUrl r = it["r"].uri();
            const Soprano::Node p = it["p"];
            const Soprano::Node v = it["v"];

            if ( !all.contains( r ) ) {
                UserVisibleNode* r_uvn = new UserVisibleNode( r );
                all.insert( r, r_uvn );
            }
            if( v.isLiteral() )
                all[r]->userVisible = (v.literal().toBool() ? 1 : -1);
            if ( p.isResource() &&
                 p.uri() != r &&
                 p.uri() != Soprano::Vocabulary::RDFS::Resource() ) {
                if ( !all.contains( p.uri() ) ) {
                    UserVisibleNode* p_uvn = new UserVisibleNode( p.uri() );
                    all.insert( p.uri(), p_uvn );
                    all[r]->parents.insert( p.uri(), p_uvn );
                }
                else {
                    all[r]->parents.insert( p.uri(), all[p.uri()] );
                }
            }
        }
    }

    //
    // create a list of all statements we need to add
    //
    QList<Soprano::Statement> statements;
    for ( QHash<QUrl, UserVisibleNode*>::iterator it = all.begin();
          it != all.end(); ++it ) {
        it.value()->updateUserVisibility( statements );
    }

    //
    // cleanup and add all the necessary statements to the main model
    //
    qDeleteAll( all );
    d->model->addStatements( statements );
}

#include "ontologyloader.moc"
