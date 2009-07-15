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

#ifndef _NEPOMUK_CORE_H_
#define _NEPOMUK_CORE_H_

#include <Soprano/Server/ServerCore>
#include "repository.h"

#include <QtCore/QStringList>

namespace Nepomuk {

    class Core : public Soprano::Server::ServerCore
    {
        Q_OBJECT

    public:
        Core( QObject* parent = 0 );
        ~Core();

        /**
         * reimplemented from ServerCore
         */
        Soprano::Model* model( const QString& name );

        /**
         * Open all repositories.
         */
        void init();

        /**
         * \return \p true if the core is already initialized.
         * \p false otherwise, which means the initializationDone
         * signal has not been emitted yet.
         */
        bool initialized() const;

    public Q_SLOTS:
        void optimize( const QString& repoName );
        void rebuildIndex( const QString& repoName );

    Q_SIGNALS:
        void initializationDone( bool success );

    private Q_SLOTS:
        void slotRepositoryOpened( Repository* repo, bool success );

    private:
        /**
         * reimplemented from ServerCode
         */
        Soprano::Model* createModel( const QList<Soprano::BackendSetting>& settings );
        void createRepository( const QString& name );

        RepositoryMap m_repositories;

        // initialization
        QStringList m_openingRepositories;
        QString m_currentRepoName;

        // true if one of the repos could not be opened
        bool m_failedToOpenRepository;
    };
}

#endif
