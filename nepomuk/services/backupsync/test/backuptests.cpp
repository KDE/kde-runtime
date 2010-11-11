/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "backuptests.h"

#include <nepomuk/simpleresource.h>
#include <qtest_kde.h>

#include <KTempDir>
#include <KTemporaryFile>
#include <KDebug>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <Soprano/Model>
#include <Soprano/StatementIterator>

#include <QtDBus/QDBusConnection>
#include <QtTest>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>

BackupTests::BackupTests()
{
    startServiceAndWait("nepomukbackupsync");
    
    m_backupManager = new BackupManager( QLatin1String("org.kde.nepomuk.services.nepomukbackupsync"),
                                         "/backupmanager",
                                         QDBusConnection::sessionBus(), this);

    m_model = Nepomuk::ResourceManager::instance()->mainModel();
}

void BackupTests::basicRatingRetrivalTest()
{
    KTemporaryFile file;
    QUrl oldUri;
    QVERIFY( file.open() );
    {
        Nepomuk::Resource res( file.fileName() );
        res.setRating( 5 );

        oldUri = res.resourceUri();
        kDebug() << res.rating();
    }
    
    backup();
    resetRepository();
    restore();

    Nepomuk::Resource res( file.fileName() );
    kDebug() << res.rating();
    QVERIFY( res.rating() == 5 );
    QVERIFY( oldUri != res.resourceUri() );
}


namespace {
    QList<Soprano::Statement> getStatementList( const QUrl & resUri ) {
        Soprano::Model * model = Nepomuk::ResourceManager::instance()->mainModel();
        return model->listStatements( resUri, Soprano::Node(), Soprano::Node() ).allStatements();
    }
}

void BackupTests::allPropertiesRetrivalTest()
{
    kDebug() << runningServices();
    
    KTemporaryFile file;
    QUrl resourceUri;
    QVERIFY( file.open() );
    {
        Nepomuk::Resource res( file.fileName() );
        res.setRating( 5 );
        resourceUri = res.resourceUri();
        
        kDebug() << res.rating();
    }

    QList<Soprano::Statement> stList1 = getStatementList( resourceUri );
    kDebug() << "Old List: "<< stList1;

    // Backup
    // -------
    backup();
    resetRepository();
    restore();
    // --------

    QString query = "select distinct ?r where { graph ?g { ?r ?p ?o. } . ?g a nrl:InstanceBase. }";
    QList< Soprano::Node > nodeList = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();
    kDebug() << nodeList;
    
    Nepomuk::Resource res( file.fileName() );

    QList<Soprano::Statement> stList2 = getStatementList( res.resourceUri() );
    kDebug() << "New List: " << stList2;

    QVERIFY( res.rating() == 5 );
    
    // Compare all the properties and object in both the lists
    compare( stList1, stList2 );
}

void BackupTests::compare(const QList< Soprano::Statement >& l1, const QList< Soprano::Statement >& l2)
{
    using namespace Nepomuk::Sync;

    SimpleResource res1 = SimpleResource::fromStatementList( l1 );
    SimpleResource res2 = SimpleResource::fromStatementList( l2 );

    kDebug() << res1;
    kDebug() << res2;
    
    // only check the predicates and objects - NOT the subject uri
    bool equal = res1.QHash<KUrl, Soprano::Node>::operator==( res2 );
    QVERIFY( equal );
}


void BackupTests::backup()
{
    QSignalSpy signalSpy( m_backupManager, SIGNAL( backupDone() ) );
    
    m_backupManager->backup( QString() );

    /*
    while( signalSpy.count() != 1 ) {
        // Do nothing ..
        QTest::qWait( 200 );
    }*/

    QTest::qWait( 5000 );
}

void BackupTests::restore()
{
    m_backupManager->restore( QString() );

    // There is no signal which reports when a restore has been completed
    // For now just wait for 5 seconds

    kDebug() << "Restored .. waiting for 10 secs";
    QTest::qWait( 10000 );
}

QTEST_KDEMAIN(BackupTests, NoGUI)