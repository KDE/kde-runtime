/*  This file is part of the KDE project
    Copyright (C) 2006-2008 Matthias Kretz <kretz@kde.org>

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

#include "audioport.h"
#include "audioport_p.h"
#include "xineengine.h"
#include "backend.h"
#include <QByteArray>
#include <QStringList>
#include <xine.h>
#include <xine/audio_out.h>
#include <QSharedData>
#include <kdebug.h>
#include <QtCore/QTimerEvent>
#include <QtGui/QApplication>

namespace Phonon
{
namespace Xine
{

AudioPortDeleter::AudioPortDeleter(AudioPortData *dd)
    : d(dd)
{
    moveToThread(QApplication::instance()->thread());
    XineEngine::addCleanupObject(this);
    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>(2345)));
}

bool AudioPortDeleter::event(QEvent *e)
{
    if (e->type() == 2345) {
        e->accept();
        startTimer(10000);
        return true;
    }
    return QObject::event(e);
}

void AudioPortDeleter::timerEvent(QTimerEvent *e)
{
    killTimer(e->timerId());
    deleteLater();
}

AudioPortDeleter::~AudioPortDeleter()
{
    XineEngine::removeCleanupObject(this);
}

AudioPortData::~AudioPortData()
{
    //kDebug(610) << this << " port = " << port;
    if (port) {
        if (!dontDelete) {
            xine_close_audio_driver(XineEngine::xine(), port);
        }
        port = 0;
        kDebug(610) << "----------------------------------------------- audio_port destroyed";
    }
}

AudioPort::AudioPort()
    : d(new AudioPortData)
{
}

AudioPort::AudioPort(const AudioPort &rhs)
    : d(rhs.d)
{
}

AudioPort &AudioPort::operator=(const AudioPort &rhs)
{
    waitALittleWithDying(); // xine still accesses the port after a rewire :(
    d = rhs.d;
    return *this;
}

AudioPort::~AudioPort()
{
    waitALittleWithDying(); // xine still accesses the port after a rewire :(
}

AudioPort::AudioPort(const AudioOutputDevice &deviceDesc)
    : d(new AudioPortData)
{
    QVariant v = deviceDesc.property("driver");
    if (!v.isValid()) {
        const QByteArray outputPlugin = XineEngine::audioDriverFor(deviceDesc.index());
        kDebug(610) << "use output plugin:" << outputPlugin;
        d->port = xine_open_audio_driver(XineEngine::xine(), outputPlugin.constData(), 0);
    } else {
        const QByteArray outputPlugin = v.toByteArray();
        v = deviceDesc.property("deviceIds");
        const QStringList deviceIds = v.toStringList();
        if (deviceIds.isEmpty()) {
            return;
        }
        //kDebug(610) << outputPlugin << alsaDevices;

        if (outputPlugin == "alsa") {
            foreach (const QString &device, deviceIds) {
                xine_cfg_entry_t alsaDeviceConfig;
                QByteArray deviceStr = device.toUtf8();
                if(!xine_config_lookup_entry(XineEngine::xine(), "audio.device.alsa_default_device",
                            &alsaDeviceConfig)) {
                    // the config key is not registered yet - it is registered when the alsa output
                    // plugin is opened. So we open the plugin and close it again, then we can set the
                    // setting. :(
                    xine_audio_port_t *port = xine_open_audio_driver(XineEngine::xine(), "alsa", 0);
                    if (port) {
                        xine_close_audio_driver(XineEngine::xine(), port);
                        // port == 0 does not have to be fatal, since it might be only the default device
                        // that cannot be opened
                    }
                    // now the config key should be registered
                    if(!xine_config_lookup_entry(XineEngine::xine(), "audio.device.alsa_default_device",
                                &alsaDeviceConfig)) {
                        kError(610) << "cannot set the ALSA device on Xine's ALSA output plugin";
                        return;
                    }
                }
                Q_ASSERT(alsaDeviceConfig.type == XINE_CONFIG_TYPE_STRING);
                alsaDeviceConfig.str_value = deviceStr.data();
                xine_config_update_entry(XineEngine::xine(), &alsaDeviceConfig);

                int err = xine_config_lookup_entry(XineEngine::xine(), "audio.device.alsa_front_device", &alsaDeviceConfig);
                Q_ASSERT(err);
                Q_ASSERT(alsaDeviceConfig.type == XINE_CONFIG_TYPE_STRING);
                alsaDeviceConfig.str_value = deviceStr.data();
                xine_config_update_entry(XineEngine::xine(), &alsaDeviceConfig);

                d->port = xine_open_audio_driver(XineEngine::xine(), "alsa", 0);
                if (d->port) {
                    kDebug(610) << "use ALSA device: " << device;
                    break;
                }
            }
        } else if (outputPlugin == "oss") {
            kDebug(610) << "use OSS output";
            d->port = xine_open_audio_driver(XineEngine::xine(), "oss", 0);
        }
    }
    kDebug(610) << "----------------------------------------------- audio_port created";
}

bool AudioPort::isValid() const
{
    return (d->port != 0);
}

bool AudioPort::operator==(const AudioPort &rhs) const
{
    return d->port == rhs.d->port;
}

bool AudioPort::operator!=(const AudioPort &rhs) const
{
    return d->port != rhs.d->port;
}

void AudioPort::waitALittleWithDying()
{
    if (d->ref == 1 && !d->dontDelete && !XineEngine::inShutdown()) {
        // this is the last ref to the data, so it will get deleted in a few instructions unless
        new AudioPortDeleter(d.data());
        // AudioPortDeleter refs it once more
    }
}

AudioPort::operator xine_audio_port_t *() const
{
    return d->port;
}

xine_audio_port_t *AudioPort::xinePort() const
{
    return d->port;
}

bool AudioPort::hasFailed() const
{
    if (!d->port) {
        return true;
    }
    const uint32_t cap = d->port->get_capabilities(d->port);
    if (cap == AO_CAP_NOCAP) {
        return true;
    }
    return false;
}

} // namespace Xine
} // namespace Phonon

#include "audioport_p.moc"
// vim: sw=4 sts=4 et tw=100
