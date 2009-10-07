/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "modelcopyjob.h"

#include <Soprano/StorageModel>
#include <Soprano/Backend>
#include <Soprano/Error/Error>

#include <KLocale>
#include <KDebug>
#include <kuiserverjobtracker.h>

#include <QtCore/QThread>
#include <QtCore/QTimer>


class Nepomuk::ModelCopyJob::Private : public QThread
{
public:
    void run();

    Soprano::Model* m_source;
    Soprano::Model* m_dest;

    bool m_allCopied;
    bool m_stopped;

    KUiServerJobTracker* m_jobTracker;

    ModelCopyJob* q;
};


void Nepomuk::ModelCopyJob::Private::run()
{
    m_stopped = false;
    unsigned long size = m_source->statementCount();
    unsigned long done = 0;
    kDebug() << "Need to copy" << size << "statements.";

    Soprano::StatementIterator it = m_source->listStatements();

    while ( !m_stopped ) {
        if ( it.next() ) {
            ++done;

            if ( m_dest->addStatement( *it ) != Soprano::Error::ErrorNone ) {
                kDebug() << m_dest->lastError();
                q->setErrorText( m_dest->lastError().message() );
                break;
            }

            // progress
            if ( size > 0 ) {
                // emitPercent does only emit a signal if the percent value actually changes
                q->emitPercent( done, size );
            }
        }
        else if ( it.lastError() ) {
            q->setErrorText( it.lastError().message() );
        }
    }
}


Nepomuk::ModelCopyJob::ModelCopyJob( Soprano::Model* source, Soprano::Model* dest, QObject* parent )
    : KJob( parent ),
      d( new Private() )
{
    kDebug();

    d->q = this;
    d->m_source = source;
    d->m_dest = dest;

    setCapabilities( KJob::Killable );

    d->m_jobTracker = new KUiServerJobTracker();
    d->m_jobTracker->registerJob( this );

    connect( d, SIGNAL( finished() ),
             this, SLOT( slotThreadFinished() ) );
}


Nepomuk::ModelCopyJob::~ModelCopyJob()
{
    if ( d->isRunning() ) {
        kill();
    }

    d->m_jobTracker->deleteLater();
}


Soprano::Model* Nepomuk::ModelCopyJob::source() const
{
    return d->m_source;
}


Soprano::Model* Nepomuk::ModelCopyJob::dest() const
{
    return d->m_dest;
}


void Nepomuk::ModelCopyJob::start()
{
    kDebug();
    emit description( this,
                      i18nc( "@title job", "Converting Nepomuk database" ),
                      qMakePair( i18n( "Old backend" ), qobject_cast<Soprano::StorageModel*>( d->m_source )->backend()->pluginName() ),
                      qMakePair( i18n( "New backend" ), qobject_cast<Soprano::StorageModel*>( d->m_dest )->backend()->pluginName() ) );
    d->start();
}


void Nepomuk::ModelCopyJob::slotThreadFinished()
{
    if ( !d->m_stopped ) {
        emitResult();
    }
}


bool Nepomuk::ModelCopyJob::doKill()
{
    if ( d->isRunning() ) {
        d->m_stopped = true;
        d->wait();
        return true;
    }
    else {
        return false;
    }
}

#include "modelcopyjob.moc"
