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

#include "xinestream.h"
#include "xineengine.h"
#include <QMutexLocker>
#include <QEvent>
#include <QCoreApplication>
#include <QTimer>
#include <kurl.h>
#include "audioport.h"
#include "videowidget.h"
#include "mediaobject.h"
#include "xinethread.h"
#include <klocale.h>
#include "events.h"
#include "bytestream.h"

extern "C" {
#define this _this_xine_
#include <xine/xine_internal.h>
#undef this
}

//#define DISABLE_FILE_MRLS

//#define streamClock(stream) stream->clock
#define streamClock(stream) stream->xine->clock

namespace Phonon
{
namespace Xine
{

// called from main thread
XineStream::XineStream()
    : QObject(0), // XineStream is ref-counted
    m_stream(0),
    m_event_queue(0),
    m_deinterlacer(0),
    m_state(Phonon::LoadingState),
    m_byteStream(0),
    m_prefinishMarkTimer(0),
    m_errorType(Phonon::NoError),
    m_lastSeekCommand(0),
    m_volume(100),
//    m_startTime(-1),
    m_totalTime(-1),
    m_currentTime(-1),
    m_availableTitles(-1),
    m_availableChapters(-1),
    m_availableAngles(-1),
    m_currentAngle(-1),
    m_currentTitle(-1),
    m_currentChapter(-1),
    m_transitionGap(0),
    m_streamInfoReady(false),
    m_hasVideo(false),
    m_isSeekable(false),
    m_useGaplessPlayback(false),
    m_prefinishMarkReachedNotEmitted(true),
    m_ticking(false),
    m_closing(false),
    m_tickTimer(this)
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    connect(&m_tickTimer, SIGNAL(timeout()), SLOT(emitTick()), Qt::DirectConnection);
}

XineStream::~XineStream()
{
    if (m_deinterlacer) {
        xine_post_dispose(XineEngine::xine(), m_deinterlacer);
    }
    if(m_event_queue) {
        xine_event_dispose_queue(m_event_queue);
        m_event_queue = 0;
    }
    if(m_stream) {
        xine_dispose(m_stream);
        m_stream = 0;
    }
    delete m_prefinishMarkTimer;
    m_prefinishMarkTimer = 0;
}

// xine thread
bool XineStream::xineOpen(Phonon::State newstate)
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    Q_ASSERT(m_stream);
    if (m_mrl.isEmpty() || m_closing) {
        return false;
    }
    // only call xine_open if it's not already open
    Q_ASSERT(xine_get_status(m_stream) == XINE_STATUS_IDLE);

#ifdef DISABLE_FILE_MRLS
    if (m_mrl.startsWith("file:/")) {
        kDebug(610) << "faked xine_open failed for m_mrl =" << m_mrl.constData();
        error(Phonon::NormalError, i18n("Cannot open media data at '<i>%1</i>'", m_mrl.constData()));
        return false;
    }
#endif

    // xine_open can call functions from ByteStream which will block waiting for data.
    //kDebug(610) << "xine_open(" << m_mrl.constData() << ")";
    if (xine_open(m_stream, m_mrl.constData()) == 0) {
        kDebug(610) << "xine_open failed for m_mrl =" << m_mrl.constData();
        switch (xine_get_error(m_stream)) {
        case XINE_ERROR_NONE:
            // hmm?
            abort();
        case XINE_ERROR_NO_INPUT_PLUGIN:
            error(Phonon::NormalError, i18n("cannot find input plugin for MRL [%1]", m_mrl.constData()));
            break;
        case XINE_ERROR_NO_DEMUX_PLUGIN:
            if (m_mrl.startsWith("kbytestream:/")) {
                error(Phonon::FatalError, i18n("cannot find demultiplexer plugin for the given media data"));
            } else {
                error(Phonon::FatalError, i18n("cannot find demultiplexer plugin for MRL [%1]", m_mrl.constData()));
            }
            break;
        default:
            {
                const char *const *logs = xine_get_log(XineEngine::xine(), XINE_LOG_MSG);
                error(Phonon::NormalError, QString::fromUtf8(logs[0]));
            }
            break;
//X         default:
//X             error(Phonon::NormalError, i18n("Cannot open media data at '<i>%1</i>'", m_mrl.constData()));
//X             break;
        }
        return false;
    }
    kDebug(610) << "xine_open succeeded for m_mrl =" << m_mrl.constData();
    if ((m_mrl.startsWith("dvd:/") && XineEngine::deinterlaceDVD()) ||
        (m_mrl.startsWith("vcd:/") && XineEngine::deinterlaceVCD()) ||
        (m_mrl.startsWith("file:/") && XineEngine::deinterlaceFile())) {
        // for DVDs we add an interlacer by default
        xine_video_port_t *videoPort = 0;
        Q_ASSERT(m_mediaObject);
        QSet<SinkNode *> sinks = m_mediaObject->sinks();
        foreach (SinkNode *sink, sinks) {
            Q_ASSERT(sink->threadSafeObject());
            if (sink->threadSafeObject()->videoPort()) {
                Q_ASSERT(videoPort == 0);
                videoPort = sink->threadSafeObject()->videoPort();
            }
        }
        if (!videoPort) {
            kDebug(610) << "creating xine_stream with null video port";
            videoPort = XineEngine::nullVideoPort();
        }
        m_deinterlacer = xine_post_init(XineEngine::xine(), "tvtime", 1, 0, &videoPort);
        Q_ASSERT(m_deinterlacer);

        // set method
        xine_post_in_t *paraInput = xine_post_input(m_deinterlacer, "parameters");
        Q_ASSERT(paraInput);
        Q_ASSERT(paraInput->data);
        xine_post_api_t *api = reinterpret_cast<xine_post_api_t *>(paraInput->data);
        xine_post_api_descr_t *desc = api->get_param_descr();
        char *pluginParams = static_cast<char *>(malloc(desc->struct_size));
        api->get_parameters(m_deinterlacer, pluginParams);
        for (int i = 0; desc->parameter[i].type != POST_PARAM_TYPE_LAST; ++i) {
            xine_post_api_parameter_t &p = desc->parameter[i];
            if (p.type == POST_PARAM_TYPE_INT && 0 == strcmp(p.name, "method")) {
                int *value = reinterpret_cast<int *>(pluginParams + p.offset);
                *value = XineEngine::deinterlaceMethod();
                break;
            }
        }
        api->set_parameters(m_deinterlacer, pluginParams);
        free(pluginParams);

        // connect to xine_stream_t
        xine_post_in_t *x = xine_post_input(m_deinterlacer, "video");
        Q_ASSERT(x);
        xine_post_out_t *videoOutputPort = xine_get_video_source(m_stream);
        Q_ASSERT(videoOutputPort);
        xine_post_wire(videoOutputPort, x);
    } else if (m_deinterlacer) {
        xine_post_dispose(XineEngine::xine(), m_deinterlacer);
        m_deinterlacer = 0;
    }

