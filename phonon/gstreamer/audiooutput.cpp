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

#include "common.h"
#include "audiooutput.h"
#include "backend.h"
#include "mediaobject.h"
#include "gsthelper.h"
#include <phonon/audiooutput.h>

namespace Phonon
{
namespace Gstreamer
{
AudioOutput::AudioOutput(Backend *backend, QObject *parent)
        : QObject(parent)
        , MediaNode(backend, AudioSink)
        , m_volume(1.0)
        , m_device(0) // ### get from backend
        , m_volumeElement(0)
        , m_audioBin(0)
        , m_audioSink(0)
        , m_conv(0)
{
    static int count = 0;
    m_name = "AudioOutput" + QString::number(count++);

    if (m_backend->isValid()) {
        m_audioBin = gst_bin_new (NULL);
        gst_object_ref (m_audioBin);
    
        GstElement *queue = gst_element_factory_make ("queue", NULL);
        m_conv = gst_element_factory_make ("audioconvert", NULL);
    
        // Get category from parent
        Phonon::Category category = Phonon::NoCategory;
        if (Phonon::AudioOutput *audioOutput = qobject_cast<Phonon::AudioOutput *>(parent))
            category = audioOutput->category();
    
        m_audioSink = m_backend->deviceManager()->createAudioSink(category);
        m_volumeElement = gst_element_factory_make ("volume", NULL);
    
        if (queue && m_audioBin && m_conv && m_audioSink && m_volumeElement) {
            gst_bin_add_many (GST_BIN (m_audioBin), queue, m_conv, m_volumeElement, m_audioSink, NULL);
    
            if (gst_element_link_many (queue, m_conv, m_volumeElement, m_audioSink, NULL)) {
                // Add ghost sink for audiobin
                GstPad *audiopad = gst_element_get_pad (queue, "sink");
                gst_element_add_pad (m_audioBin, gst_ghost_pad_new ("sink", audiopad));
                gst_object_unref (audiopad);
                m_isValid = true; // Initialization ok, accept input
            }
        }
    }
}

void AudioOutput::mediaNodeEvent(const MediaNodeEvent *event)
{
    if (!m_audioBin)
        return;

    switch (event->type()) {

    default:
        break;
    }
}


AudioOutput::~AudioOutput()
{
    if (m_audioBin) {
        gst_element_set_state (m_audioBin, GST_STATE_NULL);
        gst_object_unref (m_audioBin);
    }
}

qreal AudioOutput::volume() const
{
    return m_volume;
}

int AudioOutput::outputDevice() const
{
    return m_device;
}

void AudioOutput::setVolume(qreal newVolume)
{
    if (newVolume > 2.0 )
        newVolume = 2.0;
    else if (newVolume < 0.0)
        newVolume = 0.0;

    if (newVolume == m_volume)
        return;

    m_volume = newVolume;

    if (m_volumeElement) {
        g_object_set(G_OBJECT(m_volumeElement), "volume", newVolume, NULL);
    }

    emit volumeChanged(newVolume);
}

bool AudioOutput::setOutputDevice(int newDevice)
{
    if (root()) {
        m_backend->logMessage("Cannot change output device on a linked audioOutput");
        return false;
    }

    const QList<AudioDevice> deviceList = m_backend->deviceManager()->audioOutputDevices();
    if (m_audioSink &&  newDevice >= 0 && newDevice < deviceList.size()) {
        QString deviceId = deviceList.at(newDevice).gstId;
        m_device = newDevice;
        GstHelper::setProperty(m_audioSink, "device", deviceId);
        // We check if the device can be opened by checking if it can go to the ready state
        if (gst_element_set_state(m_audioSink, GST_STATE_READY) == GST_STATE_CHANGE_SUCCESS) {
            return true;
        }
    }
    return false;
}

}
} //namespace Phonon::Gstreamer

#include "moc_audiooutput.cpp"
