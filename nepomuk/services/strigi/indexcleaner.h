/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-11 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2010-11 Vishesh Handa <handa.vish@gmail.com>

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

#ifndef INDEXCLEANER_H
#define INDEXCLEANER_H

#include <QtCore/QString>
#include <QtCore/QObject>

namespace Nepomuk {
    class IndexCleaner : public QObject
    {
        Q_OBJECT
    public:
        IndexCleaner(QObject* parent=0);
        ~IndexCleaner();

    public slots:
        /**
         * Removes all previously indexed entries that are not in the list of folders
         * to index anymore.
         */
        void removeOldAndUnwantedEntries();
        bool removeAllGraphsFromQuery( const QString& query_ );

    private:

        //FIXME: Use this somehow! Also support pause/resume
        bool m_stopped;
    };

}

#endif // INDEXCLEANER_H
