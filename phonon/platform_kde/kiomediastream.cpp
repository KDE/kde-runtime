/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "kiomediastream.h"
#include "kiomediastream_p.h"
#include <kdebug.h>
#include <kprotocolmanager.h>
#include <kio/filejob.h>
#include <kio/job.h>
#include <klocale.h>

namespace Phonon
{

KioMediaStream::KioMediaStream(const QUrl &url, QObject *parent)
    : AbstractMediaStream(parent),
    d_ptr(new KioMediaStreamPrivate(url))
{
    d_ptr->q_ptr = this;
    kDebug(600);
    reset();
}

void KioMediaStream::reset()
{
    kDebug(600);
    Q_D(KioMediaStream);
    if (d->kiojob) {
        d->kiojob->disconnect(this);
        d->kiojob->kill();

        d->endOfDataSent = false;
        d->seeking = false;
        d->reading = false;
        d->open = false;
        d->seekPosition = 0;
    }

    if (KProtocolManager::supportsOpening(d->url)) {
        d->kiojob = KIO::open(d->url, QIODevice::ReadOnly);
        Q_ASSERT(d->kiojob);
        d->open = false;
        setStreamSeekable(true);
        connect(d->kiojob, SIGNAL(open(KIO::Job *)), this, SLOT(_k_bytestreamFileJobOpen(KIO::Job *)));
        connect(d->kiojob, SIGNAL(position(KIO::Job *, KIO::filesize_t)),
                this, SLOT(_k_bytestreamSeekDone(KIO::Job *, KIO::filesize_t)));
    } else {
        d->kiojob = KIO::get(d->url, KIO::NoReload, KIO::HideProgressInfo);
        Q_ASSERT(d->kiojob);
        setStreamSeekable(false);
        connect(d->kiojob, SIGNAL(totalSize(KJob *, qulonglong)),
                this, SLOT(_k_bytestreamTotalSize(KJob *,qulonglong)));
        d->kiojob->suspend();
    }

    d->kiojob->addMetaData("UserAgent", QLatin1String("KDE Phonon"));
    connect(d->kiojob, SIGNAL(data(KIO::Job *,const QByteArray &)),
            this, SLOT(_k_bytestreamData(KIO::Job *,const QByteArray &)));
    connect(d->kiojob, SIGNAL(result(KJob *)), this, SLOT(_k_bytestreamResult(KJob *)));
}

KioMediaStream::~KioMediaStream()
{
    kDebug(600);
    Q_D(KioMediaStream);
    if (d->kiojob) {
        d->kiojob->disconnect(this);
        KIO::FileJob *filejob = qobject_cast<KIO::FileJob *>(d->kiojob);
        if (filejob) {
            filejob->close();
        }
        d->kiojob->kill();
    }
    delete d_ptr;
}

void KioMediaStream::needData()
{
    Q_D(KioMediaStream);
    if (!d->kiojob) {
        // no job => job is finished and endOfData was already sent
        return;
    }
    KIO::FileJob *filejob = qobject_cast<KIO::FileJob *>(d->kiojob);
    if (filejob) {
        // while d->seeking the backend won't get any data
        if (d->seeking || !d->open) {
            d->reading = true;
        } else if (!d->reading) {
            d->reading = true;
            QMetaObject::invokeMethod(this, "_k_read", Qt::QueuedConnection);
            //filejob->read(32768);
        }
    } else {
        // KIO::TransferJob
        d->kiojob->resume();
    }
}

void KioMediaStream::enoughData()
{
    Q_D(KioMediaStream);
    kDebug(600);
    // Don't suspend when using a FileJob. The FileJob is controlled by calls to
    // FileJob::read()
    if (d->kiojob && !qobject_cast<KIO::FileJob *>(d->kiojob) && !d->kiojob->isSuspended()) {
        d->kiojob->suspend();
    } else {
        d->reading = false;
    }
}

void KioMediaStream::seekStream(qint64 position)
{
    Q_D(KioMediaStream);
    if (!d->kiojob || d->endOfDataSent) {
        // no job => job is finished and endOfData was already sent
        kDebug(600) << "no job/job finished -> recreate it";
        reset();
    }
    Q_ASSERT(d->kiojob);
    kDebug(600) << position << " = " << qulonglong(position);
    d->seeking = true;
    d->seekPosition = position;
    if (d->open) {
        KIO::FileJob *filejob = qobject_cast<KIO::FileJob *>(d->kiojob);
        filejob->seek(position);
    }
}

void KioMediaStreamPrivate::_k_bytestreamData(KIO::Job *, const QByteArray &data)
{
    Q_Q(KioMediaStream);
    Q_ASSERT(kiojob);
    if (q->streamSize() == 0) {
        q->setStreamSize(-1);
    }
    if (seeking) {
        // seek doesn't block, so don't send data to the backend until it signals us
        // that the seek is done
        kDebug(600) << "seeking: do nothing";
        return;
    }

    if (data.isEmpty()) {
        reading = false;
        if (!endOfDataSent) {
            kDebug(600) << "empty data: stopping the stream";
            endOfDataSent = true;
            q->endOfData();
        }
        return;
    }

    //kDebug(600) << "calling writeData on the Backend ByteStream " << data.size();
    q->writeData(data);
    if (reading) {
        Q_ASSERT(qobject_cast<KIO::FileJob *>(kiojob));
        QMetaObject::invokeMethod(q, "_k_read", Qt::QueuedConnection);
    }
}

void KioMediaStreamPrivate::_k_bytestreamResult(KJob *job)
{
    Q_Q(KioMediaStream);
    Q_ASSERT(kiojob == job);
    if (job->error()) {
        QString kioErrorString = job->errorString();
        kDebug(600) << "KIO Job error: " << kioErrorString;
        QObject::disconnect(kiojob, SIGNAL(data(KIO::Job *,const QByteArray &)),
                q, SLOT(_k_bytestreamData(KIO::Job *,const QByteArray &)));
        QObject::disconnect(kiojob, SIGNAL(result(KJob *)),
                q, SLOT(_k_bytestreamResult(KJob *)));
        KIO::FileJob *filejob = qobject_cast<KIO::FileJob *>(kiojob);
        if (filejob) {
            QObject::disconnect(kiojob, SIGNAL(open(KIO::Job *)),
                    q, SLOT(_k_bytestreamFileJobOpen(KIO::Job *)));
            QObject::disconnect(kiojob, SIGNAL(position(KIO::Job *, KIO::filesize_t)),
                    q, SLOT(_k_bytestreamSeekDone(KIO::Job *, KIO::filesize_t)));
        } else {
            QObject::disconnect(kiojob, SIGNAL(totalSize(KJob *, qulonglong)),
                    q, SLOT(_k_bytestreamTotalSize(KJob *,qulonglong)));
        }
        // go to ErrorState - NormalError
        q->error(NormalError, kioErrorString);
    } else if (seeking) {
        open = false;
        kiojob = 0;
        endOfDataSent = false;
        reading = false;
        q->reset();
        return;
    }
    open = false;
    kiojob = 0;
    kDebug(600) << "KIO Job is done (will delete itself) and d->kiojob reset to 0";
    endOfDataSent = true;
    q->endOfData();
    reading = false;
}

void KioMediaStreamPrivate::_k_bytestreamTotalSize(KJob *, qulonglong size)
{
    Q_Q(KioMediaStream);
    kDebug(600) << size;
    q->setStreamSize(size > 0 ? size : -1);
}

void KioMediaStreamPrivate::_k_bytestreamFileJobOpen(KIO::Job *)
{
    Q_Q(KioMediaStream);
    Q_ASSERT(kiojob);
    open = true;
    endOfDataSent = false;
    KIO::FileJob *filejob = static_cast<KIO::FileJob *>(kiojob);
    kDebug(600) << filejob->size();
    q->setStreamSize(filejob->size() > 0 ? filejob->size() : -1);

    if (seeking) {
        filejob->seek(seekPosition);
    } else if (reading) {
        //filejob->read(32768);
        QMetaObject::invokeMethod(q, "_k_read", Qt::QueuedConnection);
    }
}

void KioMediaStreamPrivate::_k_bytestreamSeekDone(KIO::Job *, KIO::filesize_t offset)
{
    Q_ASSERT(kiojob);
    kDebug(600) << offset;
    seeking = false;
    endOfDataSent = false;
    if (reading) {
        Q_Q(KioMediaStream);
        Q_ASSERT(qobject_cast<KIO::FileJob *>(kiojob));
        QMetaObject::invokeMethod(q, "_k_read", Qt::QueuedConnection);
    }
}

void KioMediaStreamPrivate::_k_read()
{
    KIO::FileJob *filejob = qobject_cast<KIO::FileJob *>(kiojob);
    Q_ASSERT(filejob);
    filejob->read(32768);
}

} // namespace Phonon

#include "kiomediastream.moc"
// vim: sw=4 sts=4 et tw=100
