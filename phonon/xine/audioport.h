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

#ifndef PHONON_XINE_AUDIOPORT_H
#define PHONON_XINE_AUDIOPORT_H

#include <QSharedDataPointer>
#include <xine.h>
#include <QSharedData>
#include <QObject>
#include <Phonon/AudioOutputDevice>


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
        AudioPort(const AudioOutputDevice &deviceDesc);

        bool isValid() const;
        bool operator==(const AudioPort &rhs) const;
        bool operator!=(const AudioPort &rhs) const;

        void waitALittleWithDying();

        operator xine_audio_port_t *() const;
        xine_audio_port_t *xinePort() const;

        bool hasFailed() const;

        AudioPort(const AudioPort &);
        AudioPort &operator=(const AudioPort &);
        ~AudioPort();

    private:
        QExplicitlySharedDataPointer<AudioPortData> d;
};

} // namespace Xine
} // namespace Phonon
#endif // PHONON_XINE_AUDIOPORT_H
