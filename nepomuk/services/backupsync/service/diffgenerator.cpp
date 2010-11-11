/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

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


#include "diffgenerator.h"
#include "backupsync.h"
#include "logstorage.h"
#include "changelog.h"

#include <QtCore/QString>

#include <Soprano/Model>
#include <Soprano/Statement>
#include <Soprano/PluginManager>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NRL>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Types/Property>

#include <KConfig>
#include <KConfigGroup>


Nepomuk::DiffGenerator::DiffGenerator( QObject* parent )
    : QThread(parent)
{
    m_model = Nepomuk::ResourceManager::instance()->mainModel();

    connect( m_model, SIGNAL(statementAdded(Soprano::Statement)),
             this, SLOT(statementAdded(Soprano::Statement)), Qt::QueuedConnection );
    connect( m_model, SIGNAL(statementRemoved(Soprano::Statement)),
             this, SLOT(statementRemoved(Soprano::Statement)), Qt::QueuedConnection );

    start();
}


Nepomuk::DiffGenerator::~DiffGenerator()
{
    stop();
    wait();
}


namespace {
    const Nepomuk::Types::Property identifyingProperty( Nepomuk::Vocabulary::backupsync::identifyingProperty() );
}

bool Nepomuk::DiffGenerator::backupStatement(const Soprano::Statement& st)
{
    // Subject
    const QUrl& subjectUri = st.subject().uri();
    if( !subjectUri.toString().contains("nepomuk:/res/") )
        return false;

    // Property
    // vHanda: Should this be done?
    //Types::Property prop( st.predicate().uri() );
    //if( prop.isSubPropertyOf( identifyingProperty ) )
    //    return true;

    // Context
    const QUrl & contextUri = st.context().uri();
    QHash<QUrl, bool>::const_iterator it = m_discardableGraphs.constFind( contextUri );
    if( it != m_discardableGraphs.constEnd() )
        return it.value();

    QString query = QString("ask { { %1 %2 %3 . } UNION { %1 %2 %4 . } UNION { %1 %2 %5 . } }")
                    .arg( Soprano::Node::resourceToN3( contextUri ),
                          Soprano::Node::resourceToN3( Soprano::Vocabulary::RDF::type() ),
                          Soprano::Node::resourceToN3( Soprano::Vocabulary::NRL::DiscardableInstanceBase() ),
                          Soprano::Node::resourceToN3( Soprano::Vocabulary::NRL::Ontology() ),
                          Soprano::Node::resourceToN3( Soprano::Vocabulary::NRL::GraphMetadata() ) );

    Soprano::Model * model = Nepomuk::ResourceManager::instance()->mainModel();

    // If it is a DiscardableInstanceBase, Ontology or GraphMetadata,
    // then we DO NOT want to back it up.
    bool shouldBackup = !model->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue();

    m_discardableGraphs.insert( st.context().uri(), shouldBackup );

    return shouldBackup;
}

void Nepomuk::DiffGenerator::statementAdded(const Soprano::Statement & st)
{
    QMutexLocker locker( &m_queueMutex );
    m_recordQueue.enqueue( ChangeLogRecord( QDateTime::currentDateTime()/*.toUTC()*/, true, st ) );
    m_queueWaiter.wakeAll();
}

void Nepomuk::DiffGenerator::statementRemoved(const Soprano::Statement & st)
{
    QMutexLocker locker( &m_queueMutex );
    m_recordQueue.enqueue( ChangeLogRecord( QDateTime::currentDateTime()/*.toUTC()*/, false, st ) );
    m_queueWaiter.wakeAll();
}


void Nepomuk::DiffGenerator::stop()
{
    m_stopped = true;
    m_queueWaiter.wakeAll();
}

/*
void Nepomuk::DiffGenerator::removeOldRecords()
{
    KConfig config( "nepomukbackupsyncrc" );
    int days = config.group( "diffgenerator" ).readEntry<int>("days", 300);

    QDateTime date = QDateTime::currentDateTime().toUTC();
    date.addDays( -days );

    ChangeLog::removeRecords( date );
}*/

void Nepomuk::DiffGenerator::run()
{
    m_stopped = false;
    LogStorage * storage = LogStorage::instance();

    while( !m_stopped ) {
        m_queueMutex.lock();
        while( !m_recordQueue.isEmpty() ) {
            // unlock after queue utilization
            m_queueMutex.unlock();

            ChangeLogRecord record = m_recordQueue.dequeue();
            //kDebug() << "Checking .. " << record.st();
            if( backupStatement( record.st() ) ) {
                storage->addRecord( record );
            }

            m_queueMutex.lock();
        }

        if( m_stopped )
            break;

        // wait for more input
        //kDebug() << "Waiting ..";
        m_queueWaiter.wait( &m_queueMutex );
        m_queueMutex.unlock();
        //kDebug() << "Woke up";
    }
}


#include "diffgenerator.moc"

