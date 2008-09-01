/*  This file is part of the KDE semantic clipboard
    Copyright (C) 2008 Tobias Wolf <twolf@access.unizh.ch>
    Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _NEPOMUK_GRAPHRETRIEVER_H_
#define _NEPOMUK_GRAPHRETRIEVER_H_

#include <KJob>

class QString;
class QUrl;

namespace Soprano {
    class Model;
    class StatementIterator;
}
namespace KIO {
    class Job;
}

namespace Nepomuk
{

    /**
     * \class GraphRetriever graphretriever.h <nepomuk/graphretriever.h>
     *
     * \brief Utility class for retrieving RDF graphs from Web locations.
     *
     * GraphRetriever allows retrieval of multiple RDF graph fragments from
     * the Web using HTTP GET. Target HTTP servers should support the RDF+XML
     * request MIME type for this to work.
     *
     * Clients can queue multiple get requests and must follow the last get
     * request with a call to either model() or serialization().
     *
     * \author Tobias Wolf <twolf@access.unizh.ch>
     */
    class GraphRetriever : public KJob
    {
        Q_OBJECT

    public:
        /**
         * Custom and default constructor.
         *
         * @param parent The parent QObject.
         */
        GraphRetriever( QObject* parent = 0 );

        /**
         * Default destructor.
         */
        ~GraphRetriever();

        static GraphRetriever* retrieve( const QUrl& uri );

        void setUrl( const QUrl& url );

        QUrl url() const;

        /**
         * Adds a get request for the given URL.
         *
         * @param url An URL specifying the location of a graph fragment to get.
         * @return True if the get request was accepted, false otherwise.
         */
        void start();

        /**
         * Returns a new Soprano model constructed from the retrieved graph fragments. The
         * client takes over ownership of the returned model.
         *
         * @return A new Soprano model constructed from the merged fragments.
         */
        Soprano::Model* model() const;

        Soprano::StatementIterator statements() const;

    private Q_SLOTS:
        /**
         * @internal
         */
        void httpRequestFinished( KJob* );

    private:
        class Private;
        Private* const d;
    };

}

#endif
