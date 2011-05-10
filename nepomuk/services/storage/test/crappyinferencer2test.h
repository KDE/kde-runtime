/*
   This file is part of the Nepomuk KDE project.
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

#ifndef CRAPPYINFERENCER2TEST_H
#define CRAPPYINFERENCER2TEST_H

#include <QObject>

class KTempDir;
class CrappyInferencer2;
namespace Soprano {
class Model;
}
namespace Nepomuk {
class ClassAndPropertyTree;
}

class CrappyInferencer2Test : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testAddAnyStatement();
    void testAddTypeStatement();
    void testAddSubClassOfStatement();
    void testRemoveAnyStatement();
    void testRemoveTypeStatement();
    void testRemoveSubClassOfStatement();
    void testRemoveAllStatements();
    void testCyclicSubClassRelation();
    void testUpdateAllResources();

private:
    KTempDir* m_storageDir;
    Soprano::Model* m_baseModel;
    Nepomuk::ClassAndPropertyTree* m_typeTree;
    CrappyInferencer2* m_model;
};

#endif
