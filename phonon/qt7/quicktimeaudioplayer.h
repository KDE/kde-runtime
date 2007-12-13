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

#ifndef Phonon_QT7_QUICKTIMEAUDIOPLAYER_H
#define Phonon_QT7_QUICKTIMEAUDIOPLAYER_H

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include <phonon/mediasource.h>
#include <Carbon/Carbon.h>
#include "audionode.h"

#include <QtCore>

namespace Phonon
{
namespace QT7
{
    class AudioGraph;
    class MediaNodeEvent;

    class QuickTimeAudioPlayer : public AudioNode
    {
        public:
            enum State {Playing, Paused, NoMedia};

            QuickTimeAudioPlayer();
            virtual ~QuickTimeAudioPlayer();

            void play();
            void pause(qint64 seekToMs = -1);
            void seek(qint64 milliseconds);

            void setVideo(Movie movieRef);
            void unsetVideo();

            bool hasAudio();
            bool isPlaying();
            void doRegularTasks();
            quint64 currentTime();
            qint64 getLongestSoundtrackDurationInMs();

            ComponentDescription getAudioNodeDescription();
            void initializeAudioUnit();
            bool fillInStreamSpecification(AudioConnection *connection, ConnectionSide side);
            void mediaNodeEvent(const MediaNodeEvent *event);

        private:
            void initSoundExtraction();
            void newGraphNotification();
            void allocateSoundSlices();
            void scheduleSoundSlices();
            void deleteOldSliceLists();

            State m_state;
            Movie m_movieRef;
            MovieAudioExtractionRef m_audioExtractionRef;
            ScheduledAudioSlice *m_sliceList;
            QList<ScheduledAudioSlice *>m_slicesToBeDeleted;

            AudioChannelLayout *m_audioChannelLayout;
        	UInt32 m_audioChannelLayoutSize;
            AudioStreamBasicDescription m_audioStreamDescription;

            bool m_discrete;
            bool m_playerUnitStarted;
            bool m_hasAudio;
            bool m_audioExtractionComplete;

            long m_samplesRemaining;
            long m_extractionStart;
            int m_sliceCount;
            int m_maxExtractionPacketCount;

            Float64 m_sampleTimeStamp;
            qint64 m_startTime;
            qint64 m_duration;
    };

}} // namespace Phonon::QT7

#endif // Phonon_QT7_QUICKTIMEAUDIOPLAYER_H
