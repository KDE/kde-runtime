/*
   Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _STRIGI_NEPOMUK_UTIL_H_
#define _STRIGI_NEPOMUK_UTIL_H_

#include <string>
#include <KUrl>

class QUrl;
class QString;

namespace Soprano {
    class Model;
    class Node;
}

namespace Strigi {

    class Variant;

    namespace Util {
        QUrl fieldUri( const std::string& s );
        QUrl fileUrl( const std::string& filename );
        std::string fieldName( const QUrl& uri );
        QUrl uniqueUri( const QString& ns, ::Soprano::Model* model );
    }

    namespace Ontology {
        QUrl indexGraphFor();
    }
}

namespace Nepomuk {
    /// remove all indexed data for \p url the datamanagement way
    bool clearIndexedData( const QUrl& url );
    bool clearIndexedData( const QList<QUrl>& urls );

    /// clears data from pre-datamanagement days
    bool clearLegacyIndexedDataForUrl( const KUrl& url );

    /// clears data from pre-datamanagement days
    bool clearLegacyIndexedDataForResourceUri( const KUrl& res );
}
#endif