    m_lastTimeUpdate.tv_sec = 0;
    xine_get_pos_length(m_stream, 0, &m_currentTime, &m_totalTime);
    getStreamInfo();
    emit length(m_totalTime);
    updateMetaData();
    // if there's a PlayCommand in the event queue the state should not go to StoppedState
    changeState(newstate);
    return true;
}

// called from main thread
int XineStream::totalTime() const
{
    if (!m_stream || m_mrl.isEmpty()) {
        return -1;
    }
    return m_totalTime;
}

// called from main thread
int XineStream::remainingTime() const
{
    if (!m_stream || m_mrl.isEmpty()) {
        return 0;
    }
    QMutexLocker locker(&m_updateTimeMutex);
    if (m_state == Phonon::PlayingState && m_lastTimeUpdate.tv_sec > 0) {
        struct timeval now;
        gettimeofday(&now, 0);
        const int diff = (now.tv_sec - m_lastTimeUpdate.tv_sec) * 1000 + (now.tv_usec - m_lastTimeUpdate.tv_usec) / 1000;
        return m_totalTime - (m_currentTime + diff);
    }
    return m_totalTime - m_currentTime;
}

// called from main thread
int XineStream::currentTime() const
{
    if (!m_stream || m_mrl.isEmpty()) {
        return -1;
    }
    QMutexLocker locker(&m_updateTimeMutex);
    if (m_state == Phonon::PlayingState && m_lastTimeUpdate.tv_sec > 0) {
        struct timeval now;
        gettimeofday(&now, 0);
        const int diff = (now.tv_sec - m_lastTimeUpdate.tv_sec) * 1000 + (now.tv_usec - m_lastTimeUpdate.tv_usec) / 1000;
        return m_currentTime + diff;
    }
    return m_currentTime;
}

// called from main thread
bool XineStream::hasVideo() const
{
    if (!m_streamInfoReady) {
        QMutexLocker locker(&m_streamInfoMutex);
        QCoreApplication::postEvent(const_cast<XineStream *>(this), new QEVENT(GetStreamInfo));
        // wait a few ms, perhaps the other thread finishes the event in time and this method
        // can return a useful value
        // FIXME: this is non-deterministic: a program might fail sometimes and sometimes work
        // because of this
        if (!m_waitingForStreamInfo.wait(&m_streamInfoMutex, 80)) {
            kDebug(610) << "waitcondition timed out";
        }
    }
    return m_hasVideo;
}

// called from main thread
bool XineStream::isSeekable() const
{
    if (!m_streamInfoReady) {
        //QMutexLocker locker(&m_streamInfoMutex);
        QCoreApplication::postEvent(const_cast<XineStream *>(this), new QEVENT(GetStreamInfo));
        // wait a few ms, perhaps the other thread finishes the event in time and this method
        // can return a useful value
        // FIXME: this is non-deterministic: a program might fail sometimes and sometimes work
        // because of this
        /*if (!m_waitingForStreamInfo.wait(&m_streamInfoMutex, 80)) {
            kDebug(610) << "waitcondition timed out";
            return false;
        } */
    }
    return m_isSeekable;
}

// xine thread
void XineStream::getStreamInfo()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());

    if (m_stream && !m_mrl.isEmpty()) {
        if (xine_get_status(m_stream) == XINE_STATUS_IDLE) {
            kDebug(610) << "calling xineOpen from ";
            if (!xineOpen(Phonon::StoppedState)) {
                return;
            }
        }
        QMutexLocker locker(&m_streamInfoMutex);
        bool hasVideo   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_HAS_VIDEO);
        bool isSeekable = xine_get_stream_info(m_stream, XINE_STREAM_INFO_SEEKABLE);
        int availableTitles   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_TITLE_COUNT);
        int availableChapters = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_CHAPTER_COUNT);
        int availableAngles   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_ANGLE_COUNT);
        m_streamInfoReady = true;
        if (m_hasVideo != hasVideo) {
            m_hasVideo = hasVideo;
            emit hasVideoChanged(m_hasVideo);
        }
        if (m_isSeekable != isSeekable) {
            m_isSeekable = isSeekable;
            emit seekableChanged(m_isSeekable);
        }
        if (m_availableTitles != availableTitles) {
            kDebug(610) << "available titles changed: " << availableTitles;
            m_availableTitles = availableTitles;
            emit availableTitlesChanged(m_availableTitles);
        }
        if (m_availableChapters != availableChapters) {
            kDebug(610) << "available chapters changed: " << availableChapters;
            m_availableChapters = availableChapters;
            emit availableChaptersChanged(m_availableChapters);
        }
        if (m_availableAngles != availableAngles) {
            kDebug(610) << "available angles changed: " << availableAngles;
            m_availableAngles = availableAngles;
            emit availableAnglesChanged(m_availableAngles);
        }
        if (m_hasVideo) {
            uint32_t width = xine_get_stream_info(m_stream, XINE_STREAM_INFO_VIDEO_WIDTH);
            uint32_t height = xine_get_stream_info(m_stream, XINE_STREAM_INFO_VIDEO_HEIGHT);
            handleDownstreamEvent(new FrameFormatChangeEvent(width, height, 0, 0));
        }
    }
    m_waitingForStreamInfo.wakeAll();
}

