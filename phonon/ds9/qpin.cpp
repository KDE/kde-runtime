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

#include "qbasefilter.h"
#include "qpin.h"
#include "compointer.h"

namespace Phonon
{
    namespace DS9
    {

		static const AM_MEDIA_TYPE defaultMediaType()
		{
			AM_MEDIA_TYPE ret;
			ret.majortype = MEDIATYPE_NULL;
			ret.subtype = MEDIASUBTYPE_NULL;
			ret.bFixedSizeSamples = TRUE;
			ret.bTemporalCompression = FALSE;
			ret.lSampleSize = 1;
			ret.formattype = GUID_NULL;
			ret.pUnk = 0;
			ret.cbFormat = 0;
			ret.pbFormat = 0;
			return ret;
		}

        class QEnumMediaTypes : public IEnumMediaTypes
        {
        public:
            QEnumMediaTypes(QPin *pin) :  m_refCount(1), m_pin(pin), m_index(0)
            {
                m_pin->AddRef();
            }

            ~QEnumMediaTypes()
            {
                m_pin->Release();
            }

            STDMETHODIMP QueryInterface(const IID &iid,void **out)
            {
                if (!out) {
                    return E_POINTER;
                }

                HRESULT hr = S_OK;
                if (iid == IID_IEnumMediaTypes) {
                    *out = static_cast<IEnumMediaTypes*>(this);
                } else if (iid == IID_IUnknown) {
                    *out = static_cast<IUnknown*>(this);
                } else {
                    *out = 0;
                    hr = E_NOINTERFACE;
                }

                if (hr == S_OK) {
                    AddRef();
                }
                return hr;
            }

            STDMETHODIMP_(ULONG) AddRef()
            {
                return InterlockedIncrement(&m_refCount);
            }

            STDMETHODIMP_(ULONG) Release()
            {
                ULONG refCount = InterlockedDecrement(&m_refCount);
                if (refCount == 0) {
                    delete this;
                }

                return refCount;
            }

            STDMETHODIMP Next(ULONG count, AM_MEDIA_TYPE **out, ULONG *fetched)
            {
                QMutexLocker locker(&m_mutex);
                if (!out) {
                    return E_POINTER;
                }

                if (!fetched && count > 1) {
                    return E_INVALIDARG;
                }

                int nbFetched = 0;
                while (nbFetched < int(count) && m_index < m_pin->mediaTypes().count()) {
                    //the caller will deallocate the memory
                    *out = static_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
                    AM_MEDIA_TYPE mt = m_pin->mediaTypes()[m_index];
                    // avoid the destruction of the pointers (they are not shared)
                    mt.pbFormat =0;
                    mt.cbFormat = 0;
                    mt.pUnk = 0;

                    **out = mt;
                    nbFetched++;
                    m_index++;
                    out++;
                }

                if (fetched) {
                    *fetched = nbFetched;
                }

                return nbFetched == count ? S_OK : S_FALSE;
            }

            STDMETHODIMP Skip(ULONG count)
            {
                QMutexLocker locker(&m_mutex);
                m_index = qMin(m_index + int(count), m_pin->mediaTypes().count());
                return  (m_index == m_pin->mediaTypes().count()) ? S_FALSE : S_OK;
            }

            STDMETHODIMP Reset()
            {
                QMutexLocker locker(&m_mutex);
                m_index = 0;
                return S_OK;
            }

            STDMETHODIMP Clone(IEnumMediaTypes **out)
            {
                QMutexLocker locker(&m_mutex);
                if (!out) {
                    return E_POINTER;
                }

                *out = new QEnumMediaTypes(m_pin);
                (*out)->Skip(m_index);
                return S_OK;
            }


        private:
            LONG m_refCount;
            QPin *m_pin;
            int m_index;
            QMutex m_mutex;
        };


        QPin::QPin(QBaseFilter *parent, PIN_DIRECTION dir, const QVector<AM_MEDIA_TYPE> &mt) :
            m_memAlloc(0), m_parent(parent), m_refCount(1),  m_connected(0),
            m_direction(dir), m_mediaTypes(mt), m_connectedType(defaultMediaType()),
            m_flushing(false)
        {
            Q_ASSERT(m_parent);
            m_parent->addPin(this);
        }

