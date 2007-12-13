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

#include "audiooutput.h"
#include <QtCore/QVector>

#include <cmath>

namespace Phonon
{
    namespace DS9
    {
        AudioOutput::AudioOutput(Backend *back, QObject *parent)
            : BackendNode(parent), m_device(-1), m_backend(back), m_volume(0.),
            m_currentIndex(0), m_crossfadeProgress(1.)
        {
        }

        AudioOutput::~AudioOutput()
        {
        }

        int AudioOutput::outputDevice() const
        {
            return m_device;
        }

        static const qreal log10over20 = 0.1151292546497022842; // ln(10) / 20

        void AudioOutput::setVolume(qreal newVolume)
        {
            for(int i = 0; i < m_filters.count(); ++i) {
                ComPointer<IBasicAudio> audio(m_filters[i]);
                if (audio) {
                    const qreal currentVolume = newVolume * (m_currentIndex == i ? m_crossfadeProgress : 1-m_crossfadeProgress);
                    const qreal newDbVolume = (qMax(0., 1.-::log(::pow(currentVolume, -log10over20)))-1.) * 10000;
                    HRESULT hr = audio->put_Volume(newDbVolume);
                    Q_UNUSED(hr);
                    Q_ASSERT(SUCCEEDED(hr));
                }
            }

            if (m_volume != newVolume) {
                m_volume = newVolume;
                emit volumeChanged(newVolume);
            }
        }

        void AudioOutput::setCrossFadingProgress(short currentIndex, qreal progress)
        {
            m_crossfadeProgress = progress;
            m_currentIndex = currentIndex;
            setVolume(m_volume);
        }

        qreal AudioOutput::volume() const
        {
            return m_volume;
        }

        bool AudioOutput::setOutputDevice(int newDevice)
        {
            //free the previous one if it was already set
            bool ret = true;
            for(QVector<Filter>::iterator it = m_filters.begin(); it != m_filters.end(); ++it) {
                Filter &filter = *it;
                filter = Filter();
                m_device = newDevice;
                ComPointer<IMoniker> mon = m_backend->getAudioOutputMoniker(newDevice);
                if (mon) {
                    HRESULT hr = mon->BindToObject(0, 0, IID_IBaseFilter, filter.pdata());
                    ret = ret && SUCCEEDED(hr);
                }
            }
            setVolume(m_volume);
            return ret;
        }

    }
} //namespace Phonon::DS9

#include "moc_audiooutput.cpp"