// xine thread
bool XineStream::createStream()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());

    if (m_stream || m_state == Phonon::ErrorState) {
        return false;
    }

    m_portMutex.lock();
    //kDebug(610) << "AudioPort.xinePort() = " << m_audioPort.xinePort();
    xine_audio_port_t *audioPort = 0;
    xine_video_port_t *videoPort = 0;
    Q_ASSERT(m_mediaObject);
    QSet<SinkNode *> sinks = m_mediaObject->sinks();
    kDebug(610) << "MediaObject is connected to " << sinks.size() << " nodes";
    foreach (SinkNode *sink, sinks) {
        Q_ASSERT(sink->threadSafeObject());
        if (sink->threadSafeObject()->audioPort()) {
            Q_ASSERT(audioPort == 0);
            audioPort = sink->threadSafeObject()->audioPort();
        }
        if (sink->threadSafeObject()->videoPort()) {
            Q_ASSERT(videoPort == 0);
            videoPort = sink->threadSafeObject()->videoPort();
        }
    }
    if (!audioPort) {
        kDebug(610) << "creating xine_stream with null audio port";
        audioPort = XineEngine::nullPort();
    }
    if (!videoPort) {
        kDebug(610) << "creating xine_stream with null video port";
        videoPort = XineEngine::nullVideoPort();
    }
    m_stream = xine_stream_new(XineEngine::xine(), audioPort, videoPort);
    /*
    if (m_audioPostLists.size() == 1) {
        m_audioPostLists.first().wireStream();
    } else if (m_audioPostLists.size() > 1) {
        kWarning(610) << "multiple AudioPaths per MediaObject is not supported. Trying anyway.";
        foreach (AudioPostList apl, m_audioPostLists) {
            apl.wireStream();
        }
    }
    */
    if (m_volume != 100) {
        xine_set_param(m_stream, XINE_PARAM_AUDIO_AMP_LEVEL, m_volume);
    }
//X     if (!m_audioPort.isValid()) {
//X         xine_set_param(m_stream, XINE_PARAM_IGNORE_AUDIO, 1);
//X     }
//X     if (!m_videoPort) {
//X         xine_set_param(m_stream, XINE_PARAM_IGNORE_VIDEO, 1);
//X     }
    m_portMutex.unlock();
    m_waitingForRewire.wakeAll();

    Q_ASSERT(!m_event_queue);
    m_event_queue = xine_event_new_queue(m_stream);
    xine_event_create_listener_thread(m_event_queue, &XineEngine::self()->xineEventListener, (void *)this);

    if (m_useGaplessPlayback) {
        kDebug(610) << "XINE_PARAM_EARLY_FINISHED_EVENT: 1";
        xine_set_param(m_stream, XINE_PARAM_EARLY_FINISHED_EVENT, 1);
#ifdef XINE_PARAM_DELAY_FINISHED_EVENT
    } else if (m_transitionGap > 0) {
        kDebug(610) << "XINE_PARAM_DELAY_FINISHED_EVENT:" << m_transitionGap;
        xine_set_param(m_stream, XINE_PARAM_DELAY_FINISHED_EVENT, m_transitionGap);
#endif // XINE_PARAM_DELAY_FINISHED_EVENT
    } else {
        kDebug(610) << "XINE_PARAM_EARLY_FINISHED_EVENT: 0";
        xine_set_param(m_stream, XINE_PARAM_EARLY_FINISHED_EVENT, 0);
    }

    return true;
}

/*
//called from main thread
void XineStream::addAudioPostList(const AudioPostList &postList)
{
    QCoreApplication::postEvent(this, new ChangeAudioPostListEvent(postList, ChangeAudioPostListEvent::Add));
}

//called from main thread
void XineStream::removeAudioPostList(const AudioPostList &postList)
{
    QCoreApplication::postEvent(this, new ChangeAudioPostListEvent(postList, ChangeAudioPostListEvent::Remove));
}
*/

//called from main thread
void XineStream::aboutToDeleteVideoWidget()
{
    kDebug(610);
    m_portMutex.lock();

    // schedule m_stream rewiring
    QCoreApplication::postEvent(this, new QEVENT(RewireVideoToNull));
    kDebug(610) << "waiting for rewire";
    m_waitingForRewire.wait(&m_portMutex);
    m_portMutex.unlock();
}

// called from main thread
void XineStream::setTickInterval(qint32 interval)
{
    QCoreApplication::postEvent(this, new SetTickIntervalEvent(interval));
}

// called from main thread
void XineStream::setPrefinishMark(qint32 time)
{
    QCoreApplication::postEvent(this, new SetPrefinishMarkEvent(time));
}

// called from main thread
void XineStream::useGaplessPlayback(bool b)
{
    if (m_useGaplessPlayback == b) {
        return;
    }
    m_useGaplessPlayback = b;
    QCoreApplication::postEvent(this, new QEVENT(TransitionTypeChanged));
}

// called from main thread
void XineStream::useGapOf(int gap)
{
    m_useGaplessPlayback = false;
    m_transitionGap = gap;
    QCoreApplication::postEvent(this, new QEVENT(TransitionTypeChanged));
}

// called from main thread
void XineStream::gaplessSwitchTo(const KUrl &url)
{
    gaplessSwitchTo(url.url().toUtf8());
}

// called from main thread
void XineStream::gaplessSwitchTo(const QByteArray &mrl)
{
    QCoreApplication::postEvent(this, new GaplessSwitchEvent(mrl));
}

// xine thread
void XineStream::changeState(Phonon::State newstate)
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    if (m_state == newstate) {
        return;
    }
    Phonon::State oldstate = m_state;
    m_state = newstate;
    if (newstate == Phonon::PlayingState) {
        if (m_ticking) {
            m_tickTimer.start();
            //kDebug(610) << "tickTimer started.";
        }
        if (m_prefinishMark > 0) {
            emitAboutToFinish();
        }
    } else if (oldstate == Phonon::PlayingState) {
        m_tickTimer.stop();
        //kDebug(610) << "tickTimer stopped.";
        m_prefinishMarkReachedNotEmitted = true;
        if (m_prefinishMarkTimer) {
            m_prefinishMarkTimer->stop();
        }
    }
    if (newstate == Phonon::ErrorState) {
        kDebug(610) << "reached error state";// from: " << kBacktrace();
        if (m_event_queue) {
            xine_event_dispose_queue(m_event_queue);
            m_event_queue = 0;
        }
        if (m_stream) {
            xine_dispose(m_stream);
            m_stream = 0;
        }
    }
    emit stateChanged(newstate, oldstate);
}

// xine thread
void XineStream::updateMetaData()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    QMultiMap<QString, QString> metaDataMap;
    metaDataMap.insert(QLatin1String("TITLE"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_TITLE)));
    metaDataMap.insert(QLatin1String("ARTIST"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_ARTIST)));
    metaDataMap.insert(QLatin1String("GENRE"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_GENRE)));
    metaDataMap.insert(QLatin1String("ALBUM"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_ALBUM)));
    metaDataMap.insert(QLatin1String("DATE"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_YEAR)));
    metaDataMap.insert(QLatin1String("TRACKNUMBER"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_TRACK_NUMBER)));
    metaDataMap.insert(QLatin1String("DESCRIPTION"),
            QString::fromUtf8(xine_get_meta_info(m_stream, XINE_META_INFO_COMMENT)));
    if(metaDataMap == m_metaDataMap)
        return;
    m_metaDataMap = metaDataMap;
    //kDebug(610) << "emitting metaDataChanged(" << m_metaDataMap << ")";
    emit metaDataChanged(m_metaDataMap);
}

