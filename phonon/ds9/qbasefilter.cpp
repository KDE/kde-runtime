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

#include <qdebug.h>

namespace Phonon
{
    namespace DS9
    {
        class QEnumPins : public IEnumPins
        {
        public:
            QEnumPins(QBaseFilter *filter) : m_refCount(1),
                m_filter(filter), m_pins(filter->pins()), m_index(0)
            {
                m_filter->AddRef();
            }

            ~QEnumPins()
            {
                m_filter->Release();
            }

            STDMETHODIMP QueryInterface(const IID &iid,void **out)
            {
                if (!out) {
                    return E_POINTER;
                }

                HRESULT hr = S_OK;
                if (iid == IID_IEnumPins) {
                    *out = static_cast<IEnumPins*>(this);
                } else if (iid == IID_IUnknown) {
                    *out = static_cast<IUnknown*>(this);
                } else {
                    *out = 0;
                    hr = E_NOINTERFACE;
                }

                if (S_OK)
                    AddRef();
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

            STDMETHODIMP Next(ULONG count,IPin **ret,ULONG *fetched)
            {
                QMutexLocker locker(&m_mutex);
                if (m_filter->pins() != m_pins) {
                    return VFW_E_ENUM_OUT_OF_SYNC;
                }

                if (fetched == 0 && count > 1) {
                    return E_INVALIDARG;
                }

                if (!ret) {
                    return E_POINTER;
                }

                int nbfetched = 0;
                while (nbfetched < int(count) && m_index < m_pins.count()) {
                    IPin *current = m_pins[m_index];
                    current->AddRef();
                    ret[nbfetched] = current;
                    nbfetched++;
                    m_index++;
                }

                if (fetched) {
                    *fetched = nbfetched;
                }

                return nbfetched == count ? S_OK : S_FALSE;
            }

            STDMETHODIMP Skip(ULONG count)
            {
                QMutexLocker locker(&m_mutex);
                if (m_filter->pins() != m_pins) {
                    return VFW_E_ENUM_OUT_OF_SYNC;
                }

                m_index = qMin(m_index + int(count), m_pins.count());
                return m_index == m_pins.count() ? S_FALSE : S_OK;
            }

            STDMETHODIMP Reset()
            {
                QMutexLocker locker(&m_mutex);
                m_index = 0;
                return S_OK;
            }

            STDMETHODIMP Clone(IEnumPins **out)
            {
                QMutexLocker locker(&m_mutex);
                if (m_filter->pins() != m_pins) {
                    return VFW_E_ENUM_OUT_OF_SYNC;
                }

                if (!out) {
                    return E_POINTER;
                }

                *out = new QEnumPins(m_filter);
                (*out)->Skip(m_index);
                return S_OK;
            }


        private:
            LONG m_refCount;
            QBaseFilter *m_filter;
            QList<QPin*> m_pins;
            int m_index;
            QMutex m_mutex;
        };


        QBaseFilter::QBaseFilter(const CLSID &clsid):
        m_refCount(1), m_clsid(clsid), m_clock(0), m_graph(0), m_state(State_Stopped)
        {
        }

        QBaseFilter::~QBaseFilter()
        {
            while (!m_pins.isEmpty()) {
                delete m_pins.first();
            }
        }

        const QList<QPin *> QBaseFilter::pins() const
        {
            QReadLocker locker(&m_lock);
            return m_pins;
        }

        void QBaseFilter::addPin(QPin *pin)
        {
            QWriteLocker locker(&m_lock);
            m_pins.append(pin);
        }

        void QBaseFilter::removePin(QPin *pin)
        {
            QWriteLocker locker(&m_lock);
            m_pins.removeAll(pin);
        }

        STDMETHODIMP QBaseFilter::QueryInterface(REFIID iid, void **out)
        {
            if (!out) {
                return E_POINTER;
            }

            HRESULT hr = S_OK;

            if (iid == IID_IBaseFilter) {
                *out = static_cast<IBaseFilter*>(this);
            } else if (iid == IID_IMediaFilter) {
                *out = static_cast<IMediaFilter*>(this);
            } else if (iid == IID_IPersist) {
                *out = static_cast<IPersist*>(this);
            } else if (iid == IID_IUnknown) {
                *out = static_cast<IUnknown*>(static_cast<IBaseFilter*>(this));
            } else if (iid == IID_IMediaSeeking) {
                *out = static_cast<IMediaSeeking*>(this);
            } else {
                *out = 0;
                hr = E_NOINTERFACE;
            }

            if (hr == S_OK) {
                AddRef();
            }

            return hr;
        }

        STDMETHODIMP_(ULONG) QBaseFilter::AddRef()
        {
            return InterlockedIncrement(&m_refCount);
        }

        STDMETHODIMP_(ULONG) QBaseFilter::Release()
        {
            ULONG refCount = InterlockedDecrement(&m_refCount);
            if (refCount == 0) {
                delete this;
            }

            return refCount;
        }

