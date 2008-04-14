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

#ifndef _NEPOMUK_MAIN_MODEL_H_
#define _NEPOMUK_MAIN_MODEL_H_

#include <Soprano/Model>
#include <Soprano/Client/DBusClient>
#include <Soprano/Client/LocalSocketClient>

namespace Soprano {
    namespace Client {
        class DBusModel;
        class LocalSocketModel;
    }
    namespace Util {
        class MutexModel;
    }
}

namespace Nepomuk {
    /**
     * \brief The main %Nepomuk data storage model.
     *
     * All user data is stored in the %Nepomuk main model.
     * This is a wrapper model which communicates all commands
     * to the %Nepomuk server.
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class MainModel : public Soprano::Model
    {
        Q_OBJECT

    public:
        /**
         * Create a new main model.
         */
        MainModel( QObject* parent = 0 );

        /**
         * Destructor.
         */
        ~MainModel();

        /**
         * Check the connection to the %Nepomuk server.
         * \return \p true if the connection is valid and commands can be issued,
         * \p false otherwise.
         */
        bool isValid() const;

        Soprano::StatementIterator listStatements( const Soprano::Statement &partial ) const;
        Soprano::NodeIterator listContexts() const;
        Soprano::QueryResultIterator executeQuery( const QString& query, 
                                                   Soprano::Query::QueryLanguage language, 
                                                   const QString& userQueryLanguage = QString() ) const;
        bool containsStatement( const Soprano::Statement &statement ) const;
        bool containsAnyStatement( const Soprano::Statement &statement ) const;
        bool isEmpty() const;
        int statementCount() const;
        Soprano::Error::ErrorCode addStatement( const Soprano::Statement& statement );
        Soprano::Error::ErrorCode removeStatement( const Soprano::Statement& statement );
        Soprano::Error::ErrorCode removeAllStatements( const Soprano::Statement& statement );
        Soprano::Node createBlankNode();

    private:
        void init();
        Soprano::Model* model() const;

        Soprano::Client::DBusClient m_dbusClient;
        Soprano::Client::LocalSocketClient m_localSocketClient;
        Soprano::Client::DBusModel* m_dbusModel;
        Soprano::Model* m_localSocketModel;
        Soprano::Util::MutexModel* m_mutexModel;
        bool m_socketConnectFailed;
    };
}

#endif
