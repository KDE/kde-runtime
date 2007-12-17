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
                ComPointer<IBasicAudio> audio(m_filters[i], IID_IBasicAudio);
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
                OutputPin out;
                ComPointer<IGraphBuilder> graph;
                FILTER_STATE state = State_Stopped; 
                if (filter) {
                    FILTER_INFO info;
                    filter->QueryFilterInfo(&info);
                    graph = ComPointer<IGraphBuilder>(info.pGraph, IID_IGraphBuilder);

                    InputPin pin = BackendNode::pins(filter, PINDIR_INPUT).first();
                    if (pin->ConnectedTo(&out) == S_OK) {

                        //if it is connected we disconnect it and
                        //we'll reconnect out to the new filter
                        if (graph) {
                            filter->GetState(0, &state);
                            if (state != State_Stopped) {
                                ComPointer<IMediaControl>(graph, IID_IMediaControl)->Stop();
                            }
                            HRESULT hr = graph->RemoveFilter(filter);
                            Q_ASSERT(SUCCEEDED(hr));
                        }
                    }
                }

                ComPointer<IMoniker> mon = m_backend->getAudioOutputMoniker(newDevice);
                if (mon) {
                    HRESULT hr = mon->BindToObject(0, 0, IID_IBaseFilter, 
                        reinterpret_cast<void**>(&filter));

                    //let's connect the new filter
                    if (SUCCEEDED(hr) && graph) {
                        graph->AddFilter(filter, 0);
                        if (out && SUCCEEDED(hr)) {
                            InputPin pin = BackendNode::pins(filter, PINDIR_INPUT).first();
                            hr = graph->Connect(out, pin);
                            if (state == State_Paused) {
                                ComPointer<IMediaControl>(graph, IID_IMediaControl)->Pause();
                            } else if (state == State_Running) {
                                ComPointer<IMediaControl>(graph, IID_IMediaControl)->Run();
                            }

                        }
                    }

                    ret = ret && SUCCEEDED(hr);
                } else {
                    filter = Filter();
                }
            }

            m_device = newDevice;
            setVolume(m_volume);
            return ret;
        }

    }
} //namespace Phonon::DS9

#include "moc_audiooutput.cpp"
