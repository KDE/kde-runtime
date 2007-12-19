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

#include "qmeminputpin.h"
#include "qbasefilter.h"
#include "compointer.h"

#include <qdebug.h>

namespace Phonon
{
    namespace DS9
    {

        QMemInputPin::QMemInputPin(QBaseFilter *parent, const QVector<AM_MEDIA_TYPE> &mt) : 
            QPin(parent, PINDIR_INPUT, mt)
        {
        }

        QMemInputPin::~QMemInputPin()
        {
        }

        STDMETHODIMP QMemInputPin::QueryInterface(REFIID iid, void **out)
        {
            if (!out) {
                return E_POINTER;
            }

            if (iid == IID_IMemInputPin) {
                *out = static_cast<IMemInputPin*>(this);
                AddRef();
                return S_OK;
            } else {
                return QPin::QueryInterface(iid, out);
            }
        }

        STDMETHODIMP_(ULONG) QMemInputPin::AddRef()
        {
            return QPin::AddRef();
        }

        STDMETHODIMP_(ULONG) QMemInputPin::Release()
        {
            return QPin::Release();
        }

        //reimplementation to set the type for the output pin
        //no need to make a deep copy here
        void QMemInputPin::setConnectedType(const AM_MEDIA_TYPE &type)
        {
            QPin::setConnectedType(type);
            //we tell the output pins that they should connect with this type
            foreach(QPin *current, outputs()) {
                current->setAcceptedMediaType(type);
            }
        }

        STDMETHODIMP QMemInputPin::GetAllocator(IMemAllocator **alloc)
        {
            if (!alloc) {
                return E_POINTER;
            }

            if (*alloc = memoryAllocator(true)) {
                return S_OK;
            }

            return VFW_E_NO_ALLOCATOR;
        }

        STDMETHODIMP QMemInputPin::NotifyAllocator(IMemAllocator *alloc, BOOL readonly)
        {
            if (!alloc) {
                return E_POINTER;
            }

            {
                QWriteLocker locker(&m_lock);
                m_samplesReadonly = readonly;


            }

            setMemoryAllocator(alloc);
            return S_OK;
        }

        STDMETHODIMP QMemInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *prop)
        {
            if (!prop) {
                return E_POINTER;
            }

            //we have no particular requirements
            return E_NOTIMPL;
        }

        STDMETHODIMP QMemInputPin::Receive(IMediaSample *sample)
        {
            QMutexLocker locker(&m_mutexReceive);
            if (!sample) {
                return E_POINTER;
            }

            if (filterState() == State_Stopped) {
                return VFW_E_WRONG_STATE;
            }

            if (!m_samplesReadonly) {
                //we do it just once
                m_parent->processSample(sample);
            }

            foreach(QPin *current, outputs()) {
                ComPointer<IPin> pin;
                current->ConnectedTo(&pin);
                IMediaSample *outSample = m_samplesReadonly ? 
                    duplicateSampleForOutput(sample, current->memoryAllocator())
                    : sample;

                if (m_samplesReadonly) {
                    m_parent->processSample(outSample);
                }

                if (pin) {
                    ComPointer<IMemInputPin> input(pin, IID_IMemInputPin);
                    if (input) {
                        input->Receive(outSample);
                    }
                }

                if (m_samplesReadonly) {
                    outSample->Release();
                }

            }
            return S_OK;
        }

        STDMETHODIMP QMemInputPin::ReceiveMultiple(IMediaSample **samples,long count,long *nbDone)
        {
            //no need to lock here: there is no access to member data
            if (!samples || !nbDone) {
                return E_POINTER;
            }

            *nbDone = 0; //initialization
            while( *nbDone != count) {
                HRESULT hr = Receive(samples[*nbDone]);
                if (FAILED(hr)) {
                    return hr;
                }
                *nbDone++;
            }

            return S_OK;
        }

        STDMETHODIMP QMemInputPin::ReceiveCanBlock()
        {
            //the pin doesn't block
            return S_FALSE;
        }

        //addition
        //this should be used by the filter to tell it's input pins to which output they should route the samples

        void QMemInputPin::addOutput(QPin *output)
        {
            QWriteLocker locker(&m_lock);
            m_outputs.insert(output);
        }

        void QMemInputPin::removeOutput(QPin *output)
        {
            QWriteLocker locker(&m_lock);
            m_outputs.remove(output);
        }

        QSet<QPin*> QMemInputPin::outputs() const
        {
            QReadLocker locker(&m_lock);
            return m_outputs;
        }

        IMediaSample *QMemInputPin::duplicateSampleForOutput(IMediaSample *sample, IMemAllocator *alloc)
        {
            HRESULT hr = alloc->Commit();
            Q_ASSERT(SUCCEEDED(hr));

            IMediaSample *out;
            hr = alloc->GetBuffer(&out, 0, 0, AM_GBF_NOTASYNCPOINT);
            Q_ASSERT(SUCCEEDED(hr));

            //let's copy the sample
            {
                REFERENCE_TIME start, end;
                sample->GetTime(&start, &end);
                out->SetTime(&start, &end);
            }

            LONG length = sample->GetActualDataLength();
            hr = out->SetActualDataLength(length);
            Q_ASSERT(SUCCEEDED(hr));
            hr = out->SetDiscontinuity(sample->IsDiscontinuity());
            Q_ASSERT(SUCCEEDED(hr));
            
            {
                LONGLONG start, end;
                hr = sample->GetMediaTime(&start, &end);
                if (hr != VFW_E_MEDIA_TIME_NOT_SET) {
                    hr = out->SetMediaTime(&start, &end);
                    Q_ASSERT(SUCCEEDED(hr));
                }
            }

            AM_MEDIA_TYPE *type = 0;
            hr = sample->GetMediaType(&type);
            Q_ASSERT(SUCCEEDED(hr));
            hr = out->SetMediaType(type);
            Q_ASSERT(SUCCEEDED(hr));

            hr = out->SetPreroll(sample->IsPreroll());
            Q_ASSERT(SUCCEEDED(hr));
            hr = out->SetSyncPoint(sample->IsSyncPoint());
            Q_ASSERT(SUCCEEDED(hr));

            BYTE *dest = 0, *src = 0;
            hr = out->GetPointer(&dest);
            Q_ASSERT(SUCCEEDED(hr));
            hr = sample->GetPointer(&src);
            Q_ASSERT(SUCCEEDED(hr));

            qMemCopy(dest, src, sample->GetActualDataLength());

            return out;
        }


    }
}
