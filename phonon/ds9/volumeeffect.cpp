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

#include "volumeeffect.h"
#include "qbasefilter.h"
#include "qmeminputpin.h"

#include <initguid.h>


//TEST
#include <qslider.h>

namespace Phonon
{
    namespace DS9
    {
        // {9476B201-5D30-424a-8B0A-068C22803D0B}
        DEFINE_GUID(CLSID_VolumeEffect,
            0x9476b201, 0x5d30, 0x424a, 0x8b, 0xa, 0x6, 0x8c, 0x22, 0x80, 0x3d, 0xb);


        static const AM_MEDIA_TYPE audioMediaType()
		{
			AM_MEDIA_TYPE ret;
			ret.majortype = MEDIATYPE_Audio;
			ret.subtype = MEDIASUBTYPE_PCM;
			ret.bFixedSizeSamples = TRUE;
			ret.bTemporalCompression = FALSE;
			ret.lSampleSize = 1;
			ret.formattype = GUID_NULL;
			ret.pUnk = 0;
			ret.cbFormat = 0;
			ret.pbFormat = 0;
			return ret;
		}


        //tricky way to test that the effect actually works
        static QSlider *g_slider = 0;

        class VolumeEffectFilter : public QBaseFilter
        {
        public:
            VolumeEffectFilter();
            void setVolume(float newVolume);
            float volume() const;
            HRESULT Transform(IMediaSample * ms);

        private:
            AM_MEDIA_TYPE m_mt; //the current media type
            float m_volume;
        };

        VolumeEffectFilter::VolumeEffectFilter() : QBaseFilter(CLSID_VolumeEffect)
        {
            QVector<AM_MEDIA_TYPE> mt;

            //creating the output
            QPin *output = new QPin(this, PINDIR_OUTPUT, mt);

            //then creating the input
            mt << audioMediaType();
            QMemInputPin *input = new QMemInputPin(this, mt);
            input->addOutput(output); //make the connection here
        }

        void VolumeEffectFilter::setVolume(float newVolume)
        {
            m_volume = newVolume;
        }

        float VolumeEffectFilter::volume() const
        {
            return m_volume;
        }

        HRESULT VolumeEffectFilter::Transform(IMediaSample * ms)
        {
            if (m_volume != 1.) {
                BYTE *buffer = 0;
                ms->GetPointer(&buffer);

                REFERENCE_TIME start, end;
                ms->GetTime(&start, &end);
                if (buffer) {
                    switch(m_mt.lSampleSize)
                    {
                    case 2:
                        {
                            const qint16 *end = reinterpret_cast<qint16*>(buffer + ms->GetActualDataLength());
                            for(qint16 *current = reinterpret_cast<qint16*>(buffer); current < end; ++current) {
                                *current = qint16( *current * m_volume);
                            }
                            break;
                        }
                    case 1:
                        {
                            const BYTE *end = buffer + ms->GetActualDataLength();
                            for(BYTE *current = buffer; current < end; ++current) {
                                *current = BYTE( *current * m_volume);
                            }
                            break;
                        }
                    default:
                        qWarning("The Volume Effect can't handle a sample size of %D bytes.", m_mt.lSampleSize);

                    }
                }
            }
            return S_OK;
        }

        VolumeEffect::VolumeEffect(QObject *parent) : Effect(parent)
        {
            //creation of the effects
            for(QVector<Filter>::iterator it = m_filters.begin(); it != m_filters.end(); ++it) {
                VolumeEffectFilter *f = new VolumeEffectFilter();
                m_volumeFilters.append(f);
                *it = Filter(f);
            }

            g_slider = new QSlider();
            g_slider->setRange(0,100);
            g_slider->show();
            connect(g_slider, SIGNAL(valueChanged(int)), SLOT(test()));
        }

        float VolumeEffect::volume() const
        {
            return m_volumeFilters.first()->volume();
        }

        void VolumeEffect::setVolume(float newVolume)
        {
            foreach(VolumeEffectFilter *f, m_volumeFilters) {
                f->setVolume(newVolume);
            }
        }

        void VolumeEffect::test()
        {
            setVolume( g_slider->value() / 100.f);
        }
    }
}

#include "moc_volumeeffect.cpp"
