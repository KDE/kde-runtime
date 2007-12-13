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

#ifndef Phonon_QT7_QUICKTIMEVIDEOPLAYER_H
#define Phonon_QT7_QUICKTIMEVIDEOPLAYER_H

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include <phonon/mediasource.h>
#include <Carbon/Carbon.h>
#include <QtCore>
#include "videoframe.h"

namespace Phonon
{
namespace QT7
{
    class QuickTimeVideoPlayer : QObject
    {
        public:
            enum StateEnum {
                Playing = 0x1,
                Paused = 0x2,
                NoMedia = 0x4,
            };
            Q_DECLARE_FLAGS(State, StateEnum);

            QuickTimeVideoPlayer();
            virtual ~QuickTimeVideoPlayer();

            void setMediaSource(const MediaSource &source);
            MediaSource mediaSource() const;
            void unsetVideo();

            void play();
            void pause();
            void seek(qint64 milliseconds);

            bool videoFrameChanged(const CVTimeStamp &timeStamp) const;
            void doRegularTasks();
            CVOpenGLTextureRef createCvTexture(const CVTimeStamp &timeStamp);
            QRect videoRect() const;

            bool hasAudio() const;
            bool hasVideo() const;

            bool preRollMovie(qint64 startTime = 0);
            void moviePreRolled(float percentage);

            qint64 duration() const;
            qint64 currentTime() const;

            void setColors(qreal brightness = 0, qreal contrast = 1, qreal hue = 0, qreal saturation = 1);
            void setVolume(float volume);
            bool setAudioDevice(int id);
            void setPlaybackRate(float rate);
            
            float playbackRate() const;
            float prefferedPlaybackRate() const;

            Movie movieRef() const;
            QuickTimeVideoPlayer::State state() const;
            bool canPlayMedia() const;
            bool isPlaying();

        protected:
            bool event(QEvent *event);

        private:
            Movie m_movieRef;
            State m_state;
            bool m_playbackRateSat;
            float m_playbackRate;
            MediaSource m_mediaSource;
            int m_mediaUpdateTimer;
            QTVisualContextRef m_visualContext;
            VideoFrame m_currentFrame;

            void createVisualContext();
            bool openMovieFromFile(const MediaSource &mediaSource);
            bool openMovieFromUrl(const MediaSource &mediaSource);
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(QuickTimeVideoPlayer::State);

}} // namespace Phonon::QT7

#endif // Phonon_QT7_QUICKTIMEVIDEOPLAYER_H
