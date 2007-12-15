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

#include "mediaobject.h"
#include "videowidget.h"
#include "audiooutput.h"

#include <QtCore/QVector>
#include <QtCore/QTimerEvent>
#include <QtCore/QTimer>
#include <QtGui/QWidget>


#include <phonon/abstractmediastream.h>

#include <qdebug.h>

#include <initguid.h>
#include <qnetwork.h>
#include <comdef.h>
#include <evcode.h>

#define TIMER_INTERVAL 25 //... ms for the timer that polls the current state
#define PRELOAD_TIME 2000 // 2 sencods to load a source

namespace Phonon
{
    namespace DS9
    {

        MediaObject::MediaObject(QObject *parent) : BackendNode(parent),
            m_errorType(Phonon::NoError),
            m_state(Phonon::StoppedState),
            m_prefinishMark(0),
            m_tickInterval(0)
        {
            m_graphs << new MediaGraph(this, 0)
            //the reserved graph (used for cross-fading and gapless transition)
                << new MediaGraph(this, 1);

            //really special case
            m_mediaObject = this;
        }

        MediaObject::~MediaObject()
        {
            qDeleteAll(m_graphs);
        }

        MediaGraph *MediaObject::currentGraph() const
        {
            return m_graphs.first();
        }

        MediaGraph *MediaObject::nextGraph() const
        {
            return m_graphs.last();
        }

        //utility function to save the graph to a file
        void MediaObject::timerEvent(QTimerEvent *e)
        {
            if (e->timerId() == m_tickTimer.timerId()) {

                const qint64 remaining = remainingTime();
                const qint64 current = currentTime();

                if ( m_tickInterval != 0 && (current % m_tickInterval) <= (TIMER_INTERVAL/2)) {
                    emit tick(current);
                }

                if (qAbs(remaining + m_transitionTime - PRELOAD_TIME) <= (TIMER_INTERVAL/2)) {
                    //let's take a 2 seconds time time to actually load the next file
                    emit aboutToFinish();
                }

                if (m_transitionTime < 0 && nextGraph()->mediaSource().type() != MediaSource::Invalid) {
                    //we are currently crossfading
                    foreach(AudioOutput *audio, m_audioOutputs) {
                        audio->setCrossFadingProgress( currentGraph()->index(), qMin(1., qreal(current) / qreal(-m_transitionTime)));
                    }
                }

                if (m_transitionTime < 0 && qAbs(remaining + m_transitionTime) <= (TIMER_INTERVAL / 2)) {
                    //we need to switch graphs to run the next source in the queue (with cross-fading)
                    switchToNextSource();
                }

                if ( qAbs(remaining - m_prefinishMark) <= (TIMER_INTERVAL/2)) {
                    emit prefinishMarkReached( remaining );
                }

                ///TODO: do the management of the buffer elsewhere (porbably in a thread...)
                /*ComPointer<IAsyncReader> reader(m_graph.realSource());
                if (reader) {
                    LONGLONG total, available;
                    if (reader->Length(&total, &available) != E_UNEXPECTED) {
                        const int percent = qRound(qreal(available) / qreal(total));
                        //qDebug() << "available" << percent << '%';
                        emit bufferStatus(percent);
                    }
                }

                ComPointer<IAMOpenProgress> reader2(m_graph.realSource());
                if (reader2) {
                    LONGLONG total, available;
                    if (reader2->QueryProgress(&total, &available) != E_UNEXPECTED) {
                        const int percent = qRound(qreal(available) / qreal(total));
                        //qDebug() << "available" << percent << '%';
                        emit bufferStatus(percent);
                    }
                }*/
            }
        }

        void MediaObject::switchToNextSource()
        {
            m_graphs.swap(0,1); //swap the graphs

            //we tell the video widgets to switch now to the new source
            foreach(VideoWidget *video, m_videoWidgets) {
                video->setCurrentGraph(currentGraph()->index());
            }

            //this manages only gapless transitions
            if (currentGraph()->mediaSource().type() != MediaSource::Invalid) {
                play();
            }

            emit metaDataChanged(currentGraph()->metadata());
            emit currentSourceChanged(currentGraph()->mediaSource());
        }

