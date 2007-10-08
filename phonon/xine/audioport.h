/*  This file is part of the KDE project
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PHONON_XINE_AUDIOPORT_H
#define PHONON_XINE_AUDIOPORT_H

#include <QSharedDataPointer>
#include <xine.h>
#include <QSharedData>
#include <QObject>


namespace Phonon
{
namespace Xine
{

class AudioPortData : public QSharedData
{
    public:
        AudioPortData() : port(0), audioOutput(0), dontDelete(false) {}
        ~AudioPortData();

        xine_audio_port_t *port;
        QObject *audioOutput;
        bool dontDelete;
};

class AudioPort
{
    friend class NullSinkXT;
    friend class EffectXT;
    public:
        AudioPort();
        AudioPort(int deviceIndex);

        bool isValid() const;
        bool operator==(const AudioPort &rhs) const;
        bool operator!=(const AudioPort &rhs) const;

        void waitALittleWithDying();

        operator xine_audio_port_t *() const;
        xine_audio_port_t *xinePort() const;

        /**
         * used to send XINE_EVENT_AUDIO_DEVICE_FAILED to the AudioOutput
         */
        void setAudioOutput(QObject *audioOutput);
        QObject *audioOutput() const;

        AudioPort(const AudioPort &);
        AudioPort &operator=(const AudioPort &);
        ~AudioPort();

    private:
        QExplicitlySharedDataPointer<AudioPortData> d;
};

} // namespace Xine
} // namespace Phonon
#endif // PHONON_XINE_AUDIOPORT_H
