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
#include "audiograph.h"
#include "audiodevice.h"
#include "mediaobject.h"
#include <private/qcore_mac_p.h>

namespace Phonon
{
namespace QT7
{

AudioOutputAudioPart::AudioOutputAudioPart() : AudioNode(1, 0)
{
    m_audioDevice = AudioDevice::defaultDevice(AudioDevice::Out);
    m_volume = 1;
}

ComponentDescription AudioOutputAudioPart::getAudioNodeDescription()
{
	ComponentDescription description;
	description.componentType = kAudioUnitType_Output;
	description.componentSubType = kAudioUnitSubType_DefaultOutput;
	description.componentManufacturer = kAudioUnitManufacturer_Apple;
	description.componentFlags = 0;
	description.componentFlagsMask = 0;
    return description;
}

void AudioOutputAudioPart::initializeAudioUnit()
{
    setAudioDevice(m_audioDevice);
    setVolume(m_volume);
}

void AudioOutputAudioPart::setAudioDevice(AudioDeviceID device)
{
    m_audioDevice = device;
    if (!m_audioDevice)
        return;
    if (!m_audioUnit)
        return;
    bool ok = AudioDevice::setDevice(m_audioUnit, m_audioDevice, AudioDevice::Out);
    if (!ok)
        emit audioDeviceFailed();
}

void AudioOutputAudioPart::setVolume(float volume)
{
    if (volume < 0)
        m_volume = 0;
    if (volume > 1)
        m_volume = 1;
    else
        m_volume = volume;

    if (m_audioUnit){
        float db = volume;//20.0 * log10(volume); // convert to db
        OSStatus err = AudioUnitSetParameter(m_audioUnit, kHALOutputParam_Volume, kAudioUnitScope_Input, 0, db, 0);
        BACKEND_ASSERT2(err == noErr, "Could not set volume on output audio unit.", FATAL_ERROR)
        emit volumeChanged(qreal(db));
    }
}

float AudioOutputAudioPart::volume()
{
    return m_volume;
}

////////////////////////////////////////////////////////////////////////////////////////

AudioOutput::AudioOutput(QObject *parent) : MediaNode(AudioSink, parent)
{
    m_audioOutput = new AudioOutputAudioPart();
    setAudioNode(m_audioOutput);
    connect(m_audioOutput, SIGNAL(volumeChanged(qreal)), this, SIGNAL(volumeChanged(qreal)));
    connect(m_audioOutput, SIGNAL(audioDeviceFailed()), this, SIGNAL(audioDeviceFailed()));
    m_redirectToMovie = false;
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::setVolume(qreal volume)
{
    IMPLEMENTED;
    m_audioOutput->setVolume(float(volume));
    
    if (m_redirectToMovie && m_owningMediaObject)
       m_owningMediaObject->setVolumeOnMovie(volume);
        
    emit volumeChanged(m_audioOutput->volume());
}

qreal AudioOutput::volume() const
{
    IMPLEMENTED;
    return qreal(m_audioOutput->volume());
}

bool AudioOutput::setOutputDevice(int device)
{
    IMPLEMENTED;
    if (device == -1)
        return false;
    if (int(m_audioOutput->m_audioDevice) == device)
        return true;
        
    if (m_redirectToMovie && m_owningMediaObject){
        m_audioOutput->setAudioDevice(device);
        return m_owningMediaObject->setAudioDeviceOnMovie(device);
    }
    
    if (audioGraph()){
        MediaNodeEvent event1(MediaNodeEvent::AboutToRestartAudioStream, this);
        audioGraph()->root()->notify(&event1);
    }
    m_audioOutput->setAudioDevice(device);
    if (audioGraph()){
        MediaNodeEvent event2(MediaNodeEvent::RestartAudioStreamRequest, this);
        audioGraph()->root()->notify(&event2);
    }
    return true;
}

int AudioOutput::outputDevice() const
{
    IMPLEMENTED;
    return m_audioOutput->m_audioDevice;
}

void AudioOutput::mediaNodeEvent(const MediaNodeEvent *event)
{
    switch (event->type()){
        case MediaNodeEvent::AudioGraphCannotPlay:
           m_redirectToMovie = true;
            m_audioOutput->setAudioDevice(m_audioOutput->m_audioDevice);
            if (m_owningMediaObject)
                m_owningMediaObject->setAudioDeviceOnMovie(m_audioOutput->m_audioDevice);
            setVolume(volume());
            break;
        case MediaNodeEvent::AudioGraphInitialized:
            m_redirectToMovie = false;
            m_audioOutput->setAudioDevice(m_audioOutput->m_audioDevice);
            if (m_owningMediaObject)
                m_owningMediaObject->setAudioDeviceOnMovie(0);
            setVolume(volume());
            break;
        default:
            break;
    }
}

}} //namespace Phonon::QT7

#include "moc_audiooutput.cpp"