        STDMETHODIMP QBaseFilter::GetClassID(CLSID *clsid)
        {
            QReadLocker locker(&m_lock);
            *clsid = m_clsid;
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::Stop()
        {
            QWriteLocker locker(&m_lock);
            m_state = State_Stopped;
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::Pause()
        {
            QWriteLocker locker(&m_lock);
            m_state = State_Paused;
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::Run(REFERENCE_TIME t)
        {
            QWriteLocker locker(&m_lock);
            m_state = State_Running;
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::GetState(DWORD ms, FILTER_STATE *state)
        {
            QReadLocker locker(&m_lock);
            if (state) {
                *state = m_state;
                return S_OK;
            }
            return E_POINTER;
        }

        STDMETHODIMP QBaseFilter::SetSyncSource(IReferenceClock *clock)
        {
            QWriteLocker locker(&m_lock);
            if (clock) {
                clock->AddRef();
            }
            if (m_clock) {
                m_clock->Release();
            }
            m_clock = clock;
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::GetSyncSource(IReferenceClock **clock)
        {
            QReadLocker locker(&m_lock);
            if (!clock) {
                return E_POINTER;
            }

            if (m_clock) {
                m_clock->AddRef();
            }

            *clock = m_clock;
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::FindPin(LPCWSTR name, IPin**pin)
        {
            if (!pin) {
                return E_POINTER;
            }

            foreach(IPin *current, pins()) {
                PIN_INFO info;
                current->QueryPinInfo(&info);
                if (info.pFilter) {
                    info.pFilter->Release();
                }
                if ( wcscmp(info.achName, name) == 0) {
                    *pin = current;
                    current->AddRef();
                    return S_OK;
                }
            }

            *pin = 0;
            return VFW_E_NOT_FOUND;
        }

        STDMETHODIMP QBaseFilter::QueryFilterInfo(FILTER_INFO *info )
        {
            QReadLocker locker(&m_lock);
            if (!info) {
                return E_POINTER;
            }
            info->pGraph = m_graph;
            if (m_graph) {
                m_graph->AddRef();
            }
            qMemCopy(info->achName, m_name.utf16(), qMin(MAX_FILTER_NAME, m_name.length()+1) *2);
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::JoinFilterGraph(IFilterGraph *graph, LPCWSTR name)
        {
            QWriteLocker locker(&m_lock);
            m_graph = graph;
            m_name = QString::fromWCharArray(name);
            return S_OK;
        }

        STDMETHODIMP QBaseFilter::EnumPins( IEnumPins **ep)
        {
            if (!ep) {
                return E_POINTER;
            }

            *ep = new QEnumPins(this);
            return S_OK;
        }


        STDMETHODIMP QBaseFilter::QueryVendorInfo(LPWSTR *)
        {
            //we give no information on that
            return E_NOTIMPL;
        }

        //implementation from IMediaSeeking

        IMediaSeeking *QBaseFilter::getUpStreamMediaSeeking() const
        {
            foreach(QPin *pin, pins()) {
                if (pin->direction() == PINDIR_INPUT) {
                    IPin *out = pin->connected();
                    if (out) {
                        IMediaSeeking *ms = 0;
                        out->QueryInterface(IID_IMediaSeeking, reinterpret_cast<void**>(&ms));
                        if (ms) {
                            return ms;
                        }
                    }
                }
            }
            //none was found
            return 0;
        }
        

        STDMETHODIMP QBaseFilter::GetCapabilities(DWORD *pCapabilities)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetCapabilities(pCapabilities);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::CheckCapabilities(DWORD *pCapabilities)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->CheckCapabilities(pCapabilities);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::IsFormatSupported(const GUID *pFormat)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->IsFormatSupported(pFormat);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::QueryPreferredFormat(GUID *pFormat)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->QueryPreferredFormat(pFormat);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetTimeFormat(GUID *pFormat)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetTimeFormat(pFormat);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::IsUsingTimeFormat(const GUID *pFormat)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->IsUsingTimeFormat(pFormat);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::SetTimeFormat(const GUID *pFormat)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->SetTimeFormat(pFormat);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetDuration(LONGLONG *pDuration)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetDuration(pDuration);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetStopPosition(LONGLONG *pStop)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetStopPosition(pStop);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetCurrentPosition(LONGLONG *pCurrent)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetCurrentPosition(pCurrent);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat,
            LONGLONG Source, const GUID *pSourceFormat)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetPositions(pCurrent, pStop);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetAvailable(pEarliest, pLatest);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::SetRate(double dRate)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->SetRate(dRate);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetRate(double *pdRate)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetRate(pdRate);
                ms->Release();
            }
            return ret;
        }

        STDMETHODIMP QBaseFilter::GetPreroll(LONGLONG *pllPreroll)
        {
            IMediaSeeking *ms = getUpStreamMediaSeeking();
            HRESULT ret = E_NOTIMPL;
            if (ms) {
                ret = ms->GetPreroll(pllPreroll);
                ms->Release();
            }
            return ret;
        }

        //addition
        void QBaseFilter::processSample(IMediaSample *sample)
        {
        }

    }
}
