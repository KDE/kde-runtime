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

#include "iodevicereader.h"
#include "qasyncreader.h"

#include <phonon/streaminterface.h>

#include <initguid.h>

namespace Phonon
{
    namespace DS9
    {

        // {A222A867-A6FF-4f90-B514-0719A44D9F66}
        DEFINE_GUID(CLSID_IODeviceReader,
            0xa222a867, 0xa6ff, 0x4f90, 0xb5, 0x14, 0x7, 0x19, 0xa4, 0x4d, 0x9f, 0x66);

        //these mediatypes define a stream, its type will be autodetected by DirectShow
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

        class StreamReader : public QAsyncReader, public Phonon::StreamInterface
        {
        public:
            StreamReader(QBaseFilter *parent, const MediaSource &source) : 
              QAsyncReader(parent, getMediaTypes()),
                m_seekable(false), m_pos(0), m_size(-1)
            {
                connectToSource(source);
            }

            //for Phonon::StreamInterface
            void writeData(const QByteArray &data)
            {
                QWriteLocker locker(&m_lock);
                m_pos += data.size();
                m_buffer += data;
            }

            void endOfData()
            {
            }

            void setStreamSize(qint64 newSize)
            {
                QWriteLocker locker(&m_lock);
                m_size = newSize;
            }

            qint64 streamSize() const
            {
                QReadLocker locker(&m_lock);
                return m_size;
            }

            void setStreamSeekable(bool s)
            {
                QWriteLocker locker(&m_lock);
                m_seekable = s;
            }

            bool streamSeekable() const
            {
                QReadLocker locker(&m_lock);
                return m_seekable;
            }

            void setCurrentPos(qint64 pos)
            {
                QWriteLocker locker(&m_lock);
                m_pos = pos;
                seekStream(pos);
                m_buffer.clear();
            }

            qint64 currentPos() const
            {
                QReadLocker locker(&m_lock);
                return m_pos;
            }

            int currentBufferSize() const
            {
                QReadLocker locker(&m_lock);
                return m_buffer.size();
            }

            //virtual pure members
            
            //implementation from IAsyncReader
            STDMETHODIMP Length(LONGLONG *total, LONGLONG *available)
            {
                QReadLocker locker(&m_lock);
                if (total) {
                    *total = m_size;
                }

                if (available) {
                    *available = m_size;
                }

                return S_OK;
            }


            HRESULT read(LONGLONG pos, LONG length, BYTE *buffer, LONG *actual)
            {
                QMutexLocker locker(&m_mutexRead);

                if(streamSize() !=1 && pos + length > streamSize()) {
                    //it tries to read outside of the boundaries
                    return E_FAIL;
                }

                if (currentPos() - currentBufferSize() != pos) {
                    if (!streamSeekable()) {
                        return S_FALSE;
                    }
                    setCurrentPos(pos);
                }

                int oldSize = currentBufferSize();
                while (currentBufferSize() < int(length)) {
                    needData();
                    if (oldSize == currentBufferSize()) {
                        break; //we didn't get any data
                    }
                    oldSize = currentBufferSize();
                }

                DWORD bytesRead = qMin(currentBufferSize(), int(length));
                {
                    QWriteLocker locker(&m_lock);
                    qMemCopy(buffer, m_buffer.data(), bytesRead);
                    //truncate the buffer
                    m_buffer = m_buffer.mid(bytesRead);
                }

                if (actual) {
                    *actual = bytesRead; //initialization
                }

                return bytesRead == length ? S_OK : S_FALSE;
            }

        private:
            //for Phonon::StreamInterface
            QByteArray m_buffer;
            bool m_seekable;
            qint64 m_pos;
            qint64 m_size;

            QMutex m_mutexRead;
        };

        IODeviceReader::IODeviceReader(const MediaSource &source) :
        QBaseFilter(CLSID_IODeviceReader)
        {
            //create the output pin
            new StreamReader(this, source);
        }

        IODeviceReader::~IODeviceReader()
        {
        }
    }
}