        QPin::~QPin()
        {
            m_parent->removePin(this);
            setMemoryAllocator(0);
        }

        //reimplementation from IUnknown
        STDMETHODIMP QPin::QueryInterface(REFIID iid, void**out)
        {
            if (!out) {
                return E_POINTER;
            }

            HRESULT hr = S_OK;

            if (iid == IID_IPin) {
                *out = static_cast<IPin*>(this);
            } else if (iid == IID_IUnknown) {
                *out = static_cast<IUnknown*>(this);
            } else {
                *out = 0;
                hr = E_NOINTERFACE;
            }

            if (hr == S_OK) {
                AddRef();
            }
            return hr;
        }

        STDMETHODIMP_(ULONG) QPin::AddRef()
        {
            return InterlockedIncrement(&m_refCount);
        }

        STDMETHODIMP_(ULONG) QPin::Release()
        {
            ULONG refCount = InterlockedDecrement(&m_refCount);
            if (refCount == 0) {
                delete this;
            }

            return refCount;
        }

        //this is called on the input pins
        STDMETHODIMP QPin::ReceiveConnection(IPin *pin, const AM_MEDIA_TYPE *type)
        {
            QMutexLocker locker(&m_mutex);
            if (!pin ||!type) {
                return E_POINTER;
            }

            if (connected()) {
                return VFW_E_ALREADY_CONNECTED;
            }

            if (filterState() != State_Stopped) {
                return VFW_E_NOT_STOPPED;
            }

            if (QueryAccept(type) != S_OK) {
                return VFW_E_TYPE_NOT_ACCEPTED;
            }

            setConnected(pin);
            setConnectedType(*type);

            return S_OK;
        }

        //this is called on the output pins
        STDMETHODIMP QPin::Connect(IPin *pin, const AM_MEDIA_TYPE *type)
        {
            QMutexLocker locker(&m_mutex);
            if (!pin) {
                return E_POINTER;
            }

            if (connected()) {
                return VFW_E_ALREADY_CONNECTED;
            }

            if (filterState() != State_Stopped) {
                return VFW_E_NOT_STOPPED;
            }

            HRESULT hr = S_OK;

            setConnected(pin);
            if (!type) {

                //let(s first try the output pin's mediaTypes
                if (checkOutputMediaTypesConnection(pin) != S_OK &&
                    checkOwnMediaTypesConnection(pin) != S_OK) {
                    hr = VFW_E_NO_ACCEPTABLE_TYPES;
                }
            } else if (QueryAccept(type) == S_OK) {
                setConnectedType(*type);
            } else {
                hr = VFW_E_TYPE_NOT_ACCEPTED;
            }

            if (FAILED(hr)) {
                setConnected(0);
                setConnectedType(defaultMediaType());
            } else {
                ComPointer<IMemInputPin> input(pin, IID_IMemInputPin);
                
                if (input) {
                    ComPointer<IMemAllocator> alloc;
                    input->GetAllocator(&alloc);
                    if (alloc) {
                        //be default we take the allocator from the input pin
                        //we have no reason to force using our own

                        ALLOCATOR_PROPERTIES prop;
                        alloc->GetProperties(&prop);
                        if (prop.cBuffers == 0) {
                            if (input->GetAllocatorRequirements(&prop) != S_OK) {
                                prop = getDefaultAllocatorProperties();
                            }
                            ALLOCATOR_PROPERTIES actual;
                            hr = alloc->SetProperties(&prop, &actual);
                            Q_ASSERT(SUCCEEDED(hr));
                        }

                        setMemoryAllocator(alloc);
                    }
                }
                if (memoryAllocator() == 0) {
                    ALLOCATOR_PROPERTIES prop;
                    if (input && input->GetAllocatorRequirements(&prop) == S_OK) {
                        createDefaultMemoryAllocator(&prop);
                    } else {
                        createDefaultMemoryAllocator();
                    }
                }

                Q_ASSERT(memoryAllocator() != 0);
                if (input) {
                    input->NotifyAllocator(memoryAllocator(), TRUE); //TRUE is arbitrarily chosen here
                }

            }

            return hr;
        }

