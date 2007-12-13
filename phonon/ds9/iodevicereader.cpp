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

        //for the streams...
        //the filter
        IODeviceReader::IODeviceReader(const MediaSource &source) :
        QBaseFilter(CLSID_IODeviceReader)
        {
            //create the output pin
            new QAsyncReader(this, source);
        }

        IODeviceReader::~IODeviceReader()
        {
        }
    }
}
