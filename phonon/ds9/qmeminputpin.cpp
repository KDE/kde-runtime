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

#include "compointer.h"

namespace Phonon
{
    namespace DS9
    {

        QMemInputPin::QMemInputPin(QBaseFilter *parent, const QVector<AM_MEDIA_TYPE> &mt) :  QPin(parent, PINDIR_INPUT, mt)
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


        STDMETHODIMP QMemInputPin::GetAllocator(IMemAllocator **alloc)
        {
            QMutexLocker locker(&m_mutex);
            if (!alloc) {
                return E_POINTER;
            }

            *alloc = m_memAlloc;
            if (m_memAlloc) {
                m_memAlloc->AddRef();
                return S_OK;
            }

            return VFW_E_NO_ALLOCATOR;
        }

        STDMETHODIMP QMemInputPin::NotifyAllocator(IMemAllocator *alloc, BOOL readonly)
        {
            QMutexLocker locker(&m_mutex);
            if (!alloc) {
                return E_POINTER;
            }

            m_samplesReadonly = readonly;
            alloc->AddRef(); //we add a reference to it here
            if (m_memAlloc)
                m_memAlloc->Release();

            m_memAlloc = alloc;

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
            QMutexLocker locker(&m_mutex);
            if (!sample) {
                return E_POINTER;
            }

            if (filterState() == State_Stopped) {
                return VFW_E_WRONG_STATE;
            }

            //we just do nothing and transfer immediately the sample
            foreach(QPin *current, m_outputs) {
                ComPointer<IPin> pin;
                current->ConnectedTo(pin.pobject());
                if (pin) {
                    ComPointer<IMemInputPin> input(pin);
                    if (input) {
                        input->Receive(sample);
                    }
                }

            }

            return E_NOTIMPL;
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
            QMutexLocker locker(&m_mutex);
            m_outputs.insert(output);
        }

        void QMemInputPin::removeOutput(QPin *output)
        {
            QMutexLocker locker(&m_mutex);
            m_outputs.remove(output);
        }

    }
}
