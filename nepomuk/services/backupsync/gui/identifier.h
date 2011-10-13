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

#ifndef IDENTIFICATIONTHREAD_H
#define IDENTIFICATIONTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QQueue>
#include <QtCore/QUrl>

#include "syncfileidentifier.h"

namespace Soprano {
    class Model;
}

namespace Nepomuk {

    class SyncFile;
    class ChangeLog;

    class Identifier : public QThread
    {
        Q_OBJECT

        Identifier( QObject* parent = 0 );
        virtual ~Identifier();

        void stop();
        void test();

    public :
        static Identifier* instance();

    Q_SIGNALS:
        void identified( int id, const QString & oldUri, const QString & newUri );
        void notIdentified( int id, const QString & serializedStatements );

        void identificationDone( int id, int unidentified );
        void processed( const Nepomuk::ChangeLog & logFile );

        void completed( int id, int progress );
    public Q_SLOTS:
        int process( const SyncFile & sf );

        /**
         * Add the (oldUri, newUri) pair to the identifier list
         * @param oldUri has to be a nepomuk:/res/
         * @param newUri can be a nepomuk:/res/ or file:/
         */
        bool identify( int id, const QString & oldUri, const QString & newUri );

        bool ignore(int id, const QString& url, bool ignoreSubDirectories);

        void ignoreAll( int id );

        void completeIdentification( int id );

    protected:
        virtual void run();

    private :
        QQueue<SyncFileIdentifier *> m_queue;
        QMutex m_queueMutex;
        QWaitCondition m_queueWaiter;

        bool m_stopped;

        QHash< int, SyncFileIdentifier *> m_processes;
        QMutex m_processMutex;

        void emitNotIdentified( int id, const QList<Soprano::Statement> & stList );
        void identifyAllWithCompletedSignals( SyncFileIdentifier * ident );
    };

}
#endif // IDENTIFICATIONTHREAD_H
