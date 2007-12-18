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

#include <QtCore/QSet>
#include "qpin.h"

#include <dshow.h>


namespace Phonon
{
    namespace DS9
    {

        class QBaseFilter;

        //this class will be used for our effects
        class QMemInputPin : public QPin, public IMemInputPin
        {
        public:
            QMemInputPin(QBaseFilter *, const QVector<AM_MEDIA_TYPE> &);
            ~QMemInputPin();

            //reimplementation from IUnknown
            STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
            STDMETHODIMP_(ULONG) AddRef();
            STDMETHODIMP_(ULONG) Release();

            //reimplementation from IPin
            STDMETHODIMP ReceiveConnection(IPin *,const AM_MEDIA_TYPE *);


            //reimplementation from IMemAllocator
            STDMETHODIMP GetAllocator(IMemAllocator **);
            STDMETHODIMP NotifyAllocator(IMemAllocator *,BOOL);
            STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *);
            STDMETHODIMP Receive(IMediaSample *);
            STDMETHODIMP ReceiveMultiple(IMediaSample **,long,long *);
            STDMETHODIMP ReceiveCanBlock();

            //addition
            void addOutput(QPin *output);
            void removeOutput(QPin *output);
            QSet<QPin*> outputs() const;

        private:
            IMediaSample *duplicateSampleForOutput(IMediaSample *, IMemAllocator *);

            bool m_samplesReadonly;
            QSet<QPin*> m_outputs;
            QMutex m_mutexReceive;
        };
    }
}
