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
	Repository* repository( const QString& name );

	QStringList allModels() const;

    private:
	QString createStoragePath( const QString& repositoryId ) const;

	RepositoryMap m_repositories;
    };
}

#endif
