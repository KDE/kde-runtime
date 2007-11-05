/*
   $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $

   This file is part of the Strigi project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef STRIGI_SOPRANO_INDEX_MANAGER_H
#define STRIGI_SOPRANO_INDEX_MANAGER_H

#include <strigi/indexmanager.h>

class QString;

namespace Soprano {
    class Model;
}

namespace Strigi {

    class IndexReader;
    class IndexWriter;

    namespace Soprano {
	class IndexManager : public Strigi::IndexManager
	{
	public:
	    IndexManager( ::Soprano::Model*, const QString& );
	    ~IndexManager();

	    Strigi::IndexReader* indexReader();
	    Strigi::IndexWriter* indexWriter();

	private:
	    class Private;
	    Private* d;
	};
    }
}

#endif
