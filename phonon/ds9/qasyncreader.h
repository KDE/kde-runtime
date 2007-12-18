/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PHONON_QASYNCREADER_H
#define PHONON_QASYNCREADER_H

#include <QtCore/QWaitCondition>
#include <QtCore/QQueue>

#include "qpin.h"

#include <phonon/streaminterface.h>
#include <phonon/mediasource.h>

class CAsyncStream;
namespace Phonon
{
    namespace DS9
    {
        //his class reads asynchronously from a QIODevice
        class QAsyncReader : public QPin, public IAsyncReader, public Phonon::StreamInterface
        {
        public:
            QAsyncReader(QBaseFilter *, const Phonon::MediaSource &source);
            ~QAsyncReader();

            //reimplementation from IUnknown
            STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
            STDMETHODIMP_(ULONG) AddRef();
            STDMETHODIMP_(ULONG) Release();

            //reimplementation from IAsyncReader
            STDMETHODIMP RequestAllocator(IMemAllocator *,ALLOCATOR_PROPERTIES *,IMemAllocator **);
            STDMETHODIMP Request(IMediaSample *,DWORD_PTR);
            STDMETHODIMP WaitForNext(DWORD,IMediaSample **,DWORD_PTR *);
            STDMETHODIMP SyncReadAligned(IMediaSample *);
            STDMETHODIMP SyncRead(LONGLONG,LONG,BYTE *);
            STDMETHODIMP Length(LONGLONG *,LONGLONG *);
            STDMETHODIMP BeginFlush();
            STDMETHODIMP EndFlush();

            //for Phonon::StreamInterface
            void writeData(const QByteArray &data);
            void endOfData();
            void setStreamSize(qint64 newSize);
            void setStreamSeekable(bool s);

        private:
            class AsyncRequest
            {
            public:
                AsyncRequest(IMediaSample *s = 0, DWORD_PTR u = 0) : sample(s), user(u) {}
                IMediaSample *sample;
                DWORD_PTR user;
            };
            HRESULT actualSyncRead(LONGLONG pos, LONG length, BYTE *buffer, LONG *actual);
            qint64 currentPos() const;
            void setCurrentPos(qint64 newPos);
            int currentBufferSize() const;
            AsyncRequest getNextRequest();
            qint64 streamSize() const;
            bool streamSeekable() const;

            Phonon::MediaSource m_source;

            QMutex m_mutexRead,
                m_mutexWait;

            QQueue<AsyncRequest> m_requestQueue;
            QWaitCondition m_requestWait;

            //for Phonon::StreamInterface
            QByteArray m_buffer;
            bool m_seekable;
            qint64 m_pos;
            qint64 m_size;
        };
    }
}

#endif
