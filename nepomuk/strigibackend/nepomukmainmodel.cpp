/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
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

#include "nepomukmainmodel.h"

#include <Soprano/Node>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Client/DBusModel>
#include <Soprano/Query/QueryLanguage>
#include <Soprano/Util/DummyModel>
#include <Soprano/Util/MutexModel>

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QDir>

#include <stdlib.h>


// FIXME: disconnect localSocketClient after n seconds of idling (but take care of not
//        disconnecting when iterators are open)

using namespace Soprano;

namespace {
    QString nepomukServerSocketPath() {
        QString kdeHome = getenv( "KDEHOME" );
        if ( kdeHome.isEmpty() ) {
            kdeHome = QDir::homePath() + "/.kde4";
        }
        return kdeHome + "/share/apps/nepomuk/socket";
    }
}


Nepomuk::MainModel::MainModel( QObject* parent )
    : Soprano::Model(),
      m_dbusClient( "org.kde.NepomukStorage" ),
      m_dbusModel( 0 ),
      m_localSocketModel( 0 ),
      m_mutexModel( 0 ),
      m_socketConnectFailed( false )
{
    setParent( parent );
    init();
}


Nepomuk::MainModel::~MainModel()
{
    delete m_mutexModel;
    delete m_localSocketModel;
    delete m_dbusModel;
}



void Nepomuk::MainModel::init()
{
    if ( !m_dbusModel ) {
        m_dbusModel = m_dbusClient.createModel( "main" );
    }

    if ( !m_mutexModel ) {
        m_mutexModel = new Soprano::Util::MutexModel( Soprano::Util::MutexModel::ReadWriteMultiThreading );
    }

    // we may get disconnected from the server but we don't want to try
    // to connect every time the model is requested
    // FIXME: I doubt that this will work since the client is created in another thread than the
    //        connections and that is something Qt really does not like. :(
    if ( !m_socketConnectFailed && !m_localSocketClient.isConnected() ) {
        delete m_localSocketModel;
        QString socketName = nepomukServerSocketPath();
        if ( m_localSocketClient.connect( socketName ) ) {
            m_localSocketModel = m_localSocketClient.createModel( "main" );
        }
        else {
            m_socketConnectFailed = true;
            qDebug() << "Failed to connect to Nepomuk server via local socket" << socketName;
        }
    }
}


Soprano::Model* Nepomuk::MainModel::model() const
{
    const_cast<MainModel*>(this)->init();

    // we always prefer the faster local socket client
    if ( m_localSocketModel ) {
        if ( m_mutexModel->parentModel() != m_localSocketModel ) {
            m_mutexModel->setParentModel( m_localSocketModel );
        }
    }
    else if ( m_dbusModel ) {
        if ( m_mutexModel->parentModel() != m_dbusModel ) {
            m_mutexModel->setParentModel( m_dbusModel );
        }
    }

    return m_mutexModel;
}


bool Nepomuk::MainModel::isValid() const
{
    return m_dbusClient.isValid() || m_localSocketClient.isConnected();
}


Soprano::StatementIterator Nepomuk::MainModel::listStatements( const Statement& partial ) const
{
    Soprano::StatementIterator it = model()->listStatements( partial );
    setError( model()->lastError() );
    return it;
}


Soprano::NodeIterator Nepomuk::MainModel::listContexts() const
{
    Soprano::NodeIterator it = model()->listContexts();
    setError( model()->lastError() );
    return it;
}


Soprano::QueryResultIterator Nepomuk::MainModel::executeQuery( const QString& query,
                                                               Soprano::Query::QueryLanguage language,
                                                               const QString& userQueryLanguage ) const
{
    Soprano::QueryResultIterator it = model()->executeQuery( query, language, userQueryLanguage );
    setError( model()->lastError() );
    return it;
}


bool Nepomuk::MainModel::containsStatement( const Statement& statement ) const
{
    bool b = model()->containsStatement( statement );
    setError( model()->lastError() );
    return b;
}


bool Nepomuk::MainModel::containsAnyStatement( const Statement &statement ) const
{
    bool b = model()->containsAnyStatement( statement );
    setError( model()->lastError() );
    return b;
}


bool Nepomuk::MainModel::isEmpty() const
{
    bool b = model()->isEmpty();
    setError( model()->lastError() );
    return b;
}


int Nepomuk::MainModel::statementCount() const
{
    int c = model()->statementCount();
    setError( model()->lastError() );
    return c;
}


Soprano::Error::ErrorCode Nepomuk::MainModel::addStatement( const Statement& statement )
{
    Soprano::Error::ErrorCode c = model()->addStatement( statement );
    setError( model()->lastError() );
    return c;
}


Soprano::Error::ErrorCode Nepomuk::MainModel::removeStatement( const Statement& statement )
{
    Soprano::Error::ErrorCode c = model()->removeStatement( statement );
    setError( model()->lastError() );
    return c;
}


Soprano::Error::ErrorCode Nepomuk::MainModel::removeAllStatements( const Statement& statement )
{
    Soprano::Error::ErrorCode c = model()->removeAllStatements( statement );
    setError( model()->lastError() );
    return c;
}


Soprano::Node Nepomuk::MainModel::createBlankNode()
{
    Soprano::Node n = model()->createBlankNode();
    setError( model()->lastError() );
    return n;
}

#include "nepomukmainmodel.moc"