// xine thread
void XineStream::playbackFinished()
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_prefinishMarkReachedNotEmitted && m_prefinishMark > 0) {
            emit prefinishMarkReached(0);
        }
        changeState(Phonon::StoppedState);
        xine_close(m_stream); // TODO: is it necessary? should xine_close be called as late as possible?
        m_streamInfoReady = false;
        m_prefinishMarkReachedNotEmitted = true;
        emit finished();
    }
    m_waitingForClose.wakeAll();
}

// xine thread
inline void XineStream::error(Phonon::ErrorType type, const QString &string)
{
    m_errorType = type;
    m_errorString = string;
    changeState(Phonon::ErrorState);
}

const char *nameForEvent(int e)
{
    switch (e) {
    case Event::Reference:
        return "Reference";
    case Event::UiChannelsChanged:
        return "UiChannelsChanged";
    case Event::MediaFinished:
        return "MediaFinished";
    case Event::UpdateTime:
        return "UpdateTime";
    case Event::GaplessSwitch:
        return "GaplessSwitch";
    case Event::NewMetaData:
        return "NewMetaData";
    case Event::Progress:
        return "Progress";
    case Event::GetStreamInfo:
        return "GetStreamInfo";
    case Event::UpdateVolume:
        return "UpdateVolume";
    case Event::MrlChanged:
        return "MrlChanged";
    case Event::TransitionTypeChanged:
        return "TransitionTypeChanged";
    case Event::RewireVideoToNull:
        return "RewireVideoToNull";
    case Event::PlayCommand:
        return "PlayCommand";
    case Event::PauseCommand:
        return "PauseCommand";
    case Event::StopCommand:
        return "StopCommand";
    case Event::SetTickInterval:
        return "SetTickInterval";
    case Event::SetPrefinishMark:
        return "SetPrefinishMark";
    case Event::SeekCommand:
        return "SeekCommand";
    //case Event::EventSend:
        //return "EventSend";
    case Event::SetParam:
        return "SetParam";
        /*
    case Event::ChangeAudioPostList:
        return "ChangeAudioPostList";
        */
//X case Event::AudioRewire:
//X     return "AudioRewire";
    default:
        return 0;
    }
}

// xine thread
bool XineStream::event(QEvent *ev)
{
    if (ev->type() != QEvent::ThreadChange) {
        Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    }
    const char *eventName = nameForEvent(ev->type());
    if (m_closing) {
        // when closing all events except MrlChanged are ignored. MrlChanged is used to detach from
        // a kbytestream:/ MRL
        switch (ev->type()) {
        case Event::MrlChanged:
        //case Event::ChangeAudioPostList:
            break;
        default:
            if (eventName) {
                kDebug(610) << "####################### ignoring Event: " << eventName;
            }
            return QObject::event(ev);
        }
    }
    if (eventName) {
        if (static_cast<int>(ev->type()) == Event::Progress) {
            ProgressEvent *e = static_cast<ProgressEvent *>(ev);
            kDebug(610) << "################################ Event: " << eventName << ": " << e->percent;
        } else {
            kDebug(610) << "################################ Event: " << eventName;
        }
    }
    switch (ev->type()) {
    case Event::Reference:
        ev->accept();
        {
            ReferenceEvent *e = static_cast<ReferenceEvent *>(ev);
            setMrlInternal(e->mrl);
            if (xine_get_status(m_stream) != XINE_STATUS_IDLE) {
                m_mutex.lock();
                xine_close(m_stream);
                m_streamInfoReady = false;
                m_prefinishMarkReachedNotEmitted = true;
                m_mutex.unlock();
            }
            if (xineOpen(Phonon::BufferingState)) {
                internalPlay();
            }
        }
        return true;
    case Event::UiChannelsChanged:
        ev->accept();
        // check chapter, title, angle and substreams
        if (m_stream) {
            int availableTitles   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_TITLE_COUNT);
            int availableChapters = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_CHAPTER_COUNT);
            int availableAngles   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_ANGLE_COUNT);
            if (m_availableTitles != availableTitles) {
                kDebug(610) << "available titles changed: " << availableTitles;
                m_availableTitles = availableTitles;
                emit availableTitlesChanged(m_availableTitles);
            }
            if (m_availableChapters != availableChapters) {
                kDebug(610) << "available chapters changed: " << availableChapters;
                m_availableChapters = availableChapters;
                emit availableChaptersChanged(m_availableChapters);
            }
            if (m_availableAngles != availableAngles) {
                kDebug(610) << "available angles changed: " << availableAngles;
                m_availableAngles = availableAngles;
                emit availableAnglesChanged(m_availableAngles);
            }

            int currentTitle   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_TITLE_NUMBER);
            int currentChapter = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_CHAPTER_NUMBER);
            int currentAngle   = xine_get_stream_info(m_stream, XINE_STREAM_INFO_DVD_ANGLE_NUMBER);
            if (currentAngle != m_currentAngle) {
                kDebug(610) << "current angle changed: " << currentAngle;
                m_currentAngle = currentAngle;
                emit angleChanged(m_currentAngle);
            }
            if (currentChapter != m_currentChapter) {
                kDebug(610) << "current chapter changed: " << currentChapter;
                m_currentChapter = currentChapter;
                emit chapterChanged(m_currentChapter);
            }
            if (currentTitle != m_currentTitle) {
                kDebug(610) << "current title changed: " << currentTitle;
                m_currentTitle = currentTitle;
                emit titleChanged(m_currentTitle);
            }
        }
        return true;
    case Event::Error:
        ev->accept();
        {
            ErrorEvent *e = static_cast<ErrorEvent *>(ev);
            error(e->type, e->reason);
        }
        return true;
    case Event::PauseForBuffering:
        ev->accept();
        if (m_stream) {
            xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE); //_x_set_speed (m_stream, XINE_SPEED_PAUSE);
            streamClock(m_stream)->set_option (streamClock(m_stream), CLOCK_SCR_ADJUSTABLE, 0);
        }
        return true;
    case Event::UnpauseForBuffering:
        ev->accept();
        if (m_stream) {
           if (Phonon::PausedState != m_state) {
               xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL); //_x_set_speed (m_stream, XINE_SPEED_NORMAL);
           }
           streamClock(m_stream)->set_option (streamClock(m_stream), CLOCK_SCR_ADJUSTABLE, 1);
        }
        return true;
        /*
    case Event::ChangeAudioPostList:
            ev->accept();
            {
                ChangeAudioPostListEvent *e = static_cast<ChangeAudioPostListEvent *>(ev);
                if (e->what == ChangeAudioPostListEvent::Add) {
                    Q_ASSERT(!m_audioPostLists.contains(e->postList));
                    m_audioPostLists << e->postList;
                    if (m_stream) {
                        if (m_audioPostLists.size() > 1) {
                            kWarning(610) << "attaching multiple AudioPaths to one MediaObject is not supported yet.";
                        }
                        e->postList.wireStream();
                    }
                    e->postList.setXineStream(this);
                } else { // Remove
                    e->postList.unsetXineStream(this);
                    const int r = m_audioPostLists.removeAll(e->postList);
                    Q_ASSERT(1 == r);
                    if (m_stream) {
                        if (m_audioPostLists.size() > 0) {
                            m_audioPostLists.last().wireStream();
                        } else {
                            xine_post_wire_audio_port(xine_get_audio_source(m_stream), XineEngine::nullPort());
                        }
                    }
                }
            }
            return true;
            */
