/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef NEPOMUKINDEXFEEDER_H
#define NEPOMUKINDEXFEEDER_H

#include <QtCore/QUrl>
#include <QtCore/QStack>
#include <QtCore/QSet>

namespace Soprano {
    class Statement;
    class Node;
}
class KUrl;

namespace Nepomuk {
    class IndexFeeder : public QObject
    {
        Q_OBJECT
    public:
        IndexFeeder( QObject* parent = 0 );
        virtual ~IndexFeeder();

    public Q_SLOTS:
        /**
         * Starts the feeding process for a file with resourceUri \p uri.
         * This function should be called before adding any statements.
         * The function may be called repeatedly.
         *
         * Should be preceded by an end()
         *
         * \sa end
         */
        void begin( const QUrl & uri );

        /**
         * Adds \p st to the list of statements to be added.
         * \p st may contain Blank Nodes.The context is ignored.
         * Should be called between begin and end
         *
         * \sa begin end
         */
        void addStatement( const Soprano::Statement & st );

        /**
         * Adds the subject, predicate, object to the list of statements
         * to be added. The Subject or Object may contain Blank Nodes
         * Should be called between begin and end
         *
         * \sa begin end
         */
        void addStatement( const Soprano::Node & subject,
                           const Soprano::Node & predicate,
                           const Soprano::Node & object );

        /**
         * Finishes the feeding process, and starts with the resolution and merging
         * of resources based on the statements provided.
         *
         * addStatement should not be called after this function, unless begin has already
         * been called.
         *
         * \param forceCommit If true the request will be handled syncroneously. This is used
         * to commit folder resources to Nepomuk right away since they might be needed later on.
         *
         * \sa begin
         */
        void end();

        /**
         * Returns the uri of the main resource of the last request that was handled
         */
        QUrl lastRequestUri() const;
        
    private:

        struct ResourceStruct {
            QUrl uri;
            QMultiHash<QUrl, Soprano::Node> propHash;
        };

        // Maps the uri to the ResourceStuct
        typedef QHash<QUrl, ResourceStruct> ResourceHash;

        struct Request {
            QUrl uri;
            ResourceHash hash;
        };

        QUrl m_lastRequestUri;
        
        /**
         * The stack is used to store the internal state of the Feeder, a new item is pushed into
         * the stack every time begin() is called, and the top most item is poped and sent into the
         * update queue when end() is called.
         */
        QStack<Request> m_stack;

        /**
         * Handle a single request, i.e. store all its data to Nepomuk.
         */
        void handleRequest( Nepomuk::IndexFeeder::Request& request );

        /// Generates a discardable graph for \p resourceUri
        QUrl generateGraph( const QUrl& resourceUri ) const;

        /**
         * Creates a sparql query which returns 1 resource which matches all the properties,
         * and objects present in the propHash of the ResourceStruct
         */
        QString buildResourceQuery( const ResourceStruct & rs ) const;

        /**
         * Adds all the statements present in the ResourceStruct to the internal model.
         * The contex is created via generateGraph
         *
         * \sa generateGraph
         */
        void addToModel( const ResourceStruct &rs ) const;
    };

}

#endif // NEPOMUKINDEXFEEDER_H
