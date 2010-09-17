/*
   Copyright (c) 2010 Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "countqueryrunnable.h"
#include "folder.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Query/Query>

#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/LiteralValue>
#include <Soprano/Model>

#include <KDebug>


Nepomuk::Query::CountQueryRunnable::CountQueryRunnable( Folder* folder )
    : QRunnable(),
      m_folder( folder )
{
    kDebug();
}


Nepomuk::Query::CountQueryRunnable::~CountQueryRunnable()
{
}


void Nepomuk::Query::CountQueryRunnable::run()
{
    int count = -1;
    const QString query = m_folder->query().toSparqlQuery( Query::CreateCountQuery );
    Soprano::QueryResultIterator it = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    if( it.next() ) {
        count = it.binding( 0 ).literal().toInt();
    }
    kDebug() << count;
    if( m_folder )
        m_folder->countQueryFinished( count );
}
