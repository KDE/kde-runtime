/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

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

#include "bytestream.h"
#include <kdebug.h>
#include <klocale.h>

#include "xineengine.h"
#include <QEvent>
#include <QTimer>
#include <cstring>
#include <cstdio>
#include <unistd.h>

extern "C" {
#define this this_xine
#include <xine/input_plugin.h> // needed for MAX_PREVIEW_SIZE
#include <xine/xine_internal.h>
#undef this
}

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#  define PXINE_VDEBUG kDebug( 610 )
#else
#  define PXINE_VDEBUG kndDebug()
#endif
#define PXINE_DEBUG kDebug( 610 )

namespace Phonon
{
namespace Xine
{
ByteStream::ByteStream(const MediaSource &mediaSource, MediaObject* parent)
    : QObject(0), // don't let MediaObject's ~QObject delete us - the input plugin will delete us
    m_mediaObject(parent),
    m_streamSize(0),
    m_currentPosition(0),
    m_buffersize(0),
    m_offset(0),
    m_seekable(false),
    m_mrlSet(false),
    m_stopped(false),
    m_eod(false),
    m_buffering(false),
    m_playRequested(false)
{
    connect(this, SIGNAL(needDataQueued()), this, SLOT(needData()), Qt::QueuedConnection);
    connect(this, SIGNAL(seekStreamQueued(qint64)), this, SLOT(syncSeekStream(qint64)), Qt::QueuedConnection);

    connectToSource(mediaSource);

    // created in the main thread
    //m_mainThread = pthread_self();
}

//X void ByteStream::setMrl()
//X {
//X     if (m_mrlSet || m_streamSize <= 0 || m_preview.size() < MAX_PREVIEW_SIZE) {
//X         return;
//X     }
//X     m_mrlSet = true;
//X     stream().setMrl(mrl());
//X //X     if (m_playRequested) {
//X //X         m_playRequested = false;
//X //X         MediaObject::play();
//X //X     }
//X }

void ByteStream::pullBuffer(char *buf, int len)
{
    if (m_stopped) {
        return;
    }
    // never called from main thread
    //Q_ASSERT(m_mainThread != pthread_self());

    PXINE_VDEBUG << k_funcinfo << len << ", m_offset = " << m_offset << ", m_currentPosition = "
        << m_currentPosition << ", m_buffersize = " << m_buffersize << endl;
    while (len > 0) {
        if (m_buffers.isEmpty()) {
            // pullBuffer is only called when there's => len data available
            kFatal(610) << k_funcinfo << "m_currentPosition = " << m_currentPosition
                << ", m_preview.size() = " << m_preview.size() << ", len = " << len << kBacktrace()
                << endl;
        }
        if (m_buffers.head().size() - m_offset <= len) {
            // The whole data of the next buffer is needed
            QByteArray buffer = m_buffers.dequeue();
            PXINE_VDEBUG << k_funcinfo << "dequeue one buffer of size " << buffer.size()
                << ", reading at offset = " << m_offset << ", resetting m_offset to 0" << endl;
            Q_ASSERT(buffer.size() > 0);
            int tocopy = buffer.size() - m_offset;
            Q_ASSERT(tocopy > 0);
            xine_fast_memcpy(buf, buffer.constData() + m_offset, tocopy);
            buf += tocopy;
            len -= tocopy;
            Q_ASSERT(len >= 0);
            Q_ASSERT(m_buffersize >= static_cast<size_t>(tocopy));
            m_buffersize -= tocopy;
            m_offset = 0;
        } else {
            // only a part of the next buffer is needed
            PXINE_VDEBUG << k_funcinfo << "read " << len
                << " bytes from the first buffer at offset = " << m_offset << endl;
            QByteArray &buffer = m_buffers.head();
            Q_ASSERT(buffer.size() > 0);
            xine_fast_memcpy(buf, buffer.constData() + m_offset , len);
            m_offset += len;
            Q_ASSERT(m_buffersize >= static_cast<size_t>(len));
            m_buffersize -= len;
            len = 0;
        }
    }
}

int ByteStream::peekBuffer(void *buf)
{
    if (m_stopped) {
        return 0;
    }

    // never called from main thread
    //Q_ASSERT(m_mainThread != pthread_self());

    if (m_preview.size() < MAX_PREVIEW_SIZE && !m_eod) {
        QMutexLocker lock(&m_mutex);
        // the thread needs to sleep until a wait condition is signalled from writeData
        while (!m_eod && !m_stopped && m_preview.size() < MAX_PREVIEW_SIZE) {
            PXINE_VDEBUG << k_funcinfo << "xine waits for data: " << m_buffersize << ", " << m_eod << endl;
            emit needDataQueued();
            m_waitingForData.wait(&m_mutex);
        }
        if (m_stopped) {
            PXINE_DEBUG << k_funcinfo << "returning 0, m_stopped = true" << endl;
            //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
            return 0;
        }
    }

    xine_fast_memcpy(buf, m_preview.constData(), m_preview.size());
    return m_preview.size();
}

qint64 ByteStream::readFromBuffer(void *buf, size_t count)
{
    if (m_stopped) {
        return 0;
    }
    // never called from main thread
    //Q_ASSERT(m_mainThread != pthread_self());

    qint64 currentPosition = m_currentPosition;

    PXINE_VDEBUG << k_funcinfo << count << endl;

    QMutexLocker lock(&m_mutex);
    //kDebug(610) << "LOCKED m_mutex: " << k_funcinfo;
    // get data while more is needed and while we're still receiving data
    if (m_buffersize < count && !m_eod) {
        // the thread needs to sleep until a wait condition is signalled from writeData
        while (!m_eod && !m_stopped && m_buffersize < count) {
            PXINE_VDEBUG << k_funcinfo << "xine waits for data: " << m_buffersize << ", " << m_eod << endl;
            emit needDataQueued();
            m_waitingForData.wait(&m_mutex);
        }
        if (m_stopped) {
            PXINE_DEBUG << k_funcinfo << "returning 0, m_stopped = true" << endl;
            //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
            return 0;
        }
        Q_ASSERT(currentPosition == m_currentPosition);
        //PXINE_VDEBUG << "m_buffersize = " << m_buffersize << endl;
    }
    if (m_buffersize >= count) {
        PXINE_VDEBUG << k_funcinfo << "calling pullBuffer with m_buffersize = " << m_buffersize << endl;
        pullBuffer(static_cast<char*>(buf), count);
        m_currentPosition += count;
        //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
        return count;
    }
    Q_ASSERT(m_eod);
    if (m_buffersize > 0) {
        PXINE_VDEBUG << k_funcinfo << "calling pullBuffer with m_buffersize = " << m_buffersize << endl;
        pullBuffer(static_cast<char*>(buf), m_buffersize);
        m_currentPosition += m_buffersize;
        PXINE_DEBUG << k_funcinfo << "returning less data than requested, the stream is at its end" << endl;
        //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
        return m_buffersize;
    }
    PXINE_DEBUG << k_funcinfo << "return 0, the stream is at its end" << endl;
    //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
    return 0;
}

off_t ByteStream::seekBuffer(qint64 offset)
{
    if (m_stopped) {
        return 0;
    }
    // never called from main thread
    //Q_ASSERT(m_mainThread != pthread_self());

    // no seek
    if (offset == m_currentPosition) {
        return m_currentPosition;
    }

    // impossible seek
    if (offset > m_streamSize) {
        kWarning(610) << "xine is asking to seek behind the end of the data stream";
        return m_currentPosition;
    }

    // first try to seek in the data we have buffered
    m_mutex.lock();
    //kDebug(610) << "LOCKED m_mutex: " << k_funcinfo;
    if (offset > m_currentPosition && offset < m_currentPosition + m_buffersize) {
        kDebug(610) << "seeking behind current position, but inside the buffered data";
        // seek behind the current position in the buffer
        while( offset > m_currentPosition ) {
            const int gap = offset - m_currentPosition;
            Q_ASSERT(!m_buffers.isEmpty());
            const int buffersize = m_buffers.head().size() - m_offset;
            if (buffersize <= gap) {
                // discard buffers if they hold data before offset
                Q_ASSERT(!m_buffers.isEmpty());
                QByteArray buffer = m_buffers.dequeue();
                m_buffersize -= buffersize;
                m_currentPosition += buffersize;
                m_offset = 0;
            } else {
                // offset points to data in the next buffer
                m_buffersize -= gap;
                m_currentPosition += gap;
                m_offset += gap;
            }
        }
        Q_ASSERT( offset == m_currentPosition );
        //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
        m_mutex.unlock();
        return m_currentPosition;
    } else if (offset < m_currentPosition && m_currentPosition - offset <= m_offset) {
        kDebug(610) << "seeking in current buffer: m_currentPosition = " << m_currentPosition << ", m_offset = " << m_offset;
        // seek before the current position in the buffer
        m_offset -= m_currentPosition - offset;
        m_buffersize += m_currentPosition - offset;
        Q_ASSERT(m_offset >= 0);
        m_currentPosition = offset;
        //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
        m_mutex.unlock();
        return m_currentPosition;
    }

    // the ByteStream is not seekable: no chance to seek to the requested offset
    if (!m_seekable) {
        //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
        m_mutex.unlock();
        return m_currentPosition;
    }

    PXINE_DEBUG << "seeking to a position that's not in the buffered data: clear the buffer. "
        " new offset = " << offset <<
        ", m_buffersize = " << m_buffersize <<
        ", m_offset = " << m_offset <<
        ", m_eod = " << m_eod <<
        ", m_currentPosition = " << m_currentPosition <<
        endl;

    // throw away the buffers and ask for new data
    m_buffers.clear();
    m_buffersize = 0;
    m_offset = 0;
    m_eod = false;

    m_currentPosition = offset;
    //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
    m_mutex.unlock();

    QMutexLocker seekLock(&m_seekMutex);
    if (m_stopped) {
        return 0;
    }
    emit seekStreamQueued(offset); //calls syncSeekStream from the main thread
    m_seekWaitCondition.wait(&m_seekMutex); // waits until the seekStream signal returns
    return offset;
}

off_t ByteStream::currentPosition() const
{
    return m_currentPosition;
}

ByteStream::~ByteStream()
{
    PXINE_DEBUG << k_funcinfo << endl;
}

QByteArray ByteStream::mrl() const
{
    QByteArray mrl("kbytestream:/");
    // the address can contain 0s which will null-terminate the C-string
    // use a simple encoding: 0x00 -> 0x0101, 0x01 -> 0x0102
    const ByteStream *iface = this;
    const unsigned char *that = reinterpret_cast<const unsigned char *>(&iface);
    for(unsigned int i = 0; i < sizeof(void *); ++i) {
        switch (that[i]) {
        case 0: // escape 0 as it terminates the string
            mrl += 0x01;
            mrl += 0x01;
            break;
        case 1: // escape 1 because it is used for escaping
            mrl += 0x01;
            mrl += 0x02;
            break;
        case '#': // escape # because xine splits the mrl at #s
            mrl += 0x01;
            mrl += 0x03;
            break;
        case '%': // escape % because xine will replace e.g. %20 with ' '
            mrl += 0x01;
            mrl += 0x04;
            break;
        default:
            mrl += that[i];
        }
    }
    mrl += '\0';
    return mrl;
}

void ByteStream::setStreamSize(qint64 x)
{
    PXINE_VDEBUG << k_funcinfo << x << endl;
    QMutexLocker lock(&m_streamSizeMutex);
    m_streamSize = x;
    if (m_streamSize != 0) {
        emit needDataQueued();
        m_waitForStreamSize.wakeAll();
//X         setMrl();
    }
}

void ByteStream::setPauseForBuffering(bool b)
{
    if (b) {
        QCoreApplication::postEvent(&m_mediaObject->stream(), new QEvent(static_cast<QEvent::Type>(2019)));
        m_buffering = true;
    } else {
        QCoreApplication::postEvent(&m_mediaObject->stream(), new QEvent(static_cast<QEvent::Type>(2020)));
        m_buffering = false;
    }
}
void ByteStream::endOfData()
{
    PXINE_VDEBUG << k_funcinfo << endl;
    QMutexLocker lock(&m_mutex);
    m_eod = true;
    // don't reset the XineStream because many demuxers hit eod while trying to find the format of
    // the data
    // stream().setMrl(mrl());
    m_waitingForData.wakeAll();
}

void ByteStream::setStreamSeekable(bool seekable)
{
    m_seekable = seekable;
}

//X bool ByteStream::isSeekable() const
//X {
//X     if (m_seekable) {
//X         return MediaObject::isSeekable();
//X     }
//X     return false;
//X }

void ByteStream::writeData(const QByteArray &data)
{
    if (data.size() <= 0) {
        return;
    }

    // first fill the preview buffer
    if (m_preview.size() != MAX_PREVIEW_SIZE) {
        PXINE_DEBUG << k_funcinfo << "fill preview" << endl;
        // more data than the preview buffer needs
        if (m_preview.size() + data.size() > MAX_PREVIEW_SIZE) {
            int tocopy = MAX_PREVIEW_SIZE - m_preview.size();
            m_preview += data.left(tocopy);
        } else { // all data fits into the preview buffer
            m_preview += data;
        }

        PXINE_VDEBUG << k_funcinfo << "filled preview buffer to " << m_preview.size() << endl;
//X         if (m_preview.size() == MAX_PREVIEW_SIZE) { // preview buffer is full
//X             setMrl();
//X         }
    }

    PXINE_VDEBUG << k_funcinfo << data.size() << " m_streamSize = " << m_streamSize << endl;

    QMutexLocker lock(&m_mutex);
    //kDebug(610) << "LOCKED m_mutex: " << k_funcinfo;
    m_buffers.enqueue(data);
    m_buffersize += data.size();
    PXINE_VDEBUG << k_funcinfo << "m_buffersize = " << m_buffersize << endl;
    switch (m_mediaObject->state()) {
    case Phonon::BufferingState: // if nbc is buffering we want more data
    case Phonon::LoadingState: // if the preview is not ready we want me more data
        break;
    default:
        enoughData(); // else it's enough
    }
    m_waitingForData.wakeAll();
    //kDebug(610) << "UNLOCKING m_mutex: " << k_funcinfo;
}

void ByteStream::syncSeekStream(qint64 offset)
{
    PXINE_VDEBUG << k_funcinfo << endl;
    m_seekMutex.lock();
    seekStream(offset);
    m_seekWaitCondition.wakeAll();
    m_seekMutex.unlock();
}

//X void ByteStream::play()
//X {
//X     PXINE_VDEBUG << k_funcinfo << endl;
//X     m_stopped = false;
//X     if (m_streamSize <= 0 || m_preview.size() < MAX_PREVIEW_SIZE) {
//X         m_playRequested = true; // not ready yet for playback, but keep it in mind so playback can
//X         // start when ready
//X         return;
//X     }
//X     setMrl();
//X     MediaObject::play(); // goes into Phonon::BufferingState/PlayingState
//X }

qint64 ByteStream::streamSize() const
{
    if (m_streamSize == 0) {
        // stream size has not been set yet
        QMutexLocker lock(&m_streamSizeMutex);
        if (m_streamSize == 0) {
            m_waitForStreamSize.wait(&m_streamSizeMutex);
        }
    }
    return m_streamSize;
}

void ByteStream::stop()
{
    PXINE_VDEBUG << k_funcinfo << endl;

    m_mutex.lock();
    m_seekMutex.lock();
    m_streamSizeMutex.lock();
    m_stopped = true;
    // the other thread is now not between m_mutex.lock() and m_waitingForData.wait(&m_mutex), so it
    // won't get stuck in m_waitingForData.wait if it's not there right now
    m_seekWaitCondition.wakeAll();
    m_seekMutex.unlock();
    m_waitingForData.wakeAll();
    m_mutex.unlock();
    m_waitForStreamSize.wakeAll();
    m_streamSizeMutex.unlock();
}

}} //namespace Phonon::Xine

#include "bytestream.moc"
// vim: sw=4 ts=4
