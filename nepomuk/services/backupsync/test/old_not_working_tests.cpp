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

#include "backuptest.h"
#include "common.h"
#include "nie.h"

#include <Soprano/Statement>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Nepomuk/Service>

#include <kdebug.h>
#include <ktemporaryfile.h>
#include <qtest_kde.h>

#include <Soprano/Soprano>

#include <KService>
#include <KServiceTypeTrader>

#include "../service/backupsyncservice.h"
#include <backupmanager.h>

void BackupTest::initTestCase()
{
    m1 = Nepomuk::createNewModel( m_model1Dir.name() );
    Nepomuk::setAsDefaultModel( m1 );

    m_service = new Nepomuk::BackupSyncService( this );

    qDebug() << "WTF";
}


void BackupTest::cleanupTestCase()
{

    delete m_service;
}


void BackupTest::simpleBackupTest()
{
    KTempDir tempDir;

    //
    // Create File System
    //
    Nepomuk::FileGenerator fg;
    fg.create( tempDir.name(), 100 );

    //
    // Create a backup
    //
    Nepomuk::setAsDefaultModel( m1 );
    Nepomuk::MetadataGenerator mg( m1 );

    foreach( const QUrl & url, fg.urls() ) {
        mg.rate( url );
    }
    m_service->backupManager()->backup();

    QSet<Soprano::Statement> m1Statements = Nepomuk::getBackupStatements( m1 ).toSet();

    Nepomuk::setAsDefaultModel( 0 );
    delete m1;

    //
    // Restore
    //

    m2 = Nepomuk::createNewModel( m_model2Dir.name() );
    kDebug() << "Setting as default";

    Nepomuk::setAsDefaultModel( m2 );

    kDebug() << "Restoring";
    m_service->backupManager()->restore();
    kDebug() << "Restored";

    //
    // Let's hope it worked!
    //

    QSet<Soprano::Statement> m2Statements = Nepomuk::getBackupStatements( m2 ).toSet();

    qDebug() << " m1 : " << m1Statements.size();
    qDebug() << " m2 : " << m2Statements.size();

    QVERIFY( m1Statements.size() == m2Statements.size() );

    kDebug() << "Done!";
    delete( m2 );
}


QTEST_KDEMAIN(BackupTest, NoGUI)

#include "backuptest.moc"
