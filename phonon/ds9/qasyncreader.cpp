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

#include <QtCore/QFile>

#include "qasyncreader.h"
#include "qbasefilter.h"

namespace Phonon
{
    namespace DS9
    {
        //this mediatype defines a stream, its type will be autodetecteed by DirectShow

		static QVector<AM_MEDIA_TYPE> getMediaTypes()
		{
			AM_MEDIA_TYPE mt;
			mt.majortype = MEDIATYPE_Stream;
			mt.bFixedSizeSamples = TRUE;
			mt.bTemporalCompression = FALSE;
			mt.lSampleSize = 1;
			mt.formattype = GUID_NULL;
			mt.pUnk = 0;
			mt.cbFormat = 0;
			mt.pbFormat = 0;

			QVector<AM_MEDIA_TYPE> ret;
			//normal auto-detect stream
			mt.subtype = MEDIASUBTYPE_NULL;
			ret << mt;
			//AVI stream
			mt.subtype = MEDIASUBTYPE_Avi;
			ret << mt;
			//WAVE stream
			mt.subtype = MEDIASUBTYPE_WAVE;
			ret << mt;
			return ret;
		}

        QAsyncReader::QAsyncReader(QBaseFilter *parent, const Phonon::MediaSource &source) :
            QPin(parent, PINDIR_OUTPUT, getMediaTypes()),
                m_source(source), m_seekable(false), m_pos(0), m_size(-1)
        {
            connectToSource(source);
        }

        QAsyncReader::~QAsyncReader()
        {
        }

        STDMETHODIMP QAsyncReader::QueryInterface(REFIID iid, void **out)
        {
            if (!out) {
                return E_POINTER;
            }

            if (iid == IID_IAsyncReader) {
                AddRef();
                *out = static_cast<IAsyncReader*>(this);
                return S_OK;
            }

            return QPin::QueryInterface(iid, out);
        }

        STDMETHODIMP_(ULONG) QAsyncReader::AddRef()
        {
            return QPin::AddRef();
        }

        STDMETHODIMP_(ULONG) QAsyncReader::Release()
        {
            return QPin::Release();
        }


        STDMETHODIMP QAsyncReader::RequestAllocator(IMemAllocator *preferred, ALLOCATOR_PROPERTIES *prop,IMemAllocator **actual)
        {
            ALLOCATOR_PROPERTIES prop2;

            if (prop->cbAlign == 0) {
                prop->cbAlign = 1; //align on 1 char
            }

            if (preferred && preferred->SetProperties(prop, &prop2) == S_OK) {
                preferred->AddRef();
                *actual = preferred;
                return S_OK;
            }

            //we should try to create one memory allocator ourselves here
            return E_FAIL;
        }

        STDMETHODIMP QAsyncReader::Request(IMediaSample *sample,DWORD_PTR user)
        {
            if (isFlushing()) {
                return VFW_E_WRONG_STATE;
            }

            QMutexLocker locker(&m_mutex);
            m_requestQueue.enqueue(AsyncRequest(sample, user));
            m_requestWait.wakeOne();

            return S_OK;
        }

        STDMETHODIMP QAsyncReader::WaitForNext(DWORD timeout, IMediaSample **sample, DWORD_PTR *user)
        {
            if (!sample ||!user) {
                return E_POINTER;
            }

            *sample = 0;
            *user = 0;

            AsyncRequest r = getNextRequest();

            if (r.sample == 0) {
                //there is no request in the queue
                if (isFlushing()) {
                    return VFW_E_WRONG_STATE;
                } else {
                    //First we need to lock the mutex
                    QMutexLocker locker(&m_mutexWait);
                    if (m_requestWait.wait(&m_mutexWait, timeout) == false) {
                        return VFW_E_TIMEOUT;
                    }
                    r = getNextRequest();
                }
            }

            //at this point we're sure to have a request to proceed
            if (r.sample == 0) {
                return E_FAIL;
            }

            *sample = r.sample;
            *user = r.user;

            return SyncReadAligned(r.sample);
        }

        STDMETHODIMP QAsyncReader::BeginFlush()
        {
            QPin::BeginFlush();
            m_requestWait.wakeOne();
            return S_OK;
        }

        STDMETHODIMP QAsyncReader::EndFlush()
        {
            QPin::EndFlush();
            return S_OK;
        }

        STDMETHODIMP QAsyncReader::SyncReadAligned(IMediaSample *sample)
        {
            if (!sample) {
                return E_POINTER;
            }

            REFERENCE_TIME start = 0,
                stop = 0;
            HRESULT hr = sample->GetTime(&start, &stop);
            if(FAILED(hr)) {
                return hr;
            }

            LONGLONG startPos = start / 10000000;
            LONG length = static_cast<LONG>((stop - start) / 10000000);

            BYTE *buffer;
            hr = sample->GetPointer(&buffer);
            if(FAILED(hr)) {
                return hr;
            }

            LONG actual = 0;
            actualSyncRead(startPos, length, buffer, &actual);

            return sample->SetActualDataLength(actual);
        }

        STDMETHODIMP QAsyncReader::SyncRead(LONGLONG pos, LONG length, BYTE *buffer)
        {
            return actualSyncRead(pos, length, buffer, 0);
        }

        HRESULT QAsyncReader::actualSyncRead(LONGLONG pos, LONG length, BYTE *buffer, LONG *actual)
        {
            QMutexLocker locker(&m_mutex);

            if(m_size !=1 && pos + length > m_size) {
                //it tries to read outside of the boundaries
                return E_FAIL;
            }

            if (m_pos - m_buffer.size() != pos) {
                if (!m_seekable) {
                    return S_FALSE;
                }
                m_pos = pos;
                seekStream(pos);
                m_buffer.clear();
            }

            int oldSize = m_buffer.size();
            while (m_buffer.size() < int(length)) {
                needData();
                if (oldSize == m_buffer.size()) {
                    break; //we didn't get any data
                }
                oldSize = m_buffer.size();
            }

            DWORD bytesRead = qMin(m_buffer.size(), int(length));
            memcpy(buffer, m_buffer.data(), bytesRead);

            //truncate the buffer
            m_buffer = m_buffer.mid(bytesRead);

            if (actual) {
                *actual = bytesRead; //initialization
            }

            return bytesRead == length ? S_OK : S_FALSE;
        }

        STDMETHODIMP QAsyncReader::Length(LONGLONG *total, LONGLONG *available)
        {
            QMutexLocker locker(&m_mutex);
            if (total) {
                *total = m_size;
            }

            if (available) {
                *available = m_size;
            }

            return S_OK;
        }

        //from Phonon::StreamInterface
        void QAsyncReader::writeData(const QByteArray &data)
        {
            m_pos += data.size();
            m_buffer += data;
        }

        void QAsyncReader::endOfData()
        {
        }

        void QAsyncReader::setStreamSize(qint64 newSize)
        {
            QMutexLocker locker(&m_mutex);
            m_size = newSize;
        }

        void QAsyncReader::setStreamSeekable(bool s)
        {
            QMutexLocker locker(&m_mutex);
            m_seekable = s;
        }

        //addition
        QAsyncReader::AsyncRequest QAsyncReader::getNextRequest()
        {
            QMutexLocker locker(&m_mutex);
            AsyncRequest ret;
            if (!m_requestQueue.isEmpty()) {
                ret = m_requestQueue.dequeue();
            }

            return ret;
        }
    }
}
