/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
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

#include "strigiconfigfile.h"
#include "nepomukstrigi-config.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <KDebug>
#include <KStandardDirs>


namespace {
    bool convertBooleanXsdValue( const QString& value ) {
        return(  value.toLower() == QLatin1String( "true" ) ||
                 value.toLower() == QLatin1String( "1" ) );
    }
}


Nepomuk::StrigiConfigFile::StrigiConfigFile()
{
    reset();
}


Nepomuk::StrigiConfigFile::StrigiConfigFile( const QString& filename )
{
    reset();
    setFilename( filename );
}


Nepomuk::StrigiConfigFile::~StrigiConfigFile()
{
}


void Nepomuk::StrigiConfigFile::setFilename( const QString& filename )
{
    m_filename = filename;
}


void Nepomuk::StrigiConfigFile::reset()
{
    m_useDBus = true;
    m_repositories.clear();
    m_excludeFilters.clear();
    m_includeFilters.clear();
}


bool Nepomuk::StrigiConfigFile::load()
{
    kDebug() << m_filename;

    QFile file( m_filename );
    if ( file.open( QIODevice::ReadOnly ) ) {
        QDomDocument doc;
        if ( doc.setContent( &file ) ) {
            return readConfig( doc.documentElement() );
        }
        else {
            kDebug() << "Failed to parse" << m_filename;
            return false;
        }
    }
    else {
        kDebug() << "Could not open file" << m_filename;
        return false;
    }
}


bool Nepomuk::StrigiConfigFile::readConfig( const QDomElement& rootElement )
{
    if ( rootElement.tagName() != "strigiDaemonConfiguration" ) {
        kDebug() << "Invalid configuration root tag:" << rootElement.tagName();
        return false;
    }

    m_useDBus = convertBooleanXsdValue( rootElement.attribute( "useDBus", QLatin1String( "1" ) ) );

    // read repository
    QDomElement repoElem = rootElement.firstChildElement( "repository" );
    while ( !repoElem.isNull() ) {
        Repository repo = readRepositoryConfig( repoElem );
        if ( repo.isValid() )
            addRepository( repo );
        repoElem = repoElem.nextSiblingElement( "repository" );
    }

    // read filters
    return readFilterConfig( rootElement.firstChildElement( "filters" ) );
}


Nepomuk::StrigiConfigFile::Repository Nepomuk::StrigiConfigFile::readRepositoryConfig( const QDomElement& repositoryElement )
{
    Repository repo;

    QDomNamedNodeMap attributes = repositoryElement.attributes();

    // read repository configuration
    for ( int i = 0; i < attributes.size(); ++i ) {
        QDomNode attributeNode = attributes.item( i );
        QString attributeName = attributeNode.nodeName();
        QString attributeValue = attributeNode.nodeValue();
        if ( attributeName == "type" ) {
            repo.setType( attributeValue );
        }
        else if ( attributeName == "name" ) {
            repo.setName( attributeValue );
        }
        else if ( attributeName == "indexdir" ) {
            repo.setIndexDir( attributeValue );
        }
        else if ( attributeName == "writeable" ) {
            repo.setWriteable( convertBooleanXsdValue( attributeValue ) );
        }
        else if ( attributeName == "urlbase" ) {
            repo.setUrlBase( attributeValue );
        }
        else if ( attributeName == "pollingInterval" ) {
            repo.setPollingInterval( attributeValue.toInt() );
        }
        else {
            kDebug() << "Unknown config entry" << attributeName;
        }
    }

    // read indexed dirs
    QDomElement pathElement = repositoryElement.firstChildElement( "path" );
    while ( !pathElement.isNull() ) {
        QString path = pathElement.attribute( "path" );
        if ( !path.isEmpty() )
            repo.addIndexedDirectory( path );
        pathElement = pathElement.nextSiblingElement( "path" );
    }

    return repo;
}


bool Nepomuk::StrigiConfigFile::readFilterConfig( const QDomElement& filtersElement )
{
    QDomElement filterElement = filtersElement.firstChildElement( "filter" );

    while ( !filterElement.isNull() ) {
        QString pattern = filterElement.attribute( "pattern" );
        QString inExclude = filterElement.attribute( "include" );
        if ( !pattern.isEmpty() && !inExclude.isEmpty() ) {
            if ( convertBooleanXsdValue( inExclude ) )
                m_includeFilters << pattern;
            else
                m_excludeFilters << pattern;
        }
        else {
            kDebug() << "Invalid filter rule.";
            return false;
        }

        filterElement = filterElement.nextSiblingElement( "filter" );
    }

    return true;
}


