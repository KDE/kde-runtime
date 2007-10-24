/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "audiooutput.h"
#include <QVector>
#include <QtCore/QCoreApplication>
#include <kdebug.h>

#include <sys/ioctl.h>
#include <iostream>
#include <QSet>
#include "mediaobject.h"
#include "backend.h"
#include "events.h"
#include "wirecall.h"
#include "xineengine.h"
#include "xinethread.h"

#undef assert

namespace Phonon
{
namespace Xine
{

#define K_XT(type) (static_cast<type *>(SinkNode::threadSafeObject().data()))
AudioOutput::AudioOutput(QObject *parent)
    : AbstractAudioOutput(new AudioOutputXT, parent),
    m_device(1)
{
}

AudioOutput::~AudioOutput()
{
    //kDebug(610) ;
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
    m_volume = newVolume;

    int xinevolume = static_cast<int>(m_volume * 100);
    if (xinevolume > 200) {
        xinevolume = 200;
    } else if (xinevolume < 0) {
        xinevolume = 0;
    }

    upstreamEvent(new UpdateVolumeEvent(xinevolume));
    emit volumeChanged(m_volume);
}

AudioPort AudioOutputXT::audioPort() const
{
    return m_audioPort;
}

bool AudioOutput::setOutputDevice(int newDevice)
{
    m_device = newDevice;
    K_XT(AudioOutputXT)->m_audioPort.setAudioOutput(0);
    K_XT(AudioOutputXT)->m_audioPort = AudioPort(m_device);
    if (!K_XT(AudioOutputXT)->m_audioPort.isValid()) {
        kDebug(610) << "new audio port is invalid";
        return false;
    }
    K_XT(AudioOutputXT)->m_audioPort.setAudioOutput(this);
    SourceNode *src = source();
    if (src) {
        QList<WireCall> wireCall;
        wireCall << WireCall(src, this);
        QCoreApplication::postEvent(XineEngine::thread(), new RewireEvent(wireCall));
    }
    return true;
}

void AudioOutput::downstreamEvent(Event *e)
{
    Q_ASSERT(e);
    QCoreApplication::sendEvent(this, e);
    SinkNode::downstreamEvent(e);
}

void AudioOutputXT::rewireTo(SourceNodeXT *source)
{
    if (!source->audioOutputPort()) {
        return;
    }
    source->assert();
    xine_post_wire_audio_port(source->audioOutputPort(), m_audioPort);
    source->assert();
    SinkNodeXT::assert();
}

bool AudioOutput::event(QEvent *ev)
{
    switch (ev->type()) {
    case Event::AudioDeviceFailed:
        ev->accept();
        emit audioDeviceFailed();
        return true;
    default:
        return AbstractAudioOutput::event(ev);
    }
}

void AudioOutput::graphChanged()
{
    kDebug(610);
    // we got connected to a new XineStream, it needs to know our m_volume
    int xinevolume = static_cast<int>(m_volume * 100);
    if (xinevolume > 200) {
        xinevolume = 200;
    } else if (xinevolume < 0) {
        xinevolume = 0;
    }
    upstreamEvent(new UpdateVolumeEvent(xinevolume));
}

#undef K_XT
}} //namespace Phonon::Xine

#include "audiooutput.moc"
// vim: sw=4 ts=4