        State MediaObject::state() const
        {
            return m_state;
        }

        bool MediaObject::hasVideo() const
        {
            return currentGraph()->hasVideo();
        }

        bool MediaObject::isSeekable() const
        {
            return currentGraph()->isSeekable();
        }

        qint64 MediaObject::totalTime() const
        {
            return currentGraph()->totalTime();
        }

        qint64 MediaObject::currentTime() const
        {
            return currentGraph()->currentTime();
        }

        qint32 MediaObject::tickInterval() const
        {
            return m_tickInterval;
        }

        void MediaObject::setTickInterval(qint32 newTickInterval)
        {
            m_tickInterval = newTickInterval;
        }

        void MediaObject::play()
        {
            if (!catchComError(currentGraph()->play())) {
                setState(Phonon::PlayingState);
            }
        }

        QString MediaObject::errorString() const
        {
            return m_errorString;
        }

        Phonon::ErrorType MediaObject::errorType() const
        {
            return m_errorType;
        }

        void MediaObject::setState(State newstate)
        {
            if (newstate == m_state) {
                return;
            }

            //manage the timer
            if (newstate == Phonon::PlayingState
                && (m_tickInterval != 0 || m_prefinishMark != 0)) {
                m_tickTimer.start(TIMER_INTERVAL, this);
            } else {
                m_tickTimer.stop();
            }

            State oldstate = m_state;
            m_state = newstate;
            emit stateChanged(newstate, oldstate);
        }


        qint32 MediaObject::prefinishMark() const
        {
            return m_prefinishMark;
        }

        void MediaObject::setPrefinishMark(qint32 newPrefinishMark)
        {
            m_prefinishMark = newPrefinishMark;
        }

        qint32 MediaObject::transitionTime() const
        {
            return m_transitionTime;
        }

        void MediaObject::setTransitionTime(qint32 time)
        {
            m_transitionTime = time;
        }

        qint64 MediaObject::remainingTime() const
        {
            return totalTime() - currentTime();
        }


        MediaSource MediaObject::source() const
        {
            return currentGraph()->mediaSource();
        }

        void MediaObject::setNextSource(const MediaSource &source)
        {
            switch(m_state)
            {
            case Phonon::PlayingState:
            case Phonon::LoadingState:
            case Phonon::PausedState:
            case Phonon::BufferingState:
                nextGraph()->loadSource(source); //let's preload the source
                break;
            case Phonon::StoppedState:
            case Phonon::ErrorState:
                setSource(source);
                break;
            }
        }

        void MediaObject::setSource(const MediaSource &source)
        {
            const bool oldHasVideo = currentGraph()->hasVideo();

            setState(Phonon::LoadingState);

            if (catchComError(currentGraph()->loadSource(source))) {
                return;
            }

            if (oldHasVideo != currentGraph()->hasVideo()) {
                emit hasVideoChanged(currentGraph()->hasVideo());
            }

            emit metaDataChanged(currentGraph()->metadata());
            emit totalTimeChanged(currentGraph()->totalTime());
            setState(Phonon::StoppedState);
        }

        void MediaObject::pause()
        {
            switch (state())
            {
            case PlayingState:
            case BufferingState:
            case StoppedState:
                if (currentGraph()->pause() == S_OK) {
                    setState(Phonon::PausedState);
                }
                break;
            case PausedState:
            case LoadingState:
            case ErrorState:
                break;
            }
        }

        void MediaObject::stop()
        {
            switch (state())
            {
            case PlayingState:
            case BufferingState:
            case PausedState:
                if (currentGraph()->stop() == S_OK) {
                    setState(Phonon::StoppedState);
                }
                break;
            case StoppedState:
            case LoadingState:
            case ErrorState:
                break;
            }
        }

        void MediaObject::seek(qint64 time)
        {
            if (isSeekable()) {
                currentGraph()->seek(time);
            }
        }

