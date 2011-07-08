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
class ClassAndPropertyTree;
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
    void testAddProperty_protectedTypes();
    void testAddProperty_akonadi();

    void testSetProperty();
    void testSetProperty_createRes();
    void testSetProperty_overwrite();
    void testSetProperty_invalid_args();
    void testSetProperty_nieUrl1();
    void testSetProperty_nieUrl2();
    void testSetProperty_nieUrl3();
    void testSetProperty_nieUrl4();
    void testSetProperty_nieUrl5();
    void testSetProperty_nieUrl6();
    void testSetProperty_protectedTypes();

    void testRemoveProperty();
    void testRemoveProperty_file();
    void testRemoveProperty_invalid_args();
    void testRemoveProperty_protectedTypes();

    void testRemoveProperties();
    void testRemoveProperties_invalid_args();
    void testRemoveProperties_protectedTypes();

    void testRemoveResources();
    void testRemoveResources_subresources();
    void testRemoveResources_invalid_args();
    void testRemoveResources_protectedTypes();
    void testRemoveResources_mtimeRelated();

    void testCreateResource();
    void testCreateResource_invalid_args();

    void testRemoveDataByApplication1();
    void testRemoveDataByApplication2();
    void testRemoveDataByApplication3();
    void testRemoveDataByApplication4();
    void testRemoveDataByApplication5();
    void testRemoveDataByApplication6();
    void testRemoveDataByApplication7();
    void testRemoveDataByApplication8();
    void testRemoveDataByApplication9();
    void testRemoveDataByApplication10();
    void testRemoveDataByApplication11();
    void testRemoveDataByApplication_subResourcesOfSubResources();
    void testRemoveDataByApplication_realLife();
    void testRemoveDataByApplication_nieUrl();
    void testRemoveDataByApplication_mtime();
    void testRemoveDataByApplication_mtimeRelated();
    void testRemoveDataByApplication_related();
    void testRemoveDataByApplication_legacyIndexerData();

    void testRemoveAllDataByApplication1();
    void testRemoveAllDataByApplication2();
    void testRemoveAllDataByApplication3();
    void testRemoveAllDataByApplication4();

    void testStoreResources_strigiCase();
    void testStoreResources_graphRules();
    void testStoreResources_createResource();
    void testStoreResources_invalid_args();
    void testStoreResources_invalid_args_with_existing();
    void testStoreResources_file1();
    void testStoreResources_file2();
    void testStoreResources_file3();
    void testStoreResources_file4();
    void testStoreResources_fileExists();
    void testStoreResources_sameNieUrl();
    void testStoreResources_metadata();
    void testStoreResources_protectedTypes();
    void testStoreResources_superTypes();
    void testStoreResources_missingMetadata();
    void testStoreResources_multiMerge();
    void testStoreResources_realLife();
    void testStoreResources_trivialMerge();
    void testStoreResources_noTypeMatch1();
    void testStoreResources_noTypeMatch2();
    void testStoreResources_faultyMetadata();
    void testStoreResources_additionalMetadataApp();
    void testStoreResources_itemUris();
    void testStoreResources_kioProtocols();
    void testStoreResources_duplicates();
    void testStoreResources_overwriteProperties();
    void testStoreResources_correctDomainInStore();
    void testStoreResources_correctDomainInStore2();
    void testStoreResources_correctRangeInStore();
    void testStoreResources_correctRangeInStore2();
    void testStoreResources_duplicateValuesAsString();
    void testStoreResources_ontology();
    void testStoreResources_legacyUris();
    void testStoreResources_lazyCardinalities();
    void testStoreResources_graphMetadataFail();
    void testStoreResources_randomNepomukUri();

    void testMergeResources();
    void testMergeResources_protectedTypes();

    void testDescribeResources();

    void testImportResources();

private:
    KTempDir* createNieUrlTestData();

    void resetModel();
    bool haveTrailingGraphs() const;

    KTempDir* m_storageDir;
    Soprano::Model* m_model;
    Soprano::NRLModel* m_nrlModel;
    Nepomuk::ClassAndPropertyTree* m_classAndPropertyTree;
    Nepomuk::DataManagementModel* m_dmModel;
};

#endif
