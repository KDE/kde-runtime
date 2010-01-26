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

static const char s_repositoryName[] = "main";

Nepomuk::Core::Core( QObject* parent )
    : Soprano::Server::ServerCore( parent ),
      m_repository( 0 )
{
    // we give the Virtuoso server a server thread max of 100 which is already an insane number
    // just make sure we never reach that limit
    setMaximumConnectionCount( 80 );
}


Nepomuk::Core::~Core()
{
    kDebug() << "Shutting down Nepomuk storage core.";
    delete m_repository;
}


void Nepomuk::Core::init()
{
    // TODO: export the main model on org.kde.NepomukRepository via Soprano::Server::DBusExportModel

    // we have only the one repository
    model( QLatin1String( s_repositoryName ) );
}


bool Nepomuk::Core::initialized() const
{
    return( m_repository && m_repository->state() == Repository::OPEN );
}


void Nepomuk::Core::slotRepositoryOpened( Repository*, bool success )
{
    emit initializationDone( success );
}


Soprano::Model* Nepomuk::Core::model( const QString& name )
{
    // we only allow the one model
    if ( name == QLatin1String( s_repositoryName ) ) {
        if ( !m_repository ) {
            m_repository = new Repository( name );
            connect( m_repository, SIGNAL( opened( Repository*, bool ) ),
                     this, SLOT( slotRepositoryOpened( Repository*, bool ) ) );
            QTimer::singleShot( 0, m_repository, SLOT( open() ) );
        }
        return m_repository;
    }
    else {
        return 0;
    }
}

#include "nepomukcore.moc"