        STDMETHODIMP QPin::Disconnect()
        {
            QMutexLocker locker(&m_mutex);
            if (!connected()) {
                return S_FALSE;
            }

            if (filterState() != State_Stopped) {
                return VFW_E_NOT_STOPPED;
            }

            setConnected(0);
            return S_OK;
        }

        STDMETHODIMP QPin::ConnectedTo(IPin **other)
        {
            if (!other) {
                return E_POINTER;
            }

            *other = connected(true);
            if (!(*other)) {
                return VFW_E_NOT_CONNECTED;
            }

            return S_OK;
        }

        STDMETHODIMP QPin::ConnectionMediaType(AM_MEDIA_TYPE *type)
        {
            if (!type) {
                return E_POINTER;
            }

           *type = connectedType();
            if (!connected()) {
                return VFW_E_NOT_CONNECTED;
            } else {
                return S_OK;
            }
        }

        STDMETHODIMP QPin::QueryPinInfo(PIN_INFO *info)
        {
            QReadLocker locker(&m_lock);
            if (!info) {
                return E_POINTER;
            }

            info->dir = m_direction;
            info->pFilter = m_parent;
            m_parent->AddRef();
            qMemCopy(info->achName, m_name.utf16(), qMin(MAX_FILTER_NAME, m_name.length()+1) *2);

            return S_OK;
        }

        STDMETHODIMP QPin::QueryDirection(PIN_DIRECTION *dir)
        {
            QReadLocker locker(&m_lock);
            if (!dir) {
                return E_POINTER;
            }

            *dir = m_direction;
            return S_OK;
        }

        STDMETHODIMP QPin::QueryId(LPWSTR *id)
        {
            QReadLocker locker(&m_lock);
            if (!id) {
                return E_POINTER;
            }

            int nbBytes = (m_name.length()+1)*2;
            *id = static_cast<LPWSTR>(::CoTaskMemAlloc(nbBytes));
            qMemCopy(*id, m_name.utf16(), nbBytes);
            return S_OK;
        }

        STDMETHODIMP QPin::QueryAccept(const AM_MEDIA_TYPE *type)
        {
            QReadLocker locker(&m_lock);
            if (!type) {
                return E_POINTER;
            }

            foreach(const AM_MEDIA_TYPE current, m_mediaTypes) {

                if ( (type->majortype == current.majortype) &&
                    (current.subtype == MEDIASUBTYPE_NULL || type->subtype == current.subtype) &&
                    (current.formattype == GUID_NULL || type->formattype == current.formattype)
                    ) {
                    return S_OK;
                }
            }

            return S_FALSE;
        }


        STDMETHODIMP QPin::EnumMediaTypes(IEnumMediaTypes **emt)
        {
            if (!emt) {
                return E_POINTER;
            }

            *emt = new QEnumMediaTypes(this);
            return S_OK;
        }


        STDMETHODIMP QPin::EndOfStream()
        {
            return S_OK;
        }

        STDMETHODIMP QPin::BeginFlush()
        {
            QWriteLocker locker(&m_lock);
            m_flushing = true;
            return S_OK;
        }

        STDMETHODIMP QPin::EndFlush()
        {
            QWriteLocker locker(&m_lock);
            m_flushing = false;
            return S_OK;
        }

        STDMETHODIMP QPin::NewSegment(REFERENCE_TIME, REFERENCE_TIME, double)
        {
            return S_OK;
        }

        STDMETHODIMP QPin::QueryInternalConnections(IPin **, ULONG*)
        {
            //this is not implemented on purpose (the input pins are connected to all the output pins)
            return E_NOTIMPL;
        }


