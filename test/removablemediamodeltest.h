/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

#ifndef REMOVABLEMEDIAMODELTEST_H
#define REMOVABLEMEDIAMODELTEST_H

#include <QObject>
#include <QStringList>

namespace Nepomuk {
class RemovableMediaModel;
}
namespace Soprano {
class Model;
}

class RemovableMediaModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testConvertFileUrlsInStatement_data();
    void testConvertFileUrlsInStatement();
    void testConvertFileUrlsInQuery_data();
    void testConvertFileUrlsInQuery();
    void testConvertFilxUrl_data();
    void testConvertFilxUrl();
    void testConvertFilxUrls_data();
    void testConvertFilxUrls();

private:
    Soprano::Model* m_model;
    Nepomuk::RemovableMediaModel* m_rmModel;
    QStringList m_removableDevices;
};

#endif
