/*
 *  <one line to give the program's name and a brief idea of what it does.>
 *  Copyright (C) 2010  Vishesh Handa
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _RESOURCE_TEST_H_
#define _RESOURCE_TEST_H_

#include <QtTest/QtTest>
#include <KTempDir>

namespace Soprano {
    class Model;
}

namespace Nepomuk {
    class BackupSyncService;
};


class BackupTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void simpleBackupTest();
private:
    Nepomuk::BackupSyncService * m_service;

    Soprano::Model * m1;
    Soprano::Model * m2;

    KTempDir m_model1Dir;
    KTempDir m_model2Dir;
};

#endif
