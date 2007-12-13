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

#include <gst/interfaces/propertyprobe.h>
#include "devicemanager.h"
#include "backend.h"
#include "gsthelper.h"
#include "videowidget.h"
#include "xvideowidget.h"

/*
 * This class manages the list of currently
 * active output devices
 */

namespace Phonon
{
namespace Gstreamer
{

AudioDevice::AudioDevice(DeviceManager *manager, const QString &gstId)
        : gstId(gstId)
{
    //get an id
    static int counter = 0;
    id = counter++;
    //get name from device
    if (gstId == "default") {
        description = "Default audio device";
    } else {
        GstElement *aSink= manager->createAudioSink();

        if (aSink) {
            gchar *deviceDescription = NULL;

            if (GST_IS_PROPERTY_PROBE(aSink) && gst_property_probe_get_property( GST_PROPERTY_PROBE(aSink), "device" ) ) {
                g_object_set (G_OBJECT(aSink), "device", gstId.toLatin1().constData(), NULL);
                g_object_get (G_OBJECT(aSink), "device-name", &deviceDescription, NULL);
                description = QString(deviceDescription);
                g_free (deviceDescription);
                gst_object_unref (aSink);
            }
        }
    }
}

DeviceManager::DeviceManager(Backend *backend)
        : QObject(backend)
        , m_backend(backend)
{
    m_audioSink = qgetenv("PHONON_GST_AUDIOSINK");
    if (m_audioSink.isEmpty())
        m_audioSink = "auto";

    m_videoSinkWidget = qgetenv("PHONON_GST_VIDEOWIDGET");
    if (m_videoSinkWidget.isEmpty())
        m_videoSinkWidget = "auto";

    if (m_backend->isValid())
        updateDeviceList();
}

DeviceManager::~DeviceManager()
{
    m_audioDeviceList.clear();
}

/***
* Returns a Gst Audiosink based on GNOME configuration settings,
* or 0 if the element is not available.
*/
GstElement *DeviceManager::createGNOMEAudioSink(Category category)
{
    GstElement *sink = gst_element_factory_make ("gconfaudiosink", NULL);

    if (sink) {

        // set profile property on the gconfaudiosink to "music and movies"
        if (g_object_class_find_property (G_OBJECT_GET_CLASS (sink), "profile")) {
            switch (category) {
            case NotificationCategory:
                g_object_set (G_OBJECT (sink), "profile", 0, NULL); // 0 = 'sounds'
                break;
            case CommunicationCategory:
                g_object_set (G_OBJECT (sink), "profile", 2, NULL); // 2 = 'chat'
                break;
            default:
                g_object_set (G_OBJECT (sink), "profile", 1, NULL); // 1 = 'music and movies'
                break;
            }
        }
    }
    return sink;
}


bool DeviceManager::canOpenDevice(GstElement *element) const
{
    if (!element)
        return false;

    if (gst_element_set_state(element, GST_STATE_READY) == GST_STATE_CHANGE_SUCCESS)
        return true;

    gst_element_set_state(element, GST_STATE_NULL);
    return false;
}

/*
*
* Returns a GstElement with a valid audio sink
* based on the current value of PHONON_GSTREAMER_DRIVER
*
* Allowed values are auto (default), alsa, oss, arts and ess
* does not exist
*
* If no real sound sink is available a fakesink will be returned
*/
GstElement *DeviceManager::createAudioSink(Category category)
{
    GstElement *sink = 0;

    if (m_backend && m_backend->isValid()) 
    {
        if (m_audioSink == "auto") //this is the default value
        {
            //### TODO : get equivalent KDE settings here
    
            if (!qgetenv("GNOME_DESKTOP_SESSION_ID").isEmpty()) {
                sink = createGNOMEAudioSink(category);
                if (canOpenDevice(sink))
                    m_backend->logMessage("AudioOutput using gconf audio sink");
                else if (sink) {
                    gst_object_unref(sink);
                    sink = 0;
                }
            }
    
            if (!sink) {
                sink = gst_element_factory_make ("alsasink", NULL);
                if (canOpenDevice(sink))
                    m_backend->logMessage("AudioOutput using alsa audio sink");
                else if (sink) {
                    gst_object_unref(sink);
                    sink = 0;
                }
            }

            if (!sink) {
                sink = gst_element_factory_make ("autoaudiosink", NULL);
                if (canOpenDevice(sink))
                    m_backend->logMessage("AudioOutput using auto audio sink");
                else if (sink) {
                    gst_object_unref(sink);
                    sink = 0;
                }
            }

            if (!sink) {
                sink = gst_element_factory_make ("osssink", NULL);
                if (canOpenDevice(sink))
                    m_backend->logMessage("AudioOutput using oss audio sink");
                else if (sink) {
                    gst_object_unref(sink);
                    sink = 0;
                }
            }
        } else if (m_audioSink == "fake") {
            //do nothing as a fakesink will be created by default
        } else if (!m_audioSink.isEmpty()) { //Use a custom sink
            sink = gst_element_factory_make (qPrintable(m_audioSink), NULL);
            if (canOpenDevice(sink))
                m_backend->logMessage(QString("AudioOutput using %0").arg(m_audioSink));
            else if (sink) {
                gst_object_unref(sink);
                sink = 0;
            }
        }
    }

    if (!sink) { //no suitable sink found so we'll make a fake one
        sink = gst_element_factory_make("fakesink", NULL);
        if (sink) {
            m_backend->logMessage("AudioOutput Using fake audio sink");
            //without sync the sink will pull the pipeline as fast as the CPU allows
            g_object_set (G_OBJECT (sink), "sync", TRUE, NULL);
        }
    }
    Q_ASSERT(sink);
    return sink;
}

QObject *DeviceManager::createVideoWidget(QWidget *parent)
{
    if (m_videoSinkWidget == "opengl") {
        return new VideoWidget(m_backend, parent, true);
    } else if (m_videoSinkWidget == "widget") {
        return new VideoWidget(m_backend, parent, false);
    }
#ifndef Q_WS_QWS
    else if (m_videoSinkWidget == "xwindow") {
        return new XVideoWidget(m_backend, parent);
    } else {
        GstElementFactory *srcfactory = gst_element_factory_find("ximagesink");
        if (srcfactory) {
            return new XVideoWidget(m_backend, parent);
        }
    }
#endif
    return new VideoWidget(m_backend, parent, true);
}

/*
 * Returns a positive device id or -1 if device
 * does not exist
 *
 * The gstId is typically in the format hw:1,0
 */
int DeviceManager::deviceId(const QString &gstId) const
{
    for (int i = 0 ; i < m_audioDeviceList.size() ; ++i) {
        if (m_audioDeviceList[i].gstId == gstId) {
            return m_audioDeviceList[i].id;
        }
    }
    return -1;
}

/**
 * Get a human-readable description from a device id
 */
QString DeviceManager::deviceDescription(int id) const
{
    for (int i = 0 ; i < m_audioDeviceList.size() ; ++i) {
        if (m_audioDeviceList[i].id == id) {
            return m_audioDeviceList[i].description;
        }
    }
    return QString();
}

/**
 * Updates the current list of active devices
 */
void DeviceManager::updateDeviceList()
{
    //fetch list of current devices
    GstElement *audioSink= createAudioSink();

    QStringList list;

    if (audioSink) {
        list = GstHelper::extractProperties(audioSink, "device");
        list.prepend("default");

        for (int i = 0 ; i < list.size() ; ++i) {
            QString gstId = list.at(i);
            if (deviceId(gstId) == -1) {
                // This is a new device, add it
                m_audioDeviceList.append(AudioDevice(this, gstId));
                emit deviceAdded(deviceId(gstId));
                m_backend->logMessage(QString("Found new audio device %0").arg(gstId), Backend::Debug, this);
            }
        }

        if (list.size() < m_audioDeviceList.size()) {
            //a device was removed
            for (int i = m_audioDeviceList.size() -1 ; i >= 0 ; --i) {
                QString currId = m_audioDeviceList[i].gstId;
                bool found = false;
                for (int k = list.size() -1  ; k >= 0 ; --k) {
                    if (currId == list[k]) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    m_backend->logMessage(QString("Audio device lost %0").arg(currId), Backend::Debug, this);
                    emit deviceRemoved(deviceId(currId));
                    m_audioDeviceList.removeAt(i);
                }
            }
        }
    }

    gst_element_set_state (audioSink, GST_STATE_NULL);
    gst_object_unref (audioSink);
}

/**
  * Returns a list of hardware id usable by gstreamer [i.e hw:1,0]
  */
const QList<AudioDevice> DeviceManager::audioOutputDevices() const
{
    return m_audioDeviceList;
}

}
}
