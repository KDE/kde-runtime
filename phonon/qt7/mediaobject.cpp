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

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include "mediaobject.h"
#include "backendheader.h"
#include "videowidget.h"
#include "videoframe.h"
#include "audiooutput.h"
#include "quicktimeaudioplayer.h"
#include "quicktimevideoplayer.h"
#include "quicktimemetadata.h"
#include "displaylinkcallback.h"
#include "audiograph.h"
#include "mediaobjectaudionode.h"

#include <QMutexLocker>

namespace Phonon
{
namespace QT7
{

MediaObject::MediaObject(QObject *parent) : MediaNode(AudioSource | VideoSource, parent)
{
    m_owningMediaObject = this;
    m_state = Phonon::StoppedState;

    m_videoPlayer = new QuickTimeVideoPlayer();
    m_audioPlayer = new QuickTimeAudioPlayer();
    m_nextVideoPlayer = new QuickTimeVideoPlayer();
    m_nextAudioPlayer = new QuickTimeAudioPlayer();
    m_mediaObjectAudioNode = new MediaObjectAudioNode(m_audioPlayer, m_nextAudioPlayer);
    setAudioNode(m_mediaObjectAudioNode);

    m_metaData = new QuickTimeMetaData();
    m_audioGraph = new AudioGraph(this);
    m_displayLink = new DisplayLinkCallback(this);

    m_tickTimer = 0;
    m_tickInterval = 0;
    m_prefinishMark = 0;
    m_currentTime = 0;
    m_transitionTime = 0;
    m_playListIndex = 0;
    m_waitNextSwap = false;
    checkForError();
}

MediaObject::~MediaObject()
{
   QMutexLocker locker(&m_quickTimeGuard);
        delete m_displayLink;
        delete m_videoPlayer;
        delete m_nextVideoPlayer;
        delete m_metaData;
        // m_mediaObjectAudioNode is owned by super class.    
    locker.unlock();
    checkForError();
}

void MediaObject::setState(Phonon::State state)
{
    Phonon::State prevState = m_state;
    m_state = state;
    if (prevState != m_state)
        emit stateChanged(m_state, prevState);
}

void MediaObject::setSource(const MediaSource &source)
{
    IMPLEMENTED;
    setState(Phonon::LoadingState);
    
    // Save current state for event/signal handling below:
    bool prevHasVideo = m_videoPlayer->hasVideo();
    qint64 prevTotalTime = totalTime();
    m_waitNextSwap = false;
    
    QMutexLocker locker(&m_quickTimeGuard);
        // Cancel cross-fade if any:
        m_nextVideoPlayer->pause();
        m_nextAudioPlayer->pause();
        m_videoPlayer->setVolume(0);
        m_nextVideoPlayer->setVolume(0);
        m_mediaObjectAudioNode->cancelCrossFade();

        // Set new source:
        m_videoPlayer->setMediaSource(source);
        m_audioPlayer->setVideo(m_videoPlayer->movieRef());
        m_metaData->setVideo(m_videoPlayer->movieRef());        
        m_audioGraph->updateStreamSpecifications();
            
        m_nextAudioPlayer->unsetVideo();
        m_nextVideoPlayer->unsetVideo();
    locker.unlock();
    m_currentTime = 0;

    if (m_videoPlayer->canPlayMedia())
        setState(Phonon::PausedState);
    else{
        gSetErrorString("Cannot play current media.");
        gSetErrorType(NORMAL_ERROR);
        gSetErrorLocation(ERROR_LOCATION);
        checkForError();
    }

    // Emit/notify information about the new source:
    QRect videoRect = m_videoPlayer->videoRect();
    MediaNodeEvent e1(MediaNodeEvent::VideoFrameSizeChanged, &videoRect);
    notify(&e1);

    emit currentSourceChanged(source);
    emit metaDataChanged(m_metaData->metaData());

    if (prevHasVideo != m_videoPlayer->hasVideo())
        emit hasVideoChanged(m_videoPlayer->hasVideo());
    if (prevTotalTime != totalTime())
        emit totalTimeChanged(totalTime());

    checkForError();
}

void MediaObject::setNextSource(const MediaSource &source)
{
    IMPLEMENTED;
    QMutexLocker locker(&m_quickTimeGuard);
        m_nextVideoPlayer->setMediaSource(source);
        m_nextAudioPlayer->setVideo(m_nextVideoPlayer->movieRef());
    locker.unlock();
    checkForError();
}

void MediaObject::swapCurrentWithNext(qint32 transitionTime)
{
    // Save current state for event/signal handling below:
    bool prevHasVideo = m_videoPlayer->hasVideo();
    qint64 prevTotalTime = totalTime();

    if (!m_nextVideoPlayer->canPlayMedia()){
        gSetErrorString("Cannot play current media.");
        gSetErrorType(NORMAL_ERROR);
        gSetErrorLocation(ERROR_LOCATION);
        checkForError();
        return;
    }

    // Do the swap:
    QMutexLocker locker(&m_quickTimeGuard);
        m_videoPlayer->setVolume(0);
        m_nextVideoPlayer->setVolume(0);
        QuickTimeAudioPlayer *tmpAudioPlayer = m_audioPlayer;
        QuickTimeVideoPlayer *tmpVideoPlayer = m_videoPlayer;
        m_videoPlayer = m_nextVideoPlayer;
        m_audioPlayer = m_nextAudioPlayer;
        m_nextVideoPlayer = tmpVideoPlayer;
        m_nextAudioPlayer = tmpAudioPlayer;
        m_mediaObjectAudioNode->startCrossFade(transitionTime);
        m_audioGraph->updateStreamSpecifications();
        m_metaData->setVideo(m_videoPlayer->movieRef());
        m_audioPlayer->seek(0);
    locker.unlock();

    m_waitNextSwap = false;
    m_currentTime = 0;

    play();

    // Emit/notify information about the new source:
    QRect videoRect = m_videoPlayer->videoRect();
    MediaNodeEvent e1(MediaNodeEvent::VideoFrameSizeChanged, &videoRect);
    notify(&e1);

    emit currentSourceChanged(m_videoPlayer->mediaSource());
    emit metaDataChanged(m_metaData->metaData());

    if (prevHasVideo != m_videoPlayer->hasVideo())
        emit hasVideoChanged(m_videoPlayer->hasVideo());
    if (prevTotalTime != totalTime())
        emit totalTimeChanged(totalTime());
    checkForError();
}

void MediaObject::play()
{
    IMPLEMENTED;

    if (m_waitNextSwap){
        m_swapTime = QTime::currentTime();
        m_swapTime.addMSecs(m_swapTimeLeft);
        setState(Phonon::PlayingState);
        return;
    }
    if (m_currentTime == m_videoPlayer->duration())
        return;
    if (!m_videoPlayer->canPlayMedia())
        return;

    QMutexLocker locker(&m_quickTimeGuard);
        if (playAudioHint()){
            m_audioGraph->start();
            m_audioPlayer->play();
            if (m_nextAudioPlayer->currentTime() > 0)
                m_nextAudioPlayer->play();
        }
        if (playVideoHint()){
            m_videoPlayer->play();
            if (m_nextVideoPlayer->currentTime() > 0)
                m_nextVideoPlayer->play();
        }
    locker.unlock();

    setState(Phonon::PlayingState);
    checkForError();
}

void MediaObject::pause()
{
    IMPLEMENTED;
    QMutexLocker locker(&m_quickTimeGuard);
        if (playAudioHint()){
            m_audioPlayer->pause();
            m_nextAudioPlayer->pause();
        }
        if (playVideoHint()){
            m_videoPlayer->pause();
            m_nextVideoPlayer->pause();
        }
    locker.unlock();

    if (m_waitNextSwap)
        m_swapTimeLeft = m_swapTime.msecsTo(QTime::currentTime());

    setState(Phonon::PausedState);
    checkForError();
}

void MediaObject::stop()
{
    pause();
}

void MediaObject::seek(qint64 milliseconds)
{
    QMutexLocker locker(&m_quickTimeGuard);
        // Stop cross-fade if any:
        m_nextVideoPlayer->pause();
        m_nextAudioPlayer->pause();
        m_mediaObjectAudioNode->cancelCrossFade();

        // Pause (to let audio/video wait for each other) and seek:
        m_audioPlayer->pause(milliseconds);
        m_videoPlayer->pause();
        m_videoPlayer->seek(milliseconds);
    locker.unlock();

    if (m_currentTime < m_videoPlayer->duration())
        m_waitNextSwap = false;

    if (m_state == Phonon::PlayingState)
        play();
    checkForError();
}

QStringList MediaObject::availableAudioStreams() const
{
    NOT_IMPLEMENTED;
    return QStringList();
}

QStringList MediaObject::availableVideoStreams() const
{
    NOT_IMPLEMENTED;
    return QStringList();
}

QStringList MediaObject::availableSubtitleStreams() const
{
    NOT_IMPLEMENTED;
    return QStringList();
}

QString MediaObject::currentAudioStream(const QObject */*audioPath*/) const
{
    NOT_IMPLEMENTED;
    return QString();
}

QString MediaObject::currentVideoStream(const QObject */*videoPath*/) const
{
    NOT_IMPLEMENTED;
    return QString();
}

QString MediaObject::currentSubtitleStream(const QObject */*videoPath*/) const
{
    NOT_IMPLEMENTED;
    return QString();
}

void MediaObject::setCurrentAudioStream(const QString &/*streamName*/,const QObject */*audioPath*/)
{
    NOT_IMPLEMENTED;
}

void MediaObject::setCurrentVideoStream(const QString &/*streamName*/,const QObject */*videoPath*/)
{
    NOT_IMPLEMENTED;
}

void MediaObject::setCurrentSubtitleStream(const QString &/*streamName*/,const QObject */*videoPath*/)
{
    NOT_IMPLEMENTED;
}

void MediaObject::synchAudioVideo()
{
    if (m_state != Phonon::PlayingState)
        return;
    if (m_videoSinkList.isEmpty() && m_audioSinkList.isEmpty())
        return;

    qint64 t = m_currentTime;
    if (t > 0)
        seek(t);
    else if (m_state == Phonon::PlayingState)
        play();
    checkForError();
}

bool MediaObject::playVideoHint() const
{
    return (m_videoPlayer->hasVideo() && !m_videoSinkList.isEmpty()) ||
        (!m_audioSinkList.isEmpty() && m_audioGraph->graphCannotPlay());
}

bool MediaObject::playAudioHint() const
{
    return m_audioPlayer->hasAudio() && !m_audioSinkList.isEmpty();
}

qint32 MediaObject::tickInterval() const
{
    IMPLEMENTED;
    return m_tickInterval;
}

void MediaObject::setTickInterval(qint32 interval)
{
    IMPLEMENTED;
    m_tickInterval = interval;
    if (m_tickInterval > 0)
        m_tickTimer = startTimer(m_tickInterval);
    else{
        killTimer(m_tickTimer);
        m_tickTimer = 0;
    }
}

bool MediaObject::hasVideo() const
{
    IMPLEMENTED;
    return m_videoPlayer ? m_videoPlayer->hasVideo() : false;
}

bool MediaObject::isSeekable() const
{
    IMPLEMENTED;
    return true;
}

qint64 MediaObject::currentTime() const
{
    IMPLEMENTED_SILENT;
    return m_currentTime;
}

void MediaObject::updateCurrentTime()
{
    qint64 lastUpdateTime = m_currentTime;
    qint64 total = m_videoPlayer->duration();

    // Use the most appropriate way to decide current time:
    if (playVideoHint())
        m_currentTime = m_videoPlayer->currentTime();
    else if (playAudioHint()){
        qint64 t = m_audioPlayer->currentTime();
        m_currentTime = (t <= total) ? t : total;
    } else
        m_currentTime = 0;

    // Check if it's time to emit aboutToFinish:
    qint32 mark = qMin(total, total + m_transitionTime - 2000);
    if (lastUpdateTime < mark && mark <= m_currentTime)
        emit aboutToFinish();

    // Check if it's time to emit prefinishMarkReached:
    mark = total - m_prefinishMark;
    if (lastUpdateTime < mark && mark <= m_currentTime)
        emit prefinishMarkReached(total - m_currentTime);

    if (m_nextVideoPlayer->state() == QuickTimeVideoPlayer::NoMedia){
        // There is no next source in que.
        // Check if it's time to emit finished:
        if (lastUpdateTime < m_currentTime && m_currentTime == total){
            emit finished();
            pause();
        }
    } else {
        // We have a next source.
        // Check if it's time to swap to next source:
        mark = total + m_transitionTime;
        if (mark >= total){
            if (lastUpdateTime < total && total == m_currentTime){
                m_swapTime = QTime::currentTime();
                m_swapTime.addMSecs(mark - total);
                m_waitNextSwap = true;
            }
        } else if (m_videoPlayer->hasVideo() || m_nextVideoPlayer->hasVideo()){
            // Skip cross-fading for video, since there
            // tend to be problems with combinding stream formats:
            if (m_currentTime == total)
                swapCurrentWithNext(0);
        } else if (lastUpdateTime < mark && mark <= m_currentTime){
            swapCurrentWithNext(total - m_currentTime);
        }
    }
}

qint64 MediaObject::totalTime() const
{
    IMPLEMENTED_SILENT;
    return m_videoPlayer->duration();
}

Phonon::State MediaObject::state() const
{
    IMPLEMENTED;
    return m_state;
}

QString MediaObject::errorString() const
{
    IMPLEMENTED;
    return gGetErrorString();
}

Phonon::ErrorType MediaObject::errorType() const
{
    IMPLEMENTED;
    int type = gGetErrorType();
    switch (type){
    case NORMAL_ERROR:
        return Phonon::NormalError;
    case FATAL_ERROR:
        return Phonon::FatalError;
    case NO_ERROR:
    default:
        return Phonon::NoError;
    }
}

void MediaObject::checkForError()
{
    int type = gGetErrorType();
    if (type == NO_ERROR)
        return;

    // The user must listen to the state change
    // signal, and look up errorType and ErrorString
    // immidiatly for it to work.
    setState(Phonon::ErrorState);
    gClearError();
}

QuickTimeVideoPlayer* MediaObject::videoPlayer() const
{
    return m_videoPlayer;
}

MediaSource MediaObject::source() const
{
    IMPLEMENTED;
    return m_videoPlayer->mediaSource();
}

qint32 MediaObject::prefinishMark() const
{
    IMPLEMENTED;
    return m_prefinishMark;
}

void MediaObject::setPrefinishMark(qint32 mark)
{
    IMPLEMENTED;
    m_prefinishMark = mark;
}

qint32 MediaObject::transitionTime() const
{
    IMPLEMENTED;
    return m_transitionTime;
}

void MediaObject::setTransitionTime(qint32 transitionTime)
{
    IMPLEMENTED;
    m_transitionTime = transitionTime;
}

void MediaObject::setVolumeOnMovie(float volume)
{
    m_videoPlayer->setVolume(volume);
}

bool MediaObject::setAudioDeviceOnMovie(int id)
{
    return m_videoPlayer->setAudioDevice(id);
}

void MediaObject::doRegualarTasksThreadSafe()
{
    QMutexLocker locker(&m_quickTimeGuard);
        m_audioPlayer->doRegularTasks();
        m_videoPlayer->doRegularTasks();
        
        m_nextVideoPlayer->doRegularTasks();
        m_nextAudioPlayer->doRegularTasks();
        m_mediaObjectAudioNode->updateCrossFade(m_currentTime);

        if (m_mediaObjectAudioNode->m_fadeDuration == 0){
            if (m_nextVideoPlayer->isPlaying())
                m_nextVideoPlayer->unsetVideo();
            if (m_nextAudioPlayer->isPlaying())
                m_nextAudioPlayer->unsetVideo();
        }        
    locker.unlock();

    if (m_waitNextSwap &&
        m_state == Phonon::PlayingState &&
        m_transitionTime < m_swapTime.msecsTo(QTime::currentTime()))
        swapCurrentWithNext(0);
}

/**
    Callback from the display link (in a thread != main thread)
*/
void MediaObject::displayLinkCallback(const CVTimeStamp &timeStamp)
{
    QMutexLocker locker(&m_quickTimeGuard);
        // Update video:
        if (m_videoPlayer->videoFrameChanged(timeStamp)){
            VideoFrame frame(m_videoPlayer, timeStamp);
            
            if (m_nextVideoPlayer->isPlaying()){
                if (m_mediaObjectAudioNode->m_fadeDuration > 0){
                    // We're cross-fading. Set the previous' movie frames
                    // as background images, and borrow audio values for opacity:
                    VideoFrame bgFrame(m_nextVideoPlayer, timeStamp);
                    bgFrame.m_opacity = m_mediaObjectAudioNode->m_volume2;
                    frame.setBackgroundFrame(bgFrame);
                    frame.m_opacity = m_mediaObjectAudioNode->m_volume1;
                }
            }
            
            // Send the frame through the graph:
            updateVideo(frame);

            // Make sure that audio and video are in lip-synch:
            qint64 diff = m_audioPlayer->currentTime() - m_videoPlayer->currentTime();
            if (-50 > diff || diff > 50)
                m_audioPlayer->seek(m_videoPlayer->currentTime() + 20);
        }
    locker.unlock();

    // Take advantage of the callback for
    // updating the media object time:
    QCoreApplication::postEvent(this, new QEvent(QEvent::User), Qt::HighEventPriority);
}

void MediaObject::mediaNodeEvent(const MediaNodeEvent *event)
{
    switch (event->type()){
        case MediaNodeEvent::VideoSinkAdded:
        case MediaNodeEvent::AudioSinkAdded:
        case MediaNodeEvent::VideoSinkRemoved:
        case MediaNodeEvent::AudioSinkRemoved:
            synchAudioVideo();
            break;
        default:
            break;
    }
}

bool MediaObject::event(QEvent *event)
{
    switch (event->type()){
        case QEvent::User:
            updateCurrentTime();
            doRegualarTasksThreadSafe();
            break;
        case QEvent::Timer:
            QTimerEvent *timerEvent = static_cast<QTimerEvent *>(event);
            if (timerEvent->timerId() == m_tickTimer)
                emit tick(currentTime());
            break;
        default:
            break;
    }
    return QObject::event(event);
}

bool MediaObject::hasInterface(Interface /*interface*/) const
{
    return false;
}

QVariant MediaObject::interfaceCall(Interface /*interface*/, int /*command*/, const QList<QVariant> &/*arguments*/)
{
    return QVariant();
}

}} // namespace Phonon::QT7

#include "moc_mediaobject.cpp"

