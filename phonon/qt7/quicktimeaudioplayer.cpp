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

#include "quicktimeaudioplayer.h"
#include "backendheader.h"
#include "audiograph.h"
#include "medianodeevent.h"
#include "medianode.h"

namespace Phonon
{
namespace QT7
{

QuickTimeAudioPlayer::QuickTimeAudioPlayer() : AudioNode(0, 1)
{
    m_state = NoMedia;
    m_movieRef = 0;
    m_audioExtractionRef = 0;
    m_audioChannelLayout = 0;
    m_sliceList = 0;
    m_sliceCount = 30;
    m_maxExtractionPacketCount = 4096;
    m_audioExtractionComplete = false;
    m_samplesRemaining = -1;
    m_extractionStart = 0;
    m_startTime = 0;
    m_sampleTimeStamp = 0;
    m_hasAudio = false;
    m_duration = 0;
}

QuickTimeAudioPlayer::~QuickTimeAudioPlayer()
{
    unsetVideo();
}

void QuickTimeAudioPlayer::unsetVideo()
{
    if (!m_movieRef)
        return;

    if (m_audioUnit){
        OSStatus err = AudioUnitReset(m_audioUnit, kAudioUnitScope_Global, 0);
        BACKEND_ASSERT2(err == noErr, "Could not reset audio player unit when unsetting movie", FATAL_ERROR)
    }

    if (m_audioExtractionRef)
	    MovieAudioExtractionEnd(m_audioExtractionRef);

    if (m_audioChannelLayout){
        free(m_audioChannelLayout);
        m_audioChannelLayout = 0;
    }
    if (m_sliceList){
        m_slicesToBeDeleted << m_sliceList;
        m_sliceList = 0;
    }
    deleteOldSliceLists();
    m_movieRef = 0;
    m_audioExtractionRef = 0;
    m_audioExtractionComplete = false;
    m_samplesRemaining = -1;
    m_extractionStart = 0;
    m_sampleTimeStamp = 0;
    m_hasAudio = false;
    m_state = NoMedia;
}

void QuickTimeAudioPlayer::deleteOldSliceLists()
{
    // Since audio buffers might be scheduled for playback at the
    // audio device, I don't want to free them up prematurely. Therefore
    // I use the garbage list that I run everey now and then.
    for (int listIndex=0; listIndex<m_slicesToBeDeleted.size(); listIndex++){
	    ScheduledAudioSlice *slices = m_slicesToBeDeleted[listIndex];
	    bool readyToBeDeleted = true;
            for (int i=0; i<m_sliceCount; i++){
                if (!(slices[i].mFlags & kScheduledAudioSliceFlag_Complete)){
	    	readyToBeDeleted = false;
	    	break;
	        }
	    }

	    if (readyToBeDeleted){
	        m_slicesToBeDeleted.removeAt(listIndex);
	        --listIndex; // Compensate for the removal.
            for (int i=0; i<m_sliceCount; i++)
	            free(slices[i].mBufferList);
            free(slices);
        }
    }
}

void QuickTimeAudioPlayer::setVideo(Movie movieRef)
{
    unsetVideo();
    if (movieRef){
        m_movieRef = movieRef;
	    m_hasAudio = (GetMovieIndTrackType(
	        m_movieRef, 1, AudioMediaCharacteristic,
	        movieTrackCharacteristic | movieTrackEnabledOnly) != 0);
        m_duration = getLongestSoundtrackDurationInMs();
        initSoundExtraction();
        allocateSoundSlices();
        m_state = Paused;
        seek(0);
    }
}

void QuickTimeAudioPlayer::doRegularTasks()
{
    deleteOldSliceLists();

    if (!m_movieRef)
        return;

    if (!m_audioExtractionComplete){
        if (m_state == Playing)
            scheduleSoundSlices();
    }
}

void QuickTimeAudioPlayer::pause(qint64 seekToMs)
{
    if (!m_movieRef)
        return;

    m_state = Paused;
    m_startTime = (seekToMs == -1) ? currentTime() : seekToMs;
    if (m_audioUnit){
        OSStatus err = AudioUnitReset(m_audioUnit, kAudioUnitScope_Global, 0);
        BACKEND_ASSERT2(err == noErr, "Could not reset audio player unit on pause", FATAL_ERROR)
    }
}

void QuickTimeAudioPlayer::play()
{
    if (!m_movieRef || m_state == Playing)
        return;

    m_state = Playing;
    seek(m_startTime);
}

bool QuickTimeAudioPlayer::isPlaying()
{
    return m_movieRef && m_state == Playing;
}

void QuickTimeAudioPlayer::seek(qint64 milliseconds)
{
    if (!m_movieRef)
        return;

    if (milliseconds > m_duration)
        milliseconds = m_duration;

    m_startTime = milliseconds;
    if (!m_movieRef || !m_audioUnit)
        return;

    // Reset (and stop playing):
    OSStatus err = AudioUnitReset(m_audioUnit, kAudioUnitScope_Global, 0);
    m_sampleTimeStamp = 0;
     for (int i = 0; i < m_sliceCount; i++)
	    m_sliceList[i].mFlags = kScheduledAudioSliceFlag_Complete;

    // Start to play again immidiatly:
    AudioTimeStamp timeStamp;
    memset(&timeStamp, 0, sizeof(timeStamp));
	timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    timeStamp.mSampleTime = -1;
	err = AudioUnitSetProperty(m_audioUnit,
        kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global,
        0, &timeStamp, sizeof(timeStamp));
    BACKEND_ASSERT2(err == noErr, "Could not set schedule start time stamp on audio player unit", FATAL_ERROR)

    // Seek back to 'now' in the movie:
    TimeRecord timeRec;
	timeRec.scale = GetMovieTimeScale(m_movieRef);
    timeRec.base = 0;
	timeRec.value.hi = 0;
	timeRec.value.lo = (milliseconds / 1000.0f) * timeRec.scale;
	err = MovieAudioExtractionSetProperty(m_audioExtractionRef,
        kQTPropertyClass_MovieAudioExtraction_Movie,
        kQTMovieAudioExtractionMoviePropertyID_CurrentTime,
        sizeof(TimeRecord), &timeRec);
    BACKEND_ASSERT2(err == noErr, "Could not set current time on audio player unit", FATAL_ERROR)

    float durationLeftSec = float(m_duration - milliseconds) / 1000.0f;
    m_samplesRemaining = (durationLeftSec > 0) ? (durationLeftSec * m_audioStreamDescription.mSampleRate) : -1;
    m_extractionStart = float(milliseconds / 1000.0f) * m_audioStreamDescription.mSampleRate;
    m_audioExtractionComplete = false;
}

quint64 QuickTimeAudioPlayer::currentTime()
{
    if (!m_movieRef || !m_audioUnit)
        return m_startTime;

    Float64 currentUnitTime = getTimeInSamples(kAudioUnitProperty_CurrentPlayTime);
    if (currentUnitTime == -1)
        currentUnitTime = 0;

    return quint64(m_startTime + float(currentUnitTime / m_audioStreamDescription.mSampleRate) * 1000.0f);
}

bool QuickTimeAudioPlayer::hasAudio()
{
    return m_hasAudio;
}

ComponentDescription QuickTimeAudioPlayer::getAudioNodeDescription()
{
	ComponentDescription description;
	description.componentType = kAudioUnitType_Generator;
	description.componentSubType = kAudioUnitSubType_ScheduledSoundPlayer;
	description.componentManufacturer = kAudioUnitManufacturer_Apple;
	description.componentFlags = 0;
	description.componentFlagsMask = 0;
    return description;
}

void QuickTimeAudioPlayer::initializeAudioUnit()
{
}

bool QuickTimeAudioPlayer::fillInStreamSpecification(AudioConnection *connection, ConnectionSide side)
{
    if (!m_movieRef){
        if (side == Source)
            DEBUG_AUDIO_STREAM("QuickTimeAudioPlayer" << int(this) << "is source, but has no movie to use for stream spec fill.")
        return true;
    }

    if (side == Source){
        DEBUG_AUDIO_STREAM("QuickTimeAudioPlayer" << int(this) << "is source, and fills in stream spec from movie.")
        connection->m_sourceStreamDescription = m_audioStreamDescription;
        connection->m_sourceChannelLayout = (AudioChannelLayout *) malloc(m_audioChannelLayoutSize);
        memcpy(connection->m_sourceChannelLayout, m_audioChannelLayout, m_audioChannelLayoutSize);
        connection->m_sourceChannelLayoutSize = m_audioChannelLayoutSize;
        connection->m_hasSourceSpecification = true;
    }
    return true;
}

qint64 QuickTimeAudioPlayer::getLongestSoundtrackDurationInMs()
{
    if (!m_movieRef)
        return 0;

    TimeValue maxDuration = 0;
    int ntracks = GetMovieTrackCount(m_movieRef);
    if (ntracks){
        for (int i=1; i<ntracks + 1; ++i) {
            Track track = GetMovieIndTrackType(m_movieRef, i, SoundMediaType, movieTrackMediaType);
            if (track){
                TimeValue duration = GetTrackDuration(track);
                if (duration > maxDuration) maxDuration = duration;
            }
        }
        return 1000 * (float(maxDuration) / float(GetMovieTimeScale(m_movieRef)));
    }
    return 0;
}

void QuickTimeAudioPlayer::initSoundExtraction()
{
    // Initilize the extraction:
	OSStatus err = noErr;
	err = MovieAudioExtractionBegin(m_movieRef, 0, &m_audioExtractionRef);
    BACKEND_ASSERT2(err == noErr, "Could not start audio extraction on audio player unit", FATAL_ERROR)

	m_discrete = false;
#if 0
    // Extract all channels as descrete:
    err = MovieAudioExtractionSetProperty(audioExtractionRef,
        kQTPropertyClass_MovieAudioExtraction_Movie,
        kQTMovieAudioExtractionMoviePropertyID_AllChannelsDiscrete,
        sizeof (discrete),
        &discrete);
    BACKEND_ASSERT2(err == noErr, "Could not set channels discrete on audio player unit", FATAL_ERROR)
#endif

	// Get the size of the audio channel layout (may include offset):
	err = MovieAudioExtractionGetPropertyInfo(m_audioExtractionRef,
	    kQTPropertyClass_MovieAudioExtraction_Audio,
        kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
        0, &m_audioChannelLayoutSize, 0);
    BACKEND_ASSERT2(err == noErr, "Could not get channel layout size from audio extraction", FATAL_ERROR)

	// Allocate memory for the layout
	m_audioChannelLayout = (AudioChannelLayout *) calloc(1, m_audioChannelLayoutSize);
    BACKEND_ASSERT2(m_audioChannelLayout, "Could not allocate memory for channel layout on audio player unit", FATAL_ERROR)

	// Get the layout:
	err = MovieAudioExtractionGetProperty(m_audioExtractionRef,
	    kQTPropertyClass_MovieAudioExtraction_Audio,
		kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
		m_audioChannelLayoutSize, m_audioChannelLayout, 0);
    BACKEND_ASSERT2(err == noErr, "Could not get channel layout from audio extraction", FATAL_ERROR)

	// Get audio stream description:
	err = MovieAudioExtractionGetProperty(m_audioExtractionRef,
        kQTPropertyClass_MovieAudioExtraction_Audio,
        kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
        sizeof(m_audioStreamDescription), &m_audioStreamDescription, 0);
    BACKEND_ASSERT2(err == noErr, "Could not get audio stream description from audio extraction", FATAL_ERROR)
}

void QuickTimeAudioPlayer::allocateSoundSlices()
{
    // m_sliceList will contain a specified number of ScheduledAudioSlice-s that each can
    // carry audio from extraction, and be scheduled for playback at an audio unit.
    // Each ScheduledAudioSlice will contain several audio buffers, one for each sound channel.
    // Each buffer will carry (at most) a specified number of sound packets, and each packet can
    // contain one or more frames.

    // Create a list of ScheduledAudioSlices:
	m_sliceList = (ScheduledAudioSlice *) calloc(m_sliceCount, sizeof(ScheduledAudioSlice));
    BACKEND_ASSERT2(m_sliceList, "Could not allocate memory for audio slices", FATAL_ERROR)
	bzero(m_sliceList, m_sliceCount * sizeof(ScheduledAudioSlice));

    // Calculate the size of the different structures needed:
	int packetsBufferSize = m_maxExtractionPacketCount * m_audioStreamDescription.mBytesPerPacket;
    int channels = m_audioStreamDescription.mChannelsPerFrame;
	int audioBufferListSize = int(sizeof(AudioBufferList) + (channels-1) * sizeof(AudioBuffer));
	int mallocSize = audioBufferListSize + (packetsBufferSize * m_audioStreamDescription.mChannelsPerFrame);

    // Round off to Altivec sizes:
    packetsBufferSize = int(((packetsBufferSize + 15) / 16) * 16);
    audioBufferListSize = int(((audioBufferListSize + 15) / 16) * 16);

 	for (int sliceIndex = 0; sliceIndex < m_sliceCount; ++sliceIndex){
        // Create the memory chunk for this audio slice:
		AudioBufferList	*audioBufferList = (AudioBufferList*) calloc(1, mallocSize);
        BACKEND_ASSERT2(audioBufferList, "Could not allocate memory for audio buffer list", FATAL_ERROR)

        // The AudioBufferList contains an AudioBuffer for each channel in the audio stream:
		audioBufferList->mNumberBuffers = m_audioStreamDescription.mChannelsPerFrame;
		for (uint i = 0; i < audioBufferList->mNumberBuffers; ++i){
			audioBufferList->mBuffers[i].mNumberChannels = 1;
			audioBufferList->mBuffers[i].mData = (char *) audioBufferList + audioBufferListSize + (i * packetsBufferSize);
			audioBufferList->mBuffers[i].mDataByteSize = packetsBufferSize;
		}

		m_sliceList[sliceIndex].mBufferList = audioBufferList;
		m_sliceList[sliceIndex].mNumberFrames = m_maxExtractionPacketCount;
		m_sliceList[sliceIndex].mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;
		m_sliceList[sliceIndex].mCompletionProcUserData = 0;
		m_sliceList[sliceIndex].mCompletionProc = 0;
		m_sliceList[sliceIndex].mFlags = kScheduledAudioSliceFlag_Complete;
		m_sliceList[sliceIndex].mReserved = 0;
	}
}

void QuickTimeAudioPlayer::scheduleSoundSlices()
{
	// For each completed (or never used) slice, fill and schedule it.
	for (int sliceIndex = 0; sliceIndex < m_sliceCount; ++sliceIndex){
		if (m_sliceList[sliceIndex].mFlags & kScheduledAudioSliceFlag_Complete){
			if (m_samplesRemaining == 0)
				m_audioExtractionComplete = true;

			if (!m_audioExtractionComplete){
			    // Determine how many samples to read:
				int samplesCount = m_samplesRemaining;
				if ((samplesCount > m_maxExtractionPacketCount) || (samplesCount == -1))
					samplesCount = m_maxExtractionPacketCount;
				m_sliceList[sliceIndex].mTimeStamp.mSampleTime = m_sampleTimeStamp;

	            // Reset buffer sizes:
	            int byteSize = samplesCount * m_audioStreamDescription.mBytesPerPacket;
	            for (uint i = 0; i < m_sliceList[sliceIndex].mBufferList->mNumberBuffers; ++i)
                    m_sliceList[sliceIndex].mBufferList->mBuffers[i].mDataByteSize = byteSize;

	            // Do the extraction:
                UInt32 flags = 0;
                UInt32 samplesRead = samplesCount;
	            OSStatus err = MovieAudioExtractionFillBuffer(
	                m_audioExtractionRef, &samplesRead, m_sliceList[sliceIndex].mBufferList, &flags);
                BACKEND_ASSERT2(err == noErr, "Could not fill audio buffers from audio extraction", FATAL_ERROR)
	            m_audioExtractionComplete = (flags & kQTMovieAudioExtractionComplete);

	            // Play the slice:
	            if (samplesRead != 0 && m_audioUnit != 0){
                    m_sliceList[sliceIndex].mNumberFrames = samplesRead;
                    err = AudioUnitSetProperty(m_audioUnit,
                        kAudioUnitProperty_ScheduleAudioSlice, kAudioUnitScope_Global,
                        0, &m_sliceList[sliceIndex], sizeof(ScheduledAudioSlice));
                    BACKEND_ASSERT2(err == noErr, "Could not schedule audio buffers on audio unit", FATAL_ERROR)
 	            } else
					m_sliceList[sliceIndex].mFlags = kScheduledAudioSliceFlag_Complete;

                // Move the window:
				m_sampleTimeStamp += samplesRead;
				if (m_samplesRemaining != -1)
					m_samplesRemaining -= samplesRead;
			}
		}
	}
}

void QuickTimeAudioPlayer::mediaNodeEvent(const MediaNodeEvent *event)
{
    switch (event->type()){
        case MediaNodeEvent::AudioGraphAboutToBeDeleted:
        case MediaNodeEvent::AboutToRestartAudioStream:
            m_startTime = currentTime();
            break;
        case MediaNodeEvent::AudioGraphInitialized:
        case MediaNodeEvent::RestartAudioStreamRequest:
            if (m_state == Playing)
                seek(m_startTime);
            break;
       default:
            break;
    }
}

}}