//X     case Event::AudioRewire:
//X         ev->accept();
//X         if (m_stream) {
//X             AudioRewireEvent *e = static_cast<AudioRewireEvent *>(ev);
//X             e->postList->wireStream();
//X             xine_set_param(m_stream, XINE_PARAM_AUDIO_AMP_LEVEL, m_volume);
//X         }
//X             return true;
    case Event::EventSend:
        ev->accept();
        {
            EventSendEvent *e = static_cast<EventSendEvent *>(ev);
            if (m_stream) {
                xine_event_send(m_stream, e->event);
            }
            switch (e->event->type) {
            case XINE_EVENT_INPUT_MOUSE_MOVE:
            case XINE_EVENT_INPUT_MOUSE_BUTTON:
                delete static_cast<xine_input_data_t *>(e->event->data);
                break;
            }
            delete e->event;
        }
        return true;
    case Event::SetParam:
        ev->accept();
        if (m_stream) {
            SetParamEvent *e = static_cast<SetParamEvent *>(ev);
            xine_set_param(m_stream, e->param, e->value);
        }
        return true;
    case Event::MediaFinished:
        ev->accept();
        kDebug(610) << "MediaFinishedEvent m_useGaplessPlayback = " << m_useGaplessPlayback;
        if (m_stream) {
            if (m_useGaplessPlayback) {
                xine_set_param(m_stream, XINE_PARAM_GAPLESS_SWITCH, 1);
            }
            emit needNextUrl();
        }
        return true;
    case Event::UpdateTime:
        updateTime();
        ev->accept();
        return true;
    case Event::GaplessSwitch:
        ev->accept();
        {
            GaplessSwitchEvent *e = static_cast<GaplessSwitchEvent *>(ev);
            m_mutex.lock();
            if (e->mrl.isEmpty()) {
                kDebug(610) << "no GaplessSwitch";
            } else {
                setMrlInternal(e->mrl);
                kDebug(610) << "GaplessSwitch new m_mrl =" << m_mrl.constData();
            }
            if (e->mrl.isEmpty() || m_closing) {
                xine_set_param(m_stream, XINE_PARAM_GAPLESS_SWITCH, 0);
                m_mutex.unlock();
                playbackFinished();
                return true;
            }
            if (!xine_open(m_stream, m_mrl.constData())) {
                kWarning(610) << "xine_open for gapless playback failed!";
                xine_set_param(m_stream, XINE_PARAM_GAPLESS_SWITCH, 0);
                m_mutex.unlock();
                playbackFinished();
                return true; // FIXME: correct?
            }
            m_mutex.unlock();
            xine_play(m_stream, 0, 0);

            if (m_prefinishMarkReachedNotEmitted && m_prefinishMark > 0) {
                emit prefinishMarkReached(0);
            }
            m_prefinishMarkReachedNotEmitted = true;
            getStreamInfo();
            updateTime();
            updateMetaData();
        }
        return true;
    case Event::NewMetaData:
        ev->accept();
        if (m_stream) {
            getStreamInfo();
            updateMetaData();
        }
        return true;
    case Event::Progress:
        {
            ProgressEvent *e = static_cast<ProgressEvent *>(ev);
            if (e->percent < 100) {
                if (m_state == Phonon::PlayingState) {
                    changeState(Phonon::BufferingState);
                }
            } else {
                if (m_state == Phonon::BufferingState) {
                    changeState(Phonon::PlayingState);
                }
                //QTimer::singleShot(20, this, SLOT(getStartTime()));
            }
            kDebug(610) << "emit bufferStatus(" << e->percent << ")";
            emit bufferStatus(e->percent);
        }
        ev->accept();
        return true;
    case Event::GetStreamInfo:
        ev->accept();
        if (m_stream) {
            getStreamInfo();
        }
        return true;
    case Event::UpdateVolume:
        ev->accept();
        {
            UpdateVolumeEvent *e = static_cast<UpdateVolumeEvent *>(ev);
            m_volume = e->volume;
            if (m_stream) {
                xine_set_param(m_stream, XINE_PARAM_AUDIO_AMP_LEVEL, m_volume);
            }
        }
        return true;
    case Event::MrlChanged:
        ev->accept();
        {
            MrlChangedEvent *e = static_cast<MrlChangedEvent *>(ev);
            /* Always handle a MRL change request. We assume the application knows what it's
             * doing. If we return here then the stream is not reinitialized and the state
             * changes are different.
            if (m_mrl == e->mrl) {
                return true;
            } */
            State previousState = m_state;
            setMrlInternal(e->mrl);
            m_errorType = Phonon::NoError;
            m_errorString = QString();
            if (!m_stream) {
                changeState(Phonon::LoadingState);
                m_mutex.lock();
                createStream();
                m_mutex.unlock();
                if (!m_stream) {
                    kError(610) << "MrlChangedEvent: createStream didn't create a stream. This should not happen.";
                    error(Phonon::FatalError, i18n("Xine failed to create a stream."));
                    return true;
                }
            } else if (xine_get_status(m_stream) != XINE_STATUS_IDLE) {
                m_mutex.lock();
                xine_close(m_stream);
                m_streamInfoReady = false;
                m_prefinishMarkReachedNotEmitted = true;
                changeState(Phonon::LoadingState);
                m_mutex.unlock();
            }
            if (m_closing || m_mrl.isEmpty()) {
                kDebug(610) << "MrlChanged: don't call xineOpen. m_closing =" << m_closing << ", m_mrl =" << m_mrl.constData();
                m_waitingForClose.wakeAll();
            } else {
                kDebug(610) << "calling xineOpen from MrlChanged";
                if (!xineOpen(Phonon::StoppedState)) {
                    return true;
                }
                switch (e->stateForNewMrl) {
                case StoppedState:
                    break;
                case PlayingState:
                    if (m_stream) {
                        internalPlay();
                    }
                    break;
                case PausedState:
                    if (m_stream) {
                        internalPause();
                    }
                    break;
                case KeepState:
                    switch (previousState) {
                    case Phonon::PlayingState:
                    case Phonon::BufferingState:
                        if (m_stream) {
                            internalPlay();
                        }
                        break;
                    case Phonon::PausedState:
                        if (m_stream) {
                            internalPause();
                        }
                        break;
                    case Phonon::LoadingState:
                    case Phonon::StoppedState:
                    case Phonon::ErrorState:
                        break;
                    }
                }
            }
        }
        return true;
    case Event::TransitionTypeChanged:
        if (m_stream) {
            if (m_useGaplessPlayback) {
                kDebug(610) << "XINE_PARAM_EARLY_FINISHED_EVENT: 1";
                xine_set_param(m_stream, XINE_PARAM_EARLY_FINISHED_EVENT, 1);
#ifdef XINE_PARAM_DELAY_FINISHED_EVENT
            } else if (m_transitionGap > 0) {
                kDebug(610) << "XINE_PARAM_DELAY_FINISHED_EVENT:" << m_transitionGap;
                xine_set_param(m_stream, XINE_PARAM_DELAY_FINISHED_EVENT, m_transitionGap);
#endif // XINE_PARAM_DELAY_FINISHED_EVENT
            } else {
                kDebug(610) << "XINE_PARAM_EARLY_FINISHED_EVENT: 0";
                xine_set_param(m_stream, XINE_PARAM_EARLY_FINISHED_EVENT, 0);
            }
        }
        ev->accept();
        return true;
    case Event::RewireVideoToNull:
        ev->accept();
        {
            QMutexLocker locker(&m_mutex);
            if (!m_stream) {
                return true;
            }
            QMutexLocker portLocker(&m_portMutex);
            kDebug(610) << "rewiring ports";
            xine_post_out_t *videoSource = xine_get_video_source(m_stream);
            xine_video_port_t *videoPort = XineEngine::nullVideoPort();
            xine_post_wire_video_port(videoSource, videoPort);
            m_waitingForRewire.wakeAll();
        }
        return true;
    case Event::PlayCommand:
        ev->accept();
        if (m_mediaObject->sinks().isEmpty()) {
            kWarning(610) << "request to play a stream, but no valid audio/video outputs are given/available";
            error(Phonon::FatalError, i18n("Playback failed because no valid audio or video outputs are available"));
            return true;
        }
        if (m_state == Phonon::ErrorState || m_state == Phonon::PlayingState) {
            return true;
        }
        Q_ASSERT(!m_mrl.isEmpty());
        /*if (m_mrl.isEmpty()) {
            kError(610) << "PlayCommand: m_mrl is empty. This should not happen.";
            error(Phonon::NormalError, i18n("Request to play without media data"));
            return true;
        } */
        if (!m_stream) {
            QMutexLocker locker(&m_mutex);
            createStream();
            if (!m_stream) {
                kError(610) << "PlayCommand: createStream didn't create a stream. This should not happen.";
                error(Phonon::FatalError, i18n("Xine failed to create a stream."));
                return true;
            }
        }
        if (m_state == Phonon::PausedState) {
            xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
            changeState(Phonon::PlayingState);
        } else {
            //X                 int total;
            //X                 if (xine_get_pos_length(stream(), 0, &m_startTime, &total) == 1) {
            //X                     if (total > 0 && m_startTime < total && m_startTime >= 0)
            //X                         m_startTime = -1;
            //X                 } else {
            //X                     m_startTime = -1;
            //X                 }
            if (xine_get_status(m_stream) == XINE_STATUS_IDLE) {
                kDebug(610) << "calling xineOpen from PlayCommand";
                if (!xineOpen(Phonon::BufferingState)) {
                    return true;
                }
            }
            internalPlay();
        }
        return true;
    case Event::PauseCommand:
        ev->accept();
        if (m_state == Phonon::ErrorState || m_state == Phonon::PausedState) {
            return true;
        }
        Q_ASSERT(!m_mrl.isEmpty());
        /*if (m_mrl.isEmpty()) {
            kError(610) << "PauseCommand: m_mrl is empty. This should not happen.";
            error(Phonon::NormalError, i18n("Request to pause without media data"));
            return true;
        } */
        if (!m_stream) {
            QMutexLocker locker(&m_mutex);
            createStream();
            if (!m_stream) {
                kError(610) << "PauseCommand: createStream didn't create a stream. This should not happen.";
                error(Phonon::FatalError, i18n("Xine failed to create a stream."));
                return true;
            }
        }
        if (xine_get_status(m_stream) == XINE_STATUS_IDLE) {
            kDebug(610) << "calling xineOpen from PauseCommand";
            if (!xineOpen(Phonon::StoppedState)) {
                return true;
            }
        }
        internalPause();
        return true;
    case Event::StopCommand:
        ev->accept();
        if (m_state == Phonon::ErrorState || m_state == Phonon::LoadingState || m_state == Phonon::StoppedState) {
            return true;
        }
        Q_ASSERT(!m_mrl.isEmpty());
        /*if (m_mrl.isEmpty()) {
            kError(610) << "StopCommand: m_mrl is empty. This should not happen.";
            error(Phonon::NormalError, i18n("Request to stop without media data"));
            return true;
        } */
        if (!m_stream) {
            QMutexLocker locker(&m_mutex);
            createStream();
            if (!m_stream) {
                kError(610) << "StopCommand: createStream didn't create a stream. This should not happen.";
                error(Phonon::FatalError, i18n("Xine failed to create a stream."));
                return true;
            }
        }
        xine_stop(m_stream);
        changeState(Phonon::StoppedState);
        return true;
    case Event::SetTickInterval:
        ev->accept();
        {
            SetTickIntervalEvent *e = static_cast<SetTickIntervalEvent *>(ev);
            if (e->interval <= 0) {
                // disable ticks
                m_ticking = false;
                m_tickTimer.stop();
                //kDebug(610) << "tickTimer stopped.";
            } else {
                m_tickTimer.setInterval(e->interval);
                if (m_ticking == false && m_state == Phonon::PlayingState) {
                    m_tickTimer.start();
                    //kDebug(610) << "tickTimer started.";
                }
                m_ticking = true;
            }
        }
        return true;
    case Event::SetPrefinishMark:
        ev->accept();
        {
            SetPrefinishMarkEvent *e = static_cast<SetPrefinishMarkEvent *>(ev);
            m_prefinishMark = e->time;
            if (m_prefinishMark > 0) {
                updateTime();
                if (m_currentTime < m_totalTime - m_prefinishMark) { // not about to finish
                    m_prefinishMarkReachedNotEmitted = true;
                    if (m_state == Phonon::PlayingState) {
                        emitAboutToFinishIn(m_totalTime - m_prefinishMark - m_currentTime);
                    }
                }
            }
        }
        return true;
    case Event::SeekCommand:
        m_lastSeekCommand = 0;
        ev->accept();
        if (m_state == Phonon::ErrorState || !m_isSeekable) {
            return true;
        }
        {
            SeekCommandEvent *e = static_cast<SeekCommandEvent *>(ev);
            if (!e->valid) { // a newer SeekCommand is in the pipe, ignore this one
                return true;
            }
            switch(m_state) {
            case Phonon::PausedState:
            case Phonon::BufferingState:
            case Phonon::PlayingState:
                kDebug(610) << "seeking xine stream to " << e->time << "ms";
                // xine_trick_mode aborts :(
                //if (0 == xine_trick_mode(m_stream, XINE_TRICK_MODE_SEEK_TO_TIME, time)) {
                xine_play(m_stream, 0, e->time);

#ifdef XINE_PARAM_DELAY_FINISHED_EVENT
                if (!m_useGaplessPlayback && m_transitionGap > 0) {
                    kDebug(610) << "XINE_PARAM_DELAY_FINISHED_EVENT:" << m_transitionGap;
                    xine_set_param(m_stream, XINE_PARAM_DELAY_FINISHED_EVENT, m_transitionGap);
                }
#endif // XINE_PARAM_DELAY_FINISHED_EVENT

                if (Phonon::PausedState == m_state) {
                    // go back to paused speed after seek
                    xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
                } else if (Phonon::PlayingState == m_state) {
                    gettimeofday(&m_lastTimeUpdate, 0);
                }
                //}
                break;
            case Phonon::StoppedState:
            case Phonon::ErrorState:
            case Phonon::LoadingState:
                return true; // cannot seek
            }
            m_currentTime = e->time;
            const int timeToSignal = m_totalTime - m_prefinishMark - e->time;
            if (m_prefinishMark > 0) {
                if (timeToSignal > 0) { // not about to finish
                    m_prefinishMarkReachedNotEmitted = true;
                    emitAboutToFinishIn(timeToSignal);
                } else if (m_prefinishMarkReachedNotEmitted) {
                    m_prefinishMarkReachedNotEmitted = false;
                    kDebug(610) << "emitting prefinishMarkReached(" << timeToSignal + m_prefinishMark << ")";
                    emit prefinishMarkReached(timeToSignal + m_prefinishMark);
                }
            }
        }
        return true;
    default:
        return QObject::event(ev);
    }
}

