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
#include <KStandardDirs>

#include <QtCore/QTimer>

#include <Soprano/BackendSetting>


Nepomuk::Core::Core( QObject* parent )
    : Soprano::Server::ServerCore( parent )
{
}


Nepomuk::Core::~Core()
{
    kDebug(300002) << "Shutting down Nepomuk storage core.";

    KSharedConfig::Ptr config = KSharedConfig::openConfig( "nepomukserverrc" );
    config->group( "Basic Settings" ).writeEntry( "Configured repositories", m_repositories.keys() );
}


void Nepomuk::Core::init()
{
    // TODO: export the main model on org.kde.NepomukRepository via Soprano::Server::DBusExportModel

    m_failedToOpenRepository = false;

    KSharedConfig::Ptr config = KSharedConfig::openConfig( "nepomukserverrc" );

    const Soprano::Backend* backend = Repository::activeSopranoBackend();
    if ( backend ) {
        m_openingRepositories = config->group( "Basic Settings" ).readEntry( "Configured repositories", QStringList() << "main" );
        if ( !m_openingRepositories.contains( "main" ) ) {
            m_openingRepositories << "main";
        }

        foreach( const QString &repoName, m_openingRepositories ) {
            createRepository( repoName );
        }

        if ( m_openingRepositories.isEmpty() ) {
            emit initializationDone( !m_failedToOpenRepository );
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

    // make sure ServerCore knows about the repo (important for memory management)
    model( name );
}


void Nepomuk::Core::slotRepositoryOpened( Repository* repo, bool success )
{
    m_failedToOpenRepository = m_failedToOpenRepository && !success;
    m_openingRepositories.removeAll( repo->name() );
    if ( m_openingRepositories.isEmpty() ) {
        emit initializationDone( !m_failedToOpenRepository );
    }
}


Soprano::Model* Nepomuk::Core::model( const QString& name )
{
    // we need the name of the model for the repository creation
    // but on the other hand want to use the AsyncModel stuff from
    // ServerCore. Thus, we have to hack a bit
    m_currentRepoName = name;
    return ServerCore::model( name );
}


Soprano::Model* Nepomuk::Core::createModel( const QList<Soprano::BackendSetting>& )
{
    // use the name we cached in model()
    if ( !m_repositories.contains( m_currentRepoName ) ) {
        kDebug(300002) << "Creating new repository with name " << m_currentRepoName;

        // FIXME: There should be no need for conversion but who knows...
        Repository* newRepo = new Repository( m_currentRepoName );
        m_repositories.insert( m_currentRepoName, newRepo );
        newRepo->open();
        return newRepo;
    }
    else {
        return m_repositories[m_currentRepoName];
    }
}


void Nepomuk::Core::optimize( const QString& name )
{
    if ( m_repositories.contains( name ) )
        m_repositories[name]->optimize();
}

#include "nepomukcore.moc"
