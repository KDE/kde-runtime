/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
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
#ifndef Phonon_XINE_AUDIOOUTPUT_H
#define Phonon_XINE_AUDIOOUTPUT_H

#include "abstractaudiooutput.h"
#include <QFile>

#include "xineengine.h"
#include <xine.h>
#include "xinestream.h"
#include "audioport.h"
#include <phonon/audiooutputinterface.h>

namespace Phonon
{
namespace Xine
{

class AudioOutputXT : public SinkNodeXT
{
    friend class AudioOutput;
    public:
        void rewireTo(SourceNodeXT *);
        AudioPort audioPort() const;

    private:
        AudioPort m_audioPort;
};

    class MediaObject;
    class AudioOutput : public AbstractAudioOutput, public AudioOutputInterface
	{
		Q_OBJECT
        Q_INTERFACES(Phonon::AudioOutputInterface)
		public:
            AudioOutput(QObject *parent);
			~AudioOutput();

            void updateVolume(MediaObject *mp) const;

			// Attributes Getters:
            qreal volume() const;
			int outputDevice() const;

			// Attributes Setters:
            void setVolume(qreal newVolume);
            bool setOutputDevice(int newDevice);

            void downstreamEvent(QEvent *);

        protected:
            bool event(QEvent *);

		Q_SIGNALS:
            // for the Frontend
            void volumeChanged(qreal newVolume);
            void audioDeviceFailed();

            // internal
            void audioPortChanged(const AudioPort &);

		private:
            MediaObject *findMediaObject() const;
            qreal m_volume;
			int m_device;
	};
}} //namespace Phonon::Xine

// vim: sw=4 ts=4 tw=80
#endif // Phonon_XINE_AUDIOOUTPUT_H