// called from main thread
void XineStream::closeBlocking()
{
    m_mutex.lock();
    m_closing = true;
    if (m_stream && xine_get_status(m_stream) != XINE_STATUS_IDLE) {
        // this event will call xine_close
        QCoreApplication::postEvent(this, new MrlChangedEvent(QByteArray(), StoppedState));

        // wait until the xine_close is done
        m_waitingForClose.wait(&m_mutex);
        //m_closing = false;
    }
    m_mutex.unlock();
}

// called from main thread
void XineStream::setError(Phonon::ErrorType type, const QString &reason)
{
    QCoreApplication::postEvent(this, new ErrorEvent(type, reason));
}

xine_post_out_t *XineStream::audioOutputPort() const
{
    if (!m_stream) {
        return 0;
    }
    return xine_get_audio_source(m_stream);
}

xine_post_out_t *XineStream::videoOutputPort() const
{
    if (!m_stream) {
        return 0;
    }
    if (m_deinterlacer) {
        return xine_post_output(m_deinterlacer, "deinterlaced video");
    }
    return xine_get_video_source(m_stream);
}

// called from main thread
void XineStream::setUrl(const KUrl &url)
{
    setMrl(url.url().toUtf8());
}

// called from main thread
void XineStream::setMrl(const QByteArray &mrl, StateForNewMrl sfnm)
{
    kDebug(610) << mrl << ", " << sfnm;
    QCoreApplication::postEvent(this, new MrlChangedEvent(mrl, sfnm));
}

