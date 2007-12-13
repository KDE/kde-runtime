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

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QTimer>
#include <phonon/audiooutputinterface.h>
#include <gst/gst.h>

namespace Phonon {
namespace Gstreamer {
class Backend;
class DeviceManager;

class AudioDevice {
public :
    AudioDevice(DeviceManager *s, const QString &deviceId);
    int id;
    QString gstId;
    QString description;
};

class DeviceManager : public QObject {
    Q_OBJECT
public:
    DeviceManager(Backend *parent);
    virtual ~DeviceManager();
    const QList<AudioDevice> audioOutputDevices() const;
    GstPad *requestPad(int device) const;
    int deviceId(const QString &gstId) const;
    QString deviceDescription(int id) const;
    GstElement *createGNOMEAudioSink(Category category);
    GstElement *createAudioSink(Category category = NoCategory);
    QObject *createVideoWidget(QWidget *parent);

signals:
    void deviceAdded(int);
    void deviceRemoved(int);

public slots:
    void updateDeviceList();

private:
    bool canOpenDevice(GstElement *element) const;
    Backend *m_backend;
    QList <AudioDevice> m_audioDeviceList;
    QTimer m_devicePollTimer;
    QString m_audioSink;
    QString m_videoSinkWidget;
};
}
} // namespace Phonon::Gstreamer

#endif // DEVICEMANAGER_H
