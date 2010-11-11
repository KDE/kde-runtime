/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

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


#include "common.h"
#include <QDir>
#include <QtCore/qglobal.h>
#include <QtCore/QTime>
#include <cstdlib>
#include <QSet>
#include <KDebug>

#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NRL>

Soprano::Model* Nepomuk::createNewModel(const QString& dir)
{
    kDebug() << dir;
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName("virtuoso");
    Soprano::Model * model = (Soprano::Model*)backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, dir ) );
    return model;
}

void Nepomuk::setAsDefaultModel(Soprano::Model* model)
{
    Nepomuk::ResourceManager::instance()->setOverrideMainModel( model );
}



Nepomuk::FileGenerator::FileGenerator()
{
    QTime midnight(0, 0, 0);
    qsrand( midnight.secsTo(QTime::currentTime()) );

    m_FileProb = 0.50;
    m_DirProb = 0.30;
    m_CdupProb = 0.15;

    m_baseFileName = "file";
    m_baseDirName = "dir";
}

//TODO: Optimize
void Nepomuk::FileGenerator::create( const QString& dirString, int num )
{
    QDir dir( dirString );

    int fileCount = 0;
    int dirCount = 0;

    while( fileCount < num ) {

        QUrl url( m_baseFileName + QString::number( fileCount ) );
        createFile( url );

        fileCount++;
        //float p = static_cast<float>(qrand()) / RAND_MAX;
        /*
        if( p <= m_FileProb )
            createFile();
        else if( p <= m_DirProb )
            createDir();
        else if( p <= m_CdupProb )
            dir.cdUp();*/
    }
}

void Nepomuk::FileGenerator::createFile(const QUrl& url)
{
    QFile file( url.toLocalFile() );
    file.open( QIODevice::Text | QIODevice::ReadWrite | QIODevice::Truncate );

    QTextStream ts( &file );
    ts << '.';
}


void Nepomuk::FileGenerator::createDir(const QUrl& dir)
{
    QDir d;
    d.mkpath( dir.toLocalFile() );
}

QList< QUrl > Nepomuk::FileGenerator::urls()
{
    return m_urls;
}


//
// Metadata generator
//


Nepomuk::IndexMetadataGenerator::IndexMetadataGenerator()
{
    m_artists << "Coldplay" << "The Killers" << "King Kong"
              << "Sonic" << "Outlandish" << "Snow Patrol";

    m_albums << "X&Y" << "Sounds of a Rebel" << "Album 3" << "Album 4";
}


namespace {

    QUrl parent( const QUrl & url ) {
        QString str = url.toString();
        int index = str.lastIndexOf('/');
        return QUrl( str.mid( 0, index ) );
    }
}
void Nepomuk::IndexMetadataGenerator::createImageImage(const QUrl& url)
{
    QList<Soprano::Statement> stList;
    QUrl resUri = ResourceManager::instance()->generateUniqueUri( QString() );

    QDateTime contentLastModified = QDateTime::fromTime_t(1272335552L);
    long contentSize = 916334L;
    QUrl isPartOf = parent( url );
    QDateTime nieLastModified = QDateTime::fromTime_t(1272375155L);
    QString mimeType("image/png");
    //QString fileName = getFileName( url );
    int height = 50;
    int width = 50;
    int depth = 32;

    //stList << Soprano::Statement( resUri, //QUrl("<http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentLastModified>", Soprano::Node(

    /*
    <t.png>
    <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentLastModified>
    "1272335552";
    <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentSize>
    "916334";
    <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#isPartOf>
    "";
    <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#lastModified>
    "1272375155";
    <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#mimeType>
    "image/png",
    "image/png",
    "image/png";
    <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#url>
    "t.png";
    <http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#fileName>
    "t.png";
    <http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#height>
    "2048";
    <http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#interlaceMode>
    "None";
    <http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#width>
    "1536";
    <http://www.semanticdesktop.org/ontologies/nfo#colorDepth>
    "32";
    <http://www.w3.org/1999/02/22-rdf-syntax-ns#type>
    "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#RasterImage",
    "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#FileDataObject".*/
}


void Nepomuk::IndexMetadataGenerator::createMusicFile(const QUrl& url)
{

}


void Nepomuk::IndexMetadataGenerator::createTextFile(const QUrl& url)
{

}


void Nepomuk::IndexMetadataGenerator::createVideoFile(const QUrl& url)
{

}


const Nepomuk::ResourceHash& Nepomuk::IndexMetadataGenerator::resourceHash() const
{
    return m_resourceHash;
}

//
// Metadata Generator
//

Nepomuk::MetadataGenerator::MetadataGenerator(Soprano::Model* model)
{
    m_manager = ResourceManager::createManagerForModel( model );
    m_model = model;
}

void Nepomuk::MetadataGenerator::rate(const QUrl& url, int rating)
{
    rating = rating == -1 ? qrand() % 11 : rating;

    Resource res( url, QUrl(), m_manager );
    res.setRating( rating );
}


//
// Model Comparison
//


QList<Soprano::Statement> Nepomuk::getBackupStatements( Soprano::Model * model )
{
    QString query = QString::fromLatin1("select ?r ?p ?o ?g where { graph ?g { ?r ?p ?o. } ?g a %1 . FILTER(!bif:exists( ( select (1) where { ?g a  %2 . } ) )) . }").arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::NRL::InstanceBase()), Soprano::Node::resourceToN3( Soprano::Vocabulary::NRL::DiscardableInstanceBase()) );

    Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparql );

    QList<Soprano::Statement> statements;
    while( it.next() ) {
        Soprano::Statement st( it["r"], it["p"], it["o"], it["g"] );
        statements.append( st );
    }

    return statements;
}