// called from main thread
void XineStream::play()
{
    QCoreApplication::postEvent(this, new QEVENT(PlayCommand));
}

// called from main thread
void XineStream::pause()
{
    QCoreApplication::postEvent(this, new QEVENT(PauseCommand));
}

// called from main thread
void XineStream::stop()
{
    QCoreApplication::postEvent(this, new QEVENT(StopCommand));
}

// called from main thread
void XineStream::seek(qint64 time)
{
    if (m_lastSeekCommand) {
        // FIXME: There's a race here in that the SeekCommand handler might be done and the event
        // deleted in between the check and the assignment.
        m_lastSeekCommand->valid = false;
    }
    m_lastSeekCommand = new SeekCommandEvent(time);
    QCoreApplication::postEvent(this, m_lastSeekCommand);
}

// xine thread
bool XineStream::updateTime()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    if (!m_stream) {
        return false;
    }

    if (xine_get_status(m_stream) == XINE_STATUS_IDLE) {
        kDebug(610) << "calling xineOpen from ";
        if (!xineOpen(Phonon::StoppedState)) {
            return false;
        }
    }

    QMutexLocker locker(&m_updateTimeMutex);
    int newTotalTime;
    int newCurrentTime;
    if (xine_get_pos_length(m_stream, 0, &newCurrentTime, &newTotalTime) != 1) {
        //m_currentTime = -1;
        //m_totalTime = -1;
        //m_lastTimeUpdate.tv_sec = 0;
        return false;
    }
    if (newTotalTime != m_totalTime) {
        m_totalTime = newTotalTime;
        emit length(m_totalTime);
    }
    if (newCurrentTime <= 0) {
        // are we seeking? when xine seeks xine_get_pos_length returns 0 for m_currentTime
        //m_lastTimeUpdate.tv_sec = 0;
        // XineStream::currentTime will still return the old value counting with gettimeofday
        return false;
    }
    if (m_state == Phonon::PlayingState && m_currentTime != newCurrentTime) {
        gettimeofday(&m_lastTimeUpdate, 0);
    } else {
        m_lastTimeUpdate.tv_sec = 0;
    }
    m_currentTime = newCurrentTime;
    return true;
}