        HRESULT QPin::checkOutputMediaTypesConnection(IPin *pin)
        {
            IEnumMediaTypes *emt = 0;
            HRESULT hr = pin->EnumMediaTypes(&emt);
            if (FAILED(hr)) {
                return hr;
            }

            AM_MEDIA_TYPE *type = 0;
            while (emt->Next(1, &type, 0) == S_OK) {
                if (QueryAccept(type) == S_OK) {
                    setConnectedType(*type);
                    if (pin->ReceiveConnection(this, type) == S_OK) {
                        freeMediaType(type);
                        return S_OK;
                    } else {
                        freeMediaType(type);
                    }
                }
            }

            //we didn't find a suitable type
            return S_FALSE;
        }

        HRESULT QPin::checkOwnMediaTypesConnection(IPin *pin)
        {   
            foreach(const AM_MEDIA_TYPE current, mediaTypes()) {
                setConnectedType(current);
                if (pin->ReceiveConnection(this, &current) == S_OK) {
                    return S_OK;
                }
            }

            //we didn't find a suitable type
            return S_FALSE;
        }

        void QPin::freeMediaType(AM_MEDIA_TYPE *type)
        {
            if (type->cbFormat) {
                CoTaskMemFree(type->pbFormat);
            }
            if (type->pUnk) {
                type->pUnk->Release();
            }

            CoTaskMemFree(type);
        }

        //addition
        void QPin::setConnectedType(const AM_MEDIA_TYPE &type)
        {
            QWriteLocker locker(&m_lock);
            m_connectedType = type;
        }

        AM_MEDIA_TYPE QPin::connectedType() const 
        {
            QReadLocker locker(&m_lock);
            return m_connectedType;
        }

        void QPin::setConnected(IPin *pin)
        {
            QWriteLocker locker(&m_lock);
            if (pin) {
                pin->AddRef();
            }
            if (m_connected) {
                m_connected->Release();
            }
            m_connected = pin;
        }

        IPin *QPin::connected(bool addref) const
        {
            QReadLocker locker(&m_lock);
            if (addref && m_connected) {
                m_connected->AddRef();
            }
            return m_connected;
        }

        bool QPin::isFlushing() const
        {
            QReadLocker locker(&m_lock);
            return m_flushing;
        }

        FILTER_STATE QPin::filterState() const
        {
            QReadLocker locker(&m_lock);
            FILTER_STATE fstate = State_Stopped;
            m_parent->GetState(0, &fstate);
            return fstate;
        }

        QVector<AM_MEDIA_TYPE> QPin::mediaTypes() const
        {
            QReadLocker locker(&m_lock);
            return m_mediaTypes;
        }

        void QPin::setMediaType(const AM_MEDIA_TYPE &mt)
        {
            {
                QWriteLocker locker(&m_lock);
                m_mediaTypes = QVector<AM_MEDIA_TYPE>() << mt;
            }

            IPin *conn = connected();
            if (conn) {
                //try to reconnect to redefine the media type
                conn->Disconnect();
                Disconnect();
                HRESULT hr = Connect(conn, 0);


                Q_ASSERT(SUCCEEDED(hr));
            }
        }

        void QPin::createDefaultMemoryAllocator(ALLOCATOR_PROPERTIES *prop)
        {
            ComPointer<IMemAllocator> alloc(CLSID_MemoryAllocator, IID_IMemAllocator);
            if (prop) {
                alloc->SetProperties(prop, 0);
            }
            setMemoryAllocator(alloc);
        }

        void QPin::setMemoryAllocator(IMemAllocator *alloc)
        {
            QWriteLocker locker(&m_lock);
            if (alloc) {
                alloc->AddRef();
            }
            if (m_memAlloc) {
                m_memAlloc->Release();
            }
            m_memAlloc = alloc;
        }

        IMemAllocator *QPin::memoryAllocator(bool addref) const
        {
            QReadLocker locker(&m_lock);
            if (addref && m_memAlloc) {
                m_memAlloc->AddRef();
            }
            return m_memAlloc;
        }

        ALLOCATOR_PROPERTIES QPin::getDefaultAllocatorProperties() const
        {
            //those values reduce buffering a lot (good for the volume effect)
            ALLOCATOR_PROPERTIES prop;
            prop.cbAlign = 1;
            prop.cbBuffer = 16384;
            prop.cBuffers = 12;
            prop.cbPrefix = 0;
            return prop;
        }
    }
}
