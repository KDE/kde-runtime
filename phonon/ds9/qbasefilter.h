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

#ifndef PHONON_QBASEFILTER_H
#define PHONON_QBASEFILTER_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QReadWriteLock>

#include <dshow.h>

namespace Phonon
{
    namespace DS9
    {
        class QPin;
        class QBaseFilter : public IBaseFilter
        {
        public:
            QBaseFilter(const CLSID &clsid);
            virtual ~QBaseFilter();

            //reimplementation from IUnknown
            STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
            STDMETHODIMP_(ULONG) AddRef(void);
            STDMETHODIMP_(ULONG) Release();

            //reimplementation from IPersist
            STDMETHODIMP GetClassID(CLSID *);

            //reimplementation from IMediaFilter
            STDMETHODIMP Stop();
            STDMETHODIMP Pause();
            STDMETHODIMP Run(REFERENCE_TIME);
            STDMETHODIMP GetState(DWORD, FILTER_STATE*);
            STDMETHODIMP SetSyncSource(IReferenceClock*);
            STDMETHODIMP GetSyncSource(IReferenceClock**);

            //reimplementation from IBaseFilter
            STDMETHODIMP EnumPins(IEnumPins**);
            STDMETHODIMP FindPin(LPCWSTR, IPin**);
            STDMETHODIMP QueryFilterInfo(FILTER_INFO*);
            STDMETHODIMP JoinFilterGraph(IFilterGraph*, LPCWSTR);
            STDMETHODIMP QueryVendorInfo(LPWSTR*);

            //own methods
            const QList<QPin *> pins();
            void addPin(QPin *pin);
            void removePin(QPin *pin);

            //reimplement this if you want specific processing of media sample
            virtual void processSample(IMediaSample *);

        private:
            LONG m_refCount;
            CLSID m_clsid;
            QString m_name;
            IReferenceClock *m_clock;
            IFilterGraph *m_graph;
            FILTER_STATE m_state;
            QList<QPin *> m_pins;
            QReadWriteLock m_lock;
        };
    }
}

#endif
