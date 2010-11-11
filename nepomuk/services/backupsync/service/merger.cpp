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

#include "merger.h"

#include "changelog.h"
#include "syncfile.h"
#include "mergeradaptor.h"

#include <QtDBus/QDBusConnection>

#include <KDebug>

Nepomuk::Merger::Merger( QObject* parent )
    : QThread( parent )
{
    //Register DBus interface
    new MergerAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( QLatin1String("/merger"), this );
    
    start();
}


Nepomuk::Merger::~Merger()
{
    stop();
    wait();
}


void Nepomuk::Merger::stop()
{
    m_stopped = true;
    m_queueWaiter.wakeAll();
}


int Nepomuk::Merger::process(const Nepomuk::ChangeLog& changeLog )
{
    kDebug();
    m_queueMutex.lock();

    kDebug() << "Received ChangeLog -- " << changeLog.size();
    ChangeLogMerger * request = new ChangeLogMerger( changeLog );
    m_queue.enqueue( request );

    m_queueMutex.unlock();
    m_queueWaiter.wakeAll();

    return request->id();
}


void Nepomuk::Merger::run()
{
    m_stopped = false;

    while( !m_stopped ) {

        // lock for initial iteration
        m_queueMutex.lock();

        while( !m_queue.isEmpty() ) {

            ChangeLogMerger* request = m_queue.dequeue();
            //kDebug() << "Processing request #" << request->id() << " with size " << request->size();

            // unlock after queue utilization
            m_queueMutex.unlock();

            //FIXME: Fake completed signals!
            emit completed( 5 );
            request->load();
            emit completed( 55 );
            request->mergeChangeLog();
            emit completed( 100 );
            
            /*
            if( !request->done() ) {
                QMutexLocker lock( &m_processMutex );
                m_processes[ request->id() ] = request;
            }

            disconnect( request, SIGNAL(completed(int)),
                        this, SIGNAL(completed(int)) );
            disconnect( request, SIGNAL(multipleMerge(QString,QString)),
                     this, SIGNAL(multipleMerge(QString,QString)) );

            if( request->done() ){*/

            foreach( const Soprano::Statement & st, request->multipleMergers() ) {
                emit multipleMerge( st.subject().uri().toString(),
                                    st.predicate().uri().toString() );
            }
            
            m_processes.remove( request->id() );
            delete request;

            m_queueMutex.lock();
        }

        kDebug() << "Waiting...";
        m_queueWaiter.wait( &m_queueMutex );
        m_queueMutex.unlock();
        kDebug() << "Woke up.";
    }
}


#include "merger.moc"