        bool MediaObject::catchComError(HRESULT hr) const
        {
            //TODO: we should raise error here (changing the state of the mediaObject)
            switch(hr)
            {
            case S_OK:
                m_errorType = NoError;
                break;
            default:
                qDebug() << "an error occurred" << QString::fromWCharArray(_com_error(hr).ErrorMessage());
                m_errorType = Phonon::FatalError;
                break;
            }
            m_errorString = QString::fromWCharArray(_com_error(hr).ErrorMessage());


            return m_errorType != NoError;
        }


        void MediaObject::grabNode(BackendNode *node)
        {
            foreach(MediaGraph *graph, m_graphs) {
                graph->grabNode(node);
            }
            node->setMediaObject(this);
        }

        bool MediaObject::connectNodes(BackendNode *source, BackendNode *sink)
        {
            bool ret = true;
            foreach(MediaGraph *graph, m_graphs) {
                ret = ret && graph->connectNodes(source, sink);
            }
            if (ret) {
                if (VideoWidget *video = qobject_cast<VideoWidget*>(sink)) {
                    m_videoWidgets += video;
                } else if (AudioOutput *audio = qobject_cast<AudioOutput*>(sink)) {
                    m_audioOutputs += audio;
                }
            }
            return ret;
        }

        bool MediaObject::disconnectNodes(BackendNode *source, BackendNode *sink)
        {
            bool ret = true;
            foreach(MediaGraph *graph, m_graphs) {
                ret = ret && graph->disconnectNodes(source, sink);
            }
            if (ret) {
                if (VideoWidget *video = qobject_cast<VideoWidget*>(sink)) {
                    m_videoWidgets -= video;
                } else if (AudioOutput *audio = qobject_cast<AudioOutput*>(sink)) {
                    m_audioOutputs -= audio;
                }
            }
            return ret;
        }

