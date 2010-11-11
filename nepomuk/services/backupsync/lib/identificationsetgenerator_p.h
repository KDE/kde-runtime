/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

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


#ifndef IDENTIFICATION_SET_GENERATOR_P_H
#define IDENTIFICATION_SET_GENERATOR_P_H

#include <QtCore/QSet>
#include <KUrl>

namespace Soprano {
    class Model;
    class Statement;
    class QueryResultIterator;
}

namespace Nepomuk {
    namespace Sync {
        
        class IdentificationSetGenerator {
        public :
            IdentificationSetGenerator( const QSet<KUrl>& uniqueUris, Soprano::Model * m , const QSet<KUrl> & ignoreList = QSet<KUrl>());

            Soprano::Model * model;
            QSet<KUrl> done;
            QSet<KUrl> notDone;

            QList<Soprano::Statement> statements;

            Soprano::QueryResultIterator performQuery( const QStringList& uris );
            void iterate();
            QList<Soprano::Statement> generate();

            static const int maxIterationSize = 50;
        };
    }
}
#endif
