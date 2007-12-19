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

#include "fakesource.h"
#include "qpin.h"

#include <initguid.h>
#include <amvideo.h> // VIDEOINFOHEADER

namespace Phonon
{
    namespace DS9
    {
        // {DFBECCBE-4C37-4bb9-B5B6-C718EA5D1FE8}
        DEFINE_GUID(CLSID_FakeSource,
            0xdfbeccbe, 0x4c37, 0x4bb9, 0xb5, 0xb6, 0xc7, 0x18, 0xea, 0x5d, 0x1f, 0xe8);

        static WAVEFORMATEX g_defaultWaveFormat = {WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0};

        class FakePin : public QPin
        {
        public:
            FakePin(FakeSource *source, const AM_MEDIA_TYPE &mt) :
              QPin(source, PINDIR_OUTPUT, QVector<AM_MEDIA_TYPE>() << mt), m_source(source)
            {
                setAvailable(true);
            }

            ~FakePin()
            {
            }


            STDMETHODIMP Disconnect()
            {
                HRESULT hr = QPin::Disconnect();
                if (SUCCEEDED(hr)) {
                    setAvailable(true);
                }
                return hr;
            }


            STDMETHODIMP Connect(IPin *pin, const AM_MEDIA_TYPE *type)
            {
                HRESULT hr = QPin::Connect(pin, type);
                if (SUCCEEDED(hr)) {
                    setAvailable(false);
                }
                return hr;
            }

        private:
            void setAvailable(bool avail)
            {
                if (mediaTypes().first().majortype == MEDIATYPE_Audio) {
                    if (avail) {
                        m_source->addAvailableAudioPin(this);
                    } else {
                        m_source->removeAvailableAudioPin(this);
                    }
                } else {
                    if (avail) {
                        m_source->addAvailableVideoPin(this);
                    } else {
                        m_source->removeAvailableVideoPin(this);
                    }
                }
            }

            FakeSource *m_source;


        };

        FakeSource::FakeSource() : QBaseFilter(CLSID_FakeSource)
        {
            createFakeAudioPin();
            createFakeVideoPin();
        }

        FakeSource::~FakeSource()
        {
        }

        void FakeSource::addAvailableAudioPin(FakePin *pin)
        {
            availableAudioPins += pin;
        }

        void FakeSource::addAvailableVideoPin(FakePin *pin)
        {
            availableVideoPins += pin;
        }

        void FakeSource::removeAvailableAudioPin(FakePin *pin)
        {
            availableAudioPins -= pin;

            if (availableAudioPins.isEmpty()) {
                createFakeAudioPin();
            }
        }

        void FakeSource::removeAvailableVideoPin(FakePin *pin)
        {
            availableVideoPins -= pin;

            if (availableVideoPins.isEmpty()) {
                createFakeVideoPin();
            }
        }

        void FakeSource::createFakeAudioPin()
        {
            AM_MEDIA_TYPE mt;
            qMemSet(&mt, 0, sizeof(AM_MEDIA_TYPE));
            mt.majortype = MEDIATYPE_Audio;
            mt.subtype = MEDIASUBTYPE_PCM;
            mt.formattype = FORMAT_WaveFormatEx;
            mt.lSampleSize = 2;

            //fake the format (stereo 44.1 khz stereo 16 bits)
            mt.cbFormat = sizeof(WAVEFORMATEX);
            mt.pbFormat = reinterpret_cast<BYTE*>(&g_defaultWaveFormat);

            new FakePin(this, mt);
        }

        void FakeSource::createFakeVideoPin()
        {
            AM_MEDIA_TYPE mt;
            qMemSet(&mt, 0, sizeof(AM_MEDIA_TYPE));
            mt.majortype = MEDIATYPE_Video;
            mt.subtype = MEDIASUBTYPE_RGB32;
            mt.formattype = FORMAT_VideoInfo;
            new FakePin(this, mt);
        }

    }
}
