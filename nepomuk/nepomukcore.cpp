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

#include "nepomukcore.h"
#include "repository.h"

#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QtCore/QTimer>


Nepomuk::Core::Core( QObject* parent )
    : Soprano::Server::ServerCore( parent )
{
}


Nepomuk::Core::~Core()
{
    kDebug(300002) << "Shutting down Nepomuk core services.";
    qDeleteAll( m_repositories );
}


void Nepomuk::Core::init()
{
    // FIXME: export the main model on org.kde.NepomukRepository via Soprano::Server::DBusExportModel

    KSharedConfig::Ptr config = Server::self()->config();

    const Soprano::Backend* backend = Repository::activeSopranoBackend();
    if ( backend ) {
        m_openingRepositories = config->group( "Basic Settings" ).readEntry( "Configured repositories", QStringList() << "main" );
        if ( !m_openingRepositories.contains( "main" ) ) {
            m_openingRepositories << "main";
        }

        foreach( QString repoName, m_openingRepositories ) {
            createRepository( repoName );
        }

        if ( m_openingRepositories.isEmpty() ) {
            emit initializationDone( true );
        }
    }
    else {
        kError() << "No Soprano backend found. Cannot start Nepomuk repository.";
    }
}


bool Nepomuk::Core::initialized() const
{
    return m_openingRepositories.isEmpty() && !m_repositories.isEmpty();
}


void Nepomuk::Core::createRepository( const QString& name )
{
    Repository* repo = new Repository( name );
    m_repositories.insert( name, repo );
    connect( repo, SIGNAL( opened( Repository*, bool ) ),
             this, SLOT( slotRepositoryOpened( Repository*, bool ) ) );
    QTimer::singleShot( 0, repo, SLOT( open() ) );
}


void Nepomuk::Core::slotRepositoryOpened( Repository* repo, bool success )
{
    // FIXME: do something with success
    m_openingRepositories.removeAll( repo->name() );
    if ( m_openingRepositories.isEmpty() ) {
        emit initializationDone( true );
    }
}


Soprano::Model* Nepomuk::Core::model( const QString& name )
{
    return repository( name );
}


QStringList Nepomuk::Core::allModels() const
{
    QStringList models;
    foreach( Repository* repo, m_repositories ) {
        models.append( repo->name() );
    }
    return models;
}


Nepomuk::Repository* Nepomuk::Core::repository( const QString& name )
{
    if ( !m_repositories.contains( name ) ) {
        kDebug(300002) << "Creating new repository with name " << name;

        // FIXME: There should be no need for conversion but who knows...
        Repository* newRepo = new Repository( name );
        m_repositories.insert( name, newRepo );
        newRepo->open();
        return newRepo;
    }
    else {
        return m_repositories[name];
    }
}



#include "nepomukcore.moc"