        void MediaObject::handleEvents(MediaGraph *graph, long eventCode, long param1)
        {
            QString eventDescription;
            QTextStream o(&eventDescription);
            switch (eventCode)
            {
            case EC_ACTIVATE: o << "EC_ACTIVATE: A video window is being " << (param1 ? "ACTIVATED" : "DEACTIVATED"); break;
            case EC_BUFFERING_DATA:
                if (graph == currentGraph()) {
                    setState(param1 ? Phonon::BufferingState : Phonon::PlayingState);
                }
                break;
            case EC_BUILT: o << "EC_BUILT: Send by the Video Control when a graph has been built. Not forwarded to applications."; break;
            case EC_CLOCK_CHANGED: o << "EC_CLOCK_CHANGED"; break;
            case EC_CLOCK_UNSET: o << "EC_CLOCK_UNSET: The clock provider was disconnected."; break;
            case EC_CODECAPI_EVENT: o << "EC_CODECAPI_EVENT: Sent by an encoder to signal an encoding event."; break;
            case EC_COMPLETE:
                if (graph == currentGraph()) {
                    graph->stop();
                    if (m_transitionTime >= PRELOAD_TIME) {
                        emit aboutToFinish(); //give a chance to the frontend to give a next source
                    }

                    if (nextGraph()->mediaSource().type() == MediaSource::Invalid) {
                        //this is the last source, we simply finish
                        emit finished();
                    } else if (m_transitionTime == 0) {
                        //gapless transition
                        switchToNextSource(); //let's call the function immediately
                        foreach(AudioOutput *audio, m_audioOutputs) {
                            audio->setCrossFadingProgress( currentGraph()->index(), 1.); //cross-fading is in any case finished
                        }
                    } else if (m_transitionTime > 0) {
                        //management of the transition (if it is >= 0)
                        QTimer::singleShot(m_transitionTime, this, SLOT(switchToNextSource()));
                    }
                } else {
                    //it is just the end of the previous source (in case of cross-fading)
                    graph->cleanup();
                    foreach(AudioOutput *audio, m_audioOutputs) {
                        audio->setCrossFadingProgress( currentGraph()->index(), 1.); //cross-fading is in any case finished
                    }

                }
                break;
            case EC_DEVICE_LOST: o << "EC_DEVICE_LOST: A Plug and Play device was removed or has become available again."; break;
            case EC_DISPLAY_CHANGED: o << "EC_DISPLAY_CHANGED: The display mode has changed."; break;
            case EC_END_OF_SEGMENT: o << "EC_END_OF_SEGMENT: The end of a segment has been reached."; break;
            case EC_ERROR_STILLPLAYING: o << "EC_ERROR_STILLPLAYING: An asynchronous command to run the graph has failed."; break;
            case EC_ERRORABORT: o << "EC_ERRORABORT: An operation was aborted because of an error."; break;
            case EC_EXTDEVICE_MODE_CHANGE: o << "EC_EXTDEVICE_MODE_CHANGE: Not supported."; break;
            case EC_FULLSCREEN_LOST: o << "EC_FULLSCREEN_LOST: The video renderer is switching out of full-screen mode."; break;
            case EC_GRAPH_CHANGED: o << "EC_GRAPH_CHANGED: The filter graph has changed."; break;
            case EC_LENGTH_CHANGED: o << "EC_LENGTH_CHANGED: The length of a source has changed."; break;
            case EC_NEED_RESTART: o << "EC_NEED_RESTART: A filter is requesting that the graph be restarted."; break;
            case EC_NOTIFY_WINDOW: o << "EC_NOTIFY_WINDOW: Notifies a filter of the video renderer's window."; break;
            case EC_OLE_EVENT: o << "EC_OLE_EVENT: A filter is passing a text string to the application."; break;
            case EC_OPENING_FILE: o << "EC_OPENING_FILE: The graph is opening a file, or has finished opening a file."; break;
            case EC_PALETTE_CHANGED: o << "EC_PALETTE_CHANGED: The video palette has changed."; break;
            case EC_PAUSED: o << "EC_PAUSED: A pause request has completed."; break;
            case EC_PREPROCESS_COMPLETE: o << "EC_PREPROCESS_COMPLETE: Sent by the WM ASF Writer filter when it completes the pre-processing for multipass encoding."; break;
            case EC_QUALITY_CHANGE: o << "EC_QUALITY_CHANGE: The graph is dropping samples, for quality control."; break;
            case EC_REPAINT: o << "EC_REPAINT: A video renderer requires a repaint."; break;
            case EC_SEGMENT_STARTED: o << "EC_SEGMENT_STARTED: A new segment has started."; break;
            case EC_SHUTTING_DOWN: o << "EC_SHUTTING_DOWN: The filter graph is shutting down, prior to being destroyed."; break;
            case EC_SNDDEV_IN_ERROR: o << "EC_SNDDEV_IN_ERROR: A device error has occurred in an audio capture filter."; break;
            case EC_SNDDEV_OUT_ERROR: o << "EC_SNDDEV_OUT_ERROR: A device error has occurred in an audio renderer filter."; break;
            case EC_STARVATION: o << "EC_STARVATION: A filter is not receiving enough data."; break;
            case EC_STATE_CHANGE: o << "EC_STATE_CHANGE: The filter graph has changed state."; break;
            case EC_STEP_COMPLETE: o << "EC_STEP_COMPLETE: A filter performing frame stepping has stepped the specified number of frames."; break;
            case EC_STREAM_CONTROL_STARTED: o << "EC_STREAM_CONTROL_STARTED: A stream-control start command has taken effect."; break;
            case EC_STREAM_CONTROL_STOPPED: o << "EC_STREAM_CONTROL_STOPPED: A stream-control stop command has taken effect."; break;
            case EC_STREAM_ERROR_STILLPLAYING: o << "EC_STREAM_ERROR_STILLPLAYING: An error has occurred in a stream. The stream is still playing."; break;
            case EC_STREAM_ERROR_STOPPED: o << "EC_STREAM_ERROR_STOPPED: A stream has stopped because of an error."; break;
            case EC_TIMECODE_AVAILABLE: o << "EC_TIMECODE_AVAILABLE: Not supported."; break;
            case EC_UNBUILT: o << "Sent by the Video Control when a graph has been torn down. Not forwarded to applications."; break;
            case EC_USERABORT: o << "EC_USERABORT: Send by the Video Control when a graph has been torn down. Not forwarded to applications."; break;
            case EC_VIDEO_SIZE_CHANGED: o << "EC_VIDEO_SIZE_CHANGED"; break;
            case EC_VMR_RECONNECTION_FAILED: o << "EC_VMR_RECONNECTION_FAILED: Sent by the VMR-7 and the VMR-9 when it was unable to accept a dynamic format change request from the upstream decoder."; break;
            case EC_VMR_RENDERDEVICE_SET: o << "EC_VMR_RENDERDEVICE_SET: Sent when the VMR has selected its rendering mechanism."; break;
            case EC_VMR_SURFACE_FLIPPED: o << "EC_VMR_SURFACE_FLIPPED: Sent when the VMR-7's allocator presenter has called the DirectDraw Flip method on the surface being presented."; break;
            case EC_WINDOW_DESTROYED: o << "EC_WINDOW_DESTROYED: The video renderer was destroyed or removed from the graph"; break;
            case EC_WMT_EVENT: o << "EC_WMT_EVENT: Sent by the Windows Media Format SDK when an application uses the ASF Reader filter to play ASF files protected by digital rights management (DRM)."; break;
            case EC_WMT_INDEX_EVENT: o << "EC_WMT_INDEX_EVENT: Sent by the Windows Media Format SDK when an application uses the ASF Writer to index Windows Media Video files."; break;

                //documented by Microsoft but not supported in the Platform SDK
                //              case EC_BANDWIDTHCHANGE : o << "EC_BANDWIDTHCHANGE: not supported"; break;
                //              case EC_CONTENTPROPERTY_CHANGED: o << "EC_CONTENTPROPERTY_CHANGED: not supported."; break;
                //              case EC_EOS_SOON: o << "EC_EOS_SOON: not supported"; break;
                //              case EC_ERRORABORTEX: o << "EC_ERRORABORTEX: An operation was aborted because of an error."; break;
                //              case EC_FILE_CLOSED: o << "EC_FILE_CLOSED: The source file was closed because of an unexpected event."; break;
                //              case EC_LOADSTATUS: o << "EC_LOADSTATUS: Notifies the application of progress when opening a network file."; break;
                //              case EC_MARKER_HIT: o << "EC_MARKER_HIT: not supported."; break;
                //              case EC_NEW_PIN: o << "EC_NEW_PIN: not supported."; break;
                //              case EC_PLEASE_REOPEN: o << "EC_PLEASE_REOPEN: The source file has changed."; break;
                //              case EC_PROCESSING_LATENCY: o << "EC_PROCESSING_LATENCY: Indicates the amount of time that a component is taking to process each sample."; break;
                //              case EC_RENDER_FINISHED: o << "EC_RENDER_FINISHED: Not supported."; break;
                //              case EC_SAMPLE_LATENCY: o << "EC_SAMPLE_LATENCY: Specifies how far behind schedule a component is for processing samples."; break;
                //              case EC_SAMPLE_NEEDED: o << "EC_SAMPLE_NEEDED: Requests a new input sample from the Enhanced Video Renderer (EVR) filter."; break;
                //              case EC_SCRUB_TIME: o << "EC_SCRUB_TIME: Specifies the time stamp for the most recent frame step."; break;
                //              case EC_STATUS: o << "EC_STATUS: Contains two arbitrary status strings."; break;
                //              case EC_VIDEOFRAMEREADY: o << "EC_VIDEOFRAMEREADY: A video frame is ready for display."; break;

            default: o << "Unknown event" << eventCode << "(" << param1 << ")"; break;
            }
            qDebug() << eventDescription;
        }
    }
}

#include "moc_mediaobject.cpp"
