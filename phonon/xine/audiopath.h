/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>

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
#ifndef Phonon_XINE_AUDIOPATH_H
#define Phonon_XINE_AUDIOPATH_H

#include <QObject>
#include <QList>
#include <xine.h>
#include "audioport.h"
#include "audiopostlist.h"

namespace Phonon
{
namespace Xine
{
		class AbstractAudioOutput;
    class MediaObject;
	class AudioOutput;
class XineStream;

	class AudioPath : public QObject
	{
		Q_OBJECT
		public:
			AudioPath( QObject* parent );
			~AudioPath();

            void addMediaObject(MediaObject *mp);
            void removeMediaObject(MediaObject *mp);
            QList<MediaObject *> mediaObjects() { return m_mediaObjects; }

			bool hasOutput() const;
            AudioPostList audioPostList() const { return m_effects; }
            //AudioPort audioPort(XineStream *forStream) const;
            void updateVolume(MediaObject *mp) const;

		public slots:
			bool addOutput( QObject* audioOutput );
			bool removeOutput( QObject* audioOutput );
			bool insertEffect( QObject* newEffect, QObject* insertBefore = 0 );
			bool removeEffect( QObject* effect );

        private slots:
            void audioPortChanged(const AudioPort &);

		private:
			AudioOutput *m_output;
            AudioPostList m_effects;
			QList<AbstractAudioOutput*> m_outputs;
            QList<MediaObject *> m_mediaObjects;
	};
}} //namespace Phonon::Xine

// vim: sw=4 ts=4 tw=80
#endif // Phonon_XINE_AUDIOPATH_H