bool Nepomuk::StrigiConfigFile::save()
{
    kDebug() << m_filename;

    QDomDocument doc;
    QDomElement rootElement = doc.createElement( "strigiDaemonConfiguration" );
    rootElement.setAttribute( "useDBus", useDBus() ? QLatin1String( "1" ) : QLatin1String( "0" ) );
    doc.appendChild( rootElement );

    // save repositories
    foreach( Repository repo, m_repositories ) {
        QDomElement repoElem = doc.createElement( "repository" );
        repoElem.setAttribute( "name", repo.name() );
        repoElem.setAttribute( "type", repo.type() );
        repoElem.setAttribute( "indexdir", repo.indexDir() );
        repoElem.setAttribute( "writeable", repo.writeable() ? QLatin1String( "1" ) : QLatin1String( "0" ) );
        repoElem.setAttribute( "urlbase", repo.urlBase() );
        repoElem.setAttribute( "pollingInterval", QString::number( repo.pollingInterval() ) );

        // add paths
        foreach( QString path, repo.indexedDirectories() ) {
            QDomElement pathElem = doc.createElement( "path" );
            pathElem.setAttribute( "path", path );
            repoElem.appendChild( pathElem );
        }

        rootElement.appendChild( repoElem );
    }

    // save filters
    QDomElement filtersElem = doc.createElement( "filters" );
    rootElement.appendChild( filtersElem );
    foreach( QString filter, m_includeFilters ) {
        QDomElement filterElem = doc.createElement( "filter" );
        filterElem.setAttribute( "pattern", filter );
        filterElem.setAttribute( "include", "1" );
        filtersElem.appendChild( filterElem );
    }
    foreach( QString filter, m_excludeFilters ) {
        QDomElement filterElem = doc.createElement( "filter" );
        filterElem.setAttribute( "pattern", filter );
        filterElem.setAttribute( "include", "0" );
        filtersElem.appendChild( filterElem );
    }

    // save to file
    KStandardDirs::makeDir( m_filename.section( '/', 0, -2 ) );
    QFile f( m_filename );
    if ( f.open( QIODevice::WriteOnly ) ) {
        QTextStream s( &f );
        s << doc;
        return true;
    }
    else {
        kDebug() << "Could not open" << m_filename << "for writing";
        return false;
    }
}


bool Nepomuk::StrigiConfigFile::useDBus() const
{
    return m_useDBus;
}


QStringList Nepomuk::StrigiConfigFile::excludeFilters() const
{
    return m_excludeFilters;
}


QStringList Nepomuk::StrigiConfigFile::includeFilters() const
{
    return m_includeFilters;
}


QList<Nepomuk::StrigiConfigFile::Repository> Nepomuk::StrigiConfigFile::repositories() const
{
    return m_repositories;
}


Nepomuk::StrigiConfigFile::Repository& Nepomuk::StrigiConfigFile::defaultRepository()
{
    if ( m_repositories.isEmpty() ) {
        Repository repo;
        repo.setName( "localhost" ); // the default repository
        repo.setWriteable( true );
        repo.setPollingInterval( 180 ); // default value copied from Strigi sources
#ifdef HAVE_STRIGI_SOPRANO_BACKEND
        repo.setType( "sopranobackend" ); // our default
#else
        repo.setType( "clucene" );
        repo.setIndexDir( QDir::homePath() + "/.strigi/clucene" );
#endif
        repo.addIndexedDirectory( QDir::homePath() );
        repo.addIndexedDirectory( QDir::homePath() + "/.kde" );
        addRepository( repo );

        // in case there are no repositories and no filters we also create
        // default filters
        if ( m_includeFilters.isEmpty() && m_excludeFilters.isEmpty() ) {
            // exclude hidden dirs and files
            m_excludeFilters << ".*/" << ".*";
        }
    }

    return m_repositories.first();
}


const Nepomuk::StrigiConfigFile::Repository& Nepomuk::StrigiConfigFile::defaultRepository() const
{
    return const_cast<StrigiConfigFile*>( this )->defaultRepository();
}


void Nepomuk::StrigiConfigFile::setUseDBus( bool b )
{
    m_useDBus = b;
}

void Nepomuk::StrigiConfigFile::setExcludeFilters( const QStringList& filters )
{
    m_excludeFilters = filters;
}


void Nepomuk::StrigiConfigFile::addExcludeFilter( const QString& filter )
{
    m_excludeFilters << filter;
}


void Nepomuk::StrigiConfigFile::setIncludeFilters( const QStringList& filters )
{
    m_includeFilters = filters;
}


void Nepomuk::StrigiConfigFile::addInludeFilter( const QString& filter )
{
    m_includeFilters << filter;
}

void Nepomuk::StrigiConfigFile::setRepositories( const QList<Repository>& repos )
{
    m_repositories = repos;
}


void Nepomuk::StrigiConfigFile::addRepository( const Repository& repo )
{
    m_repositories << repo;
}


QString Nepomuk::StrigiConfigFile::defaultStrigiConfigFilePath()
{
    return QDir::homePath() + "/.strigi/daemon.conf";
}


QTextStream& operator<<( QTextStream& s, const Nepomuk::StrigiConfigFile& scf )
{
    s << "useDBus: " << scf.useDBus() << endl
      << "repositories:" << endl;
    foreach( Nepomuk::StrigiConfigFile::Repository repo, scf.repositories() ) {
        s << "   " << repo.name() << ":" << endl
          << "      " << "type: " << repo.type() << endl
          << "      " << "indexdir: " << repo.indexDir() << endl
          << "      " << "writeable: " << repo.writeable() << endl
          << "      " << "urlbase: " << repo.urlBase() << endl
          << "      " << "paths: " << repo.indexedDirectories().join( ":" ) << endl;
    }
    s << "include filters:" << endl;
    foreach( QString filter, scf.includeFilters() ) {
        s << "   " << filter << endl;
    }
    s << "exclude filters:" << endl;
    foreach( QString filter, scf.excludeFilters() ) {
        s << "   " << filter << endl;
    }
    return s;
}

#include "strigiconfigfile.moc"
