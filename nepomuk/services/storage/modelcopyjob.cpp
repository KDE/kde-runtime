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

#include <Soprano/Model>
#include <Soprano/Error/Error>

#include <KLocale>
#include <KDebug>


Nepomuk::ModelCopyJob::ModelCopyJob( Soprano::Model* source, Soprano::Model* dest, QObject* parent )
    : KJob( parent ),
      m_source( source ),
      m_dest( dest )
{
    kDebug();
    connect( &m_timer, SIGNAL( timeout() ),
             this, SLOT( slotCopy() ) );
}


Nepomuk::ModelCopyJob::~ModelCopyJob()
{
}


void Nepomuk::ModelCopyJob::start()
{
    kDebug();
    emit description( this, i18n( "Converting Nepomuk database" ) );

    m_size = m_source->statementCount();
    m_done = 0;
    m_allCopied = true;

    if ( m_size > 0 ) {
        setTotalAmount( KJob::Files, m_size );
    }

    m_iterator = m_source->listStatements();

    m_timer.start( 0 );
}


void Nepomuk::ModelCopyJob::slotCopy()
{
    if ( m_iterator.next() ) {
        ++m_done;

        if ( m_dest->addStatement( *m_iterator ) != Soprano::Error::ErrorNone ) {
            kDebug() << m_dest->lastError();
            emit warning( this, m_dest->lastError().message() );
            m_allCopied = false;
        }

        setProcessedAmount( KJob::Files, m_done );
    }
    else {
        kDebug() << "done";
        m_timer.stop();

        if ( !m_allCopied ) {
            setError( 1 );
            setErrorText( i18n( "Some data was lost in the conversion." ) );
        }

        emitResult();
    }
}

#include "modelcopyjob.moc"
