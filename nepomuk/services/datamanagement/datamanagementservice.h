/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>
             (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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

#ifndef DATAMANAGEMENTSERVICE_H
#define DATAMANAGEMENTSERVICE_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

#include <Nepomuk/Service>

namespace Nepomuk {
/**
 *
 * \author Sebastian Trueg <trueg@kde.org>, Vishesh Handa <handa.vish@gmail.com>
 */
class DataManagementService : public Nepomuk::Service
{
    Q_OBJECT

public:
    DataManagementService(QObject *parent = 0, const QList<QVariant>& args = QList<QVariant>());
    ~DataManagementService();

private:
    class Private;
    Private* const d;
};
}

#endif // DATAMANAGEMENTSERVICE_H