// xine thread
void XineStream::emitAboutToFinishIn(int timeToAboutToFinishSignal)
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    kDebug(610) << timeToAboutToFinishSignal;
    Q_ASSERT(m_prefinishMark > 0);
    if (!m_prefinishMarkTimer) {
        m_prefinishMarkTimer = new QTimer(this);
        //m_prefinishMarkTimer->setObjectName("prefinishMarkReached timer");
        Q_ASSERT(m_prefinishMarkTimer->thread() == XineEngine::thread());
        m_prefinishMarkTimer->setSingleShot(true);
        connect(m_prefinishMarkTimer, SIGNAL(timeout()), SLOT(emitAboutToFinish()), Qt::DirectConnection);
    }
    timeToAboutToFinishSignal -= 400; // xine is not very accurate wrt time info, so better look too
                                      // often than too late
    if (timeToAboutToFinishSignal < 0) {
        timeToAboutToFinishSignal = 0;
    }
    kDebug(610) << timeToAboutToFinishSignal;
    m_prefinishMarkTimer->start(timeToAboutToFinishSignal);
}

// xine thread
void XineStream::emitAboutToFinish()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    kDebug(610) << m_prefinishMarkReachedNotEmitted << ", " << m_prefinishMark;
    if (m_prefinishMarkReachedNotEmitted && m_prefinishMark > 0) {
        updateTime();
        const int remainingTime = m_totalTime - m_currentTime;

        kDebug(610) << remainingTime;
        if (remainingTime <= m_prefinishMark + 150) {
            m_prefinishMarkReachedNotEmitted = false;
            kDebug(610) << "emitting prefinishMarkReached(" << remainingTime << ")";
            emit prefinishMarkReached(remainingTime);
        } else {
            emitAboutToFinishIn(remainingTime - m_prefinishMark);
        }
    }
}

// xine thread
void XineStream::timerEvent(QTimerEvent *event)
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    if (m_waitForPlayingTimerId == event->timerId()) {
        if (m_state != Phonon::BufferingState) {
            // the state has already changed somewhere else (probably from XineProgressEvents)
            killTimer(m_waitForPlayingTimerId);
            m_waitForPlayingTimerId = -1;
            return;
        }
        if (updateTime()) {
            changeState(Phonon::PlayingState);
            killTimer(m_waitForPlayingTimerId);
            m_waitForPlayingTimerId = -1;
        } else {
            if (xine_get_status(m_stream) == XINE_STATUS_IDLE) {
                changeState(Phonon::StoppedState);
                killTimer(m_waitForPlayingTimerId);
                m_waitForPlayingTimerId = -1;
            } else {
                kDebug(610) << "waiting";
            }
        }
    } else {
        QObject::timerEvent(event);
    }
}

// xine thread
void XineStream::emitTick()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    if (!updateTime()) {
        kDebug(610) << "no useful time information available. skipped.";
        return;
    }
    if (m_ticking) {
        //kDebug(610) << m_currentTime;
        emit tick(m_currentTime);
    }
    if (m_prefinishMarkReachedNotEmitted && m_prefinishMark > 0) {
        const int remainingTime = m_totalTime - m_currentTime;
        const int timeToAboutToFinishSignal = remainingTime - m_prefinishMark;
        if (timeToAboutToFinishSignal <= m_tickTimer.interval()) { // about to finish
            if (timeToAboutToFinishSignal > 100) {
                emitAboutToFinishIn(timeToAboutToFinishSignal);
            } else {
                m_prefinishMarkReachedNotEmitted = false;
                kDebug(610) << "emitting prefinishMarkReached(" << remainingTime << ")";
                emit prefinishMarkReached(remainingTime);
            }
        }
    }
}

// xine thread
void XineStream::getStartTime()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
//X     if (m_startTime == -1 || m_startTime == 0) {
//X         int total;
//X         if (xine_get_pos_length(m_stream, 0, &m_startTime, &total) == 1) {
//X             if(total > 0 && m_startTime < total && m_startTime >= 0)
//X                 m_startTime = -1;
//X         } else {
//X             m_startTime = -1;
//X         }
//X     }
//X     if (m_startTime == -1 || m_startTime == 0) {
//X         QTimer::singleShot(30, this, SLOT(getStartTime()));
//X     }
}

void XineStream::internalPause()
{
    if (m_state == Phonon::PlayingState || m_state == Phonon::BufferingState) {
        xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
        changeState(Phonon::PausedState);
    } else {
        xine_play(m_stream, 0, 0);
        xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
        changeState(Phonon::PausedState);
    }
}

void XineStream::internalPlay()
{
    xine_play(m_stream, 0, 0);

#ifdef XINE_PARAM_DELAY_FINISHED_EVENT
    if (!m_useGaplessPlayback && m_transitionGap > 0) {
        kDebug(610) << "XINE_PARAM_DELAY_FINISHED_EVENT:" << m_transitionGap;
        xine_set_param(m_stream, XINE_PARAM_DELAY_FINISHED_EVENT, m_transitionGap);
    }
#endif // XINE_PARAM_DELAY_FINISHED_EVENT

    if (updateTime()) {
        changeState(Phonon::PlayingState);
    } else {
        changeState(Phonon::BufferingState);
        m_waitForPlayingTimerId = startTimer(50);
    }
}

void XineStream::setMrlInternal(const QByteArray &newMrl)
{
    if (newMrl != m_mrl) {
        if (m_mrl.startsWith("kbytestream:/")) {
            Q_ASSERT(m_byteStream);
            Q_ASSERT(ByteStream::fromMrl(m_mrl) == m_byteStream);
            if (!m_byteStream->ref.deref()) {
                m_byteStream->deleteLater();
            }
            m_byteStream = 0;
        }
        m_mrl = newMrl;
        if (m_mrl.startsWith("kbytestream:/")) {
            Q_ASSERT(m_byteStream == 0);
            m_byteStream = ByteStream::fromMrl(m_mrl);
            Q_ASSERT(m_byteStream);
            m_byteStream->ref.ref();
        }
    }
}

void XineStream::handleDownstreamEvent(Event *e)
{
    emit downstreamEvent(e);
}

} // namespace Xine
} // namespace Phonon

#include "xinestream.moc"

// vim: sw=4 ts=4
