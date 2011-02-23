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

#ifndef DATAMANAGEMENTMODELTEST_H
#define DATAMANAGEMENTMODELTEST_H

#include <QObject>

namespace Soprano {
class Model;
class NRLModel;
}
namespace Nepomuk {
class DataManagementModel;
}
class KTempDir;

class DataManagementModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testAddProperty();
    void testAddProperty_createRes();
    void testAddProperty_cardinality();
    void testAddProperty_file();
    void testAddProperty_invalidFile();
    void testAddProperty_invalid_args();

    void testSetProperty();
    void testSetProperty_createRes();
    void testSetProperty_overwrite();
    void testSetProperty_invalid_args();
    void testSetProperty_nieUrl1();
    void testSetProperty_nieUrl2();
    void testSetProperty_nieUrl3();
    void testSetProperty_nieUrl4();
    void testSetProperty_nieUrl5();

    void testRemoveProperty();
    void testRemoveProperty_file();
    void testRemoveProperty_invalid_args();

    void testRemoveProperties();
    void testRemoveProperties_invalid_args();
    
    void testRemoveResources();
    void testRemoveResources_subresources();
    void testRemoveResources_invalid_args();
    
    void testRemoveDataByApplication1();
    void testRemoveDataByApplication2();
    void testRemoveDataByApplication3();
    void testRemoveDataByApplication4();
    void testRemoveDataByApplication5();
    void testRemoveDataByApplication6();

    void testMergeResources();
    void testMergeResources_createResource();
    void testMergeResources_invalid_args();
    void testMergeResources_file1();
    void testMergeResources_file2();

    void testDescribeResources();

private:
    KTempDir* createNieUrlTestData();

    void resetModel();

    KTempDir* m_storageDir;
    Soprano::Model* m_model;
    Soprano::NRLModel* m_nrlModel;
    Nepomuk::DataManagementModel* m_dmModel;
};

#endif
