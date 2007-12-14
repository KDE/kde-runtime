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

#include <cmath>
#include <gst/interfaces/propertyprobe.h>
#include "common.h"
#include "mediaobject.h"
#include "videowidget.h"
#include "message.h"
#include "backend.h"

#include <QtCore>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtCore/QFile>
#include <QtCore/QByteRef>
#include <QtCore/QStringList>
#include <QtCore/QEvent>
#include <QApplication>

#include <unistd.h>             // for usleep

namespace Phonon
{
namespace Gstreamer
{

MediaObject::MediaObject(Backend *backend, QObject *parent)
        : QObject(parent)
        , MediaNode(backend, AudioSource | VideoSource)
        , m_state(Phonon::LoadingState)
        , m_tickTimer(new QTimer(this))
        , m_prefinishMark(0)
        , m_transitionTime(0)
        , m_prefinishMarkReachedNotEmitted(true)
        , m_aboutToFinishEmitted(false)
        , m_loading(false)
        , m_datasource(0)
        , m_decodebin(0)
        , m_audioPipe(0)
        , m_videoPipe(0)
        , m_totalTime(-1)
        , m_bufferPercent(0)
        , m_hasVideo(false)
        , m_hasAudio(false)
        , m_seekable(false)
        , m_error(Phonon::NoError)
        , m_pipeline(0)
        , m_audioGraph(0)
        , m_videoGraph(0)
{
    static int count = 0;
    m_name = "MediaObject" + QString::number(count++);

    if (!m_backend->isValid()) {
        setError(tr("Cannot start playback. \n\nCheck your Gstreamer installation and make sure you "
                    "\nhave libgstreamer-plugins-base installed."), Phonon::FatalError);
    } else {
        m_root = this;
        m_backend->addBusWatcher(this);
        createPipeline();
        connect(m_tickTimer, SIGNAL(timeout()), SLOT(emitTick()));
    }
}

MediaObject::~MediaObject()
{
    stop();
    m_backend->removeBusWatcher(this);
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
    }
    if (m_audioGraph)
        gst_object_unref(m_audioGraph);
    if (m_videoGraph)
        gst_object_unref(m_videoGraph);
}


void MediaObject::newPadAvailable (GstPad *pad)
{
    GstCaps *caps;
    GstStructure *str;
    caps = gst_pad_get_caps (pad);
    if (caps) {
        str = gst_caps_get_structure (caps, 0);
        QString mediaString(gst_structure_get_name (str));

        if (mediaString.startsWith("video")) {
            connectVideo(pad);
        } else if (mediaString.startsWith("audio")) {
            connectAudio(pad);
        } else {
            m_backend->logMessage("Could not connect pad", Backend::Warning);
        }
        gst_caps_unref (caps);
    }
}

void MediaObject::cb_newpad (GstElement *decodebin,
                             GstPad     *pad,
                             gboolean    last,
                             gpointer    data)
{
    Q_UNUSED(decodebin);
    Q_UNUSED(pad);
    Q_UNUSED(last);
    Q_UNUSED(data);

    MediaObject *media = static_cast<MediaObject*>(data);
    Q_ASSERT(media);
    media->newPadAvailable(pad);
}

void MediaObject::cb_unknown_type (GstElement *decodebin, GstPad *pad, GstCaps *caps, gpointer data)
{
    Q_UNUSED(decodebin);
    Q_UNUSED(pad);
    const gchar *typeName = gst_structure_get_name (gst_caps_get_structure (caps, 0));
    MediaObject *media = static_cast<MediaObject*>(data);
    Q_ASSERT(media);
    if (typeName) {
        gchar *capsString = gst_caps_to_string (caps);
        media->setError(QString(tr("Unknown media format: %1")).arg(capsString), Phonon::NormalError);
        g_free (capsString);
    }
}

static void notifyVideoCaps(GObject *obj, GParamSpec *, gpointer data)
{
    GstPad *pad = GST_PAD(obj);
    GstCaps *caps = gst_pad_get_caps (pad);
    Q_ASSERT(caps);

    MediaObject *media = static_cast<MediaObject*>(data);
    media->setVideoCaps(caps);
}

void MediaObject::setVideoCaps(GstCaps *caps)
{

    GstStructure *str;
    gint width, height;

    if ((str = gst_caps_get_structure (caps, 0))) {
        if (gst_structure_get_int (str, "width", &width) && gst_structure_get_int (str, "height", &height)) {
            // Let child nodes know about our new video size
            QSize size(width, height);
            MediaNodeEvent event(MediaNodeEvent::VideoSizeChanged, &size);
            notify(&event);
        }
    }
}

// Adds an element to the pipeline if not previously added
bool MediaObject::addToPipeline(GstElement *elem)
{
    bool success = true;
    if (!gst_element_get_parent(elem)) { // If not already in pipeline
        GstState state = (GST_STATE (m_decodebin) == GST_STATE_PLAYING ? GST_STATE_PLAYING : GST_STATE_PAUSED);
        // Make sure we are in the same state as the pipeline before adding
        if ( gst_element_set_state(elem, state) != GST_STATE_CHANGE_FAILURE) {
            gst_bin_add(GST_BIN(m_pipeline), elem);
        } else {
            success = false;
        }
    }
    return success;
}

void MediaObject::connectVideo(GstPad *pad)
{
    if (addToPipeline(m_videoGraph)) {
        GstPad *videopad = gst_element_get_pad (m_videoGraph, "sink");
        if (!GST_PAD_IS_LINKED (videopad) && (gst_pad_link (pad, videopad) == GST_PAD_LINK_OK)) {
            m_hasVideo = true;
            m_backend->logMessage("Video track connected");
            // Note that the notify::caps _must_ be installed after linking to work with Dapper
            g_signal_connect(pad, "notify::caps", G_CALLBACK(notifyVideoCaps), this);
            MediaNodeEvent event(MediaNodeEvent::VideoAvailable);
            notify(&event);
        }
        gst_object_unref (videopad);
    } else {
        m_backend->logMessage("The video stream could not be plugged.");
    }
}

void MediaObject::connectAudio(GstPad *pad)
{
    if (addToPipeline(m_audioGraph)) {
        GstPad *audiopad = gst_element_get_pad (m_audioGraph, "sink");
        if (!GST_PAD_IS_LINKED (audiopad) && (gst_pad_link (pad, audiopad)==GST_PAD_LINK_OK)) {
            m_hasAudio = true;
            m_backend->logMessage("Audio track connected");
        }
        gst_object_unref (audiopad);
    } else {
        m_backend->logMessage("The audio stream could not be plugged.");
    }
}

/**
 * Create a media source from a given URL.
 *
 * returns true if successful
 */
bool MediaObject::createPipefromURL(const QString &url)
{
    // Remove any existing data source
    if (m_datasource) {
        gst_bin_remove(GST_BIN(m_pipeline), m_datasource);
        gst_object_unref(m_datasource);
        m_datasource = 0;
    }

    // Verify that the uri can be parsed
    if (!gst_uri_is_valid(qPrintable(url))) {
        m_backend->logMessage(QString("%0 is not a valid URI").arg(url));
        return false;
    }

    // Create a new datasource based on the input URL
    m_datasource = gst_element_make_from_uri(GST_URI_SRC, qPrintable(url), NULL);
    if (!m_datasource)
        return false;

    // Link data source into pipeline
    gst_bin_add(GST_BIN(m_pipeline), m_datasource);
    if (!gst_element_link(m_datasource, m_decodebin)) {
        gst_bin_remove(GST_BIN(m_pipeline), m_datasource);
        return false;
    }
    return true;
}

void MediaObject::createPipeline()
{
    m_pipeline = gst_pipeline_new (NULL);
    m_decodebin = gst_element_factory_make ("decodebin", NULL);
    g_signal_connect (m_decodebin, "new-decoded-pad", G_CALLBACK (&cb_newpad), this);
    g_signal_connect (m_decodebin, "unknown-type", G_CALLBACK (&cb_unknown_type), this);

    gst_bin_add(GST_BIN(m_pipeline), m_decodebin);

    // Create a bin to contain the gst elements for this medianode

    // Set up audio graph
    m_audioGraph = gst_bin_new(NULL);
    gst_object_ref(m_audioGraph);
    m_audioPipe = gst_element_factory_make("queue", NULL);
    gst_bin_add(GST_BIN(m_audioGraph), m_audioPipe);
    GstPad *audiopad = gst_element_get_pad (m_audioPipe, "sink");
    gst_element_add_pad (m_audioGraph, gst_ghost_pad_new ("sink", audiopad));
    gst_object_unref (audiopad);

    // Set up video graph
    m_videoGraph = gst_bin_new(NULL);
    gst_object_ref(m_videoGraph);
    m_videoPipe = gst_element_factory_make("queue", NULL);
    gst_bin_add(GST_BIN(m_videoGraph), m_videoPipe);
    GstPad *videopad = gst_element_get_pad (m_videoPipe, "sink");
    gst_element_add_pad (m_videoGraph, gst_ghost_pad_new ("sink", videopad));
    gst_object_unref (videopad);

    if (m_pipeline && m_decodebin && m_audioGraph && m_videoGraph && m_audioPipe && m_videoPipe)
        m_isValid = true;
    else
        m_backend->logMessage("Could not create pipeline for media object", Backend::Warning);
}

/**
 * !reimp
 */
State MediaObject::state() const
{
    return m_state;
}

/**
 * !reimp
 */
bool MediaObject::hasVideo() const
{
    return m_hasVideo;
}

/**
 * !reimp
 */
bool MediaObject::isSeekable() const
{
    return m_seekable;
}

/**
 * !reimp
 */
qint64 MediaObject::currentTime() const
{
    switch (state()) {
    case Phonon::PausedState:
    case Phonon::BufferingState:
    case Phonon::PlayingState:
        return getPipelinePos();
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        return 0;
    case Phonon::ErrorState:
        break;
    }
    return -1;
}

/**
 * !reimp
 */
qint32 MediaObject::tickInterval() const
{
    return m_tickInterval;
}

/**
 * !reimp
 */
void MediaObject::setTickInterval(qint32 newTickInterval)
{
    m_tickInterval = newTickInterval;
    if (m_tickInterval <= 0)
        m_tickTimer->setInterval(50);
    else
        m_tickTimer->setInterval(newTickInterval);
}

/**
 * !reimp
 */
void MediaObject::play()
{
    setState(Phonon::PlayingState);
}

/**
 * !reimp
 */
QString MediaObject::errorString() const
{
    return m_errorString;
}

/**
 * !reimp
 */
Phonon::ErrorType MediaObject::errorType() const
{
    return m_error;
}

/**
 * Set the current state of the mediaObject.
 *
 * !### Note that both Playing and Paused states are set immediately
 *     This should obviously be done in response to actual gstreamer state changes
 */
void MediaObject::setState(State newstate)
{
    if (!isValid())
        return;

    if (newstate == m_state)
        return;

    switch (newstate) {
    case Phonon::BufferingState:
        m_backend->logMessage("phonon state request: buffering", Backend::Info, this);
        changeState(newstate); //immediately set buffering state
        break;

    case Phonon::PausedState:
        if (m_loading)
            beginLoad();

        m_backend->logMessage("phonon state request: paused", Backend::Info, this);
        if (gst_element_set_state(m_pipeline, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE)
            changeState(Phonon::PausedState);
        break;

    case Phonon::PlayingState:
        if (m_loading)
            beginLoad();

        m_backend->logMessage("phonon state request: Playing", Backend::Info, this);
        if (gst_element_set_state(m_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE)
            changeState(Phonon::PlayingState);
        break;

    case Phonon::StoppedState:
        m_backend->logMessage("phonon state request: Stopped", Backend::Info, this);
        if (gst_element_set_state(m_pipeline, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE)
            changeState(Phonon::StoppedState);
        break;

    case Phonon::ErrorState:
        m_backend->logMessage("phonon state request : Error", Backend::Warning, this);
        m_backend->logMessage(QString("Last error : %0").arg(errorString()) , Backend::Warning, this);
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        changeState(Phonon::ErrorState); //immediately set error state
        break;

    case Phonon::LoadingState:
        m_backend->logMessage("phonon state request: Loading", Backend::Info, this);
        changeState(Phonon::LoadingState);
        break;
    }
}

/*
 * Signals that the requested state has completed
 * by emitting stateChanged and updates the internal state.
 */
void MediaObject::changeState(State newstate)
{
    if (newstate == m_state)
        return;

    emit stateChanged(newstate, m_state);
    m_state = newstate;

    switch (newstate) {
    case Phonon::PausedState:
        m_backend->logMessage("phonon state changed: paused", Backend::Info, this);
        break;

    case Phonon::BufferingState:
        m_backend->logMessage("phonon state changed: buffering", Backend::Info, this);
        break;

    case Phonon::PlayingState:
        m_backend->logMessage("phonon state changed: Playing", Backend::Info, this);
        break;

    case Phonon::StoppedState:
        m_backend->logMessage("phonon state changed: Stopped", Backend::Info, this);
        m_tickTimer->stop();
        break;

    case Phonon::ErrorState:
        m_backend->logMessage("phonon state changed : Error", Backend::Info, this);
        m_backend->logMessage(errorString(), Backend::Warning, this);
        break;

    case Phonon::LoadingState:
        m_backend->logMessage("phonon state changed: Loading", Backend::Info, this);
        break;
    }
}

void MediaObject::setError(const QString &errorString, Phonon::ErrorType error)
{
    m_errorString = errorString;
    m_error = error;
    m_tickTimer->stop();

    changeState(Phonon::ErrorState);
}

qint64 MediaObject::totalTime() const
{
    return m_totalTime;
}

qint32 MediaObject::prefinishMark() const
{
    return m_prefinishMark;
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
    return m_source;
}

void MediaObject::setNextSource(const MediaSource &source)
{
    m_nextSource = source;
}

/**
 * Update total time value from the pipeline
 */
bool MediaObject::updateTotalTime()
{
    GstFormat   format = GST_FORMAT_TIME;
    gint64 duration = 0;
    if (gst_element_query_duration (GST_ELEMENT(m_pipeline), &format, &duration)) {
        setTotalTime(duration / GST_MSECOND);
        return true;
    }
    return false;
}

/**
 * Checks if the current source is seekable
 */
void MediaObject::updateSeekable()
{
    if (!isValid())
        return;

    GstQuery *query;
    gboolean result;
    gint64 start, stop;
    query = gst_query_new_seeking(GST_FORMAT_TIME);
    result = gst_element_query (m_pipeline, query);
    if (result) {
        gboolean seekable;
        GstFormat format;
        gst_query_parse_seeking (query, &format, &seekable, &start, &stop);

        if (m_seekable != seekable) {
            m_seekable = seekable;
            emit seekableChanged(m_seekable);
        }

        if (m_seekable)
            m_backend->logMessage("Stream is seekable");
        else
            m_backend->logMessage("Stream is non-seekable");
    } else {
        m_backend->logMessage("updateSeekable query failed");
    }
    gst_query_unref (query);
}

qint64 MediaObject::getPipelinePos() const
{
    Q_ASSERT(m_pipeline);

    gint64 pos = 0;
    GstFormat format = GST_FORMAT_TIME;
    gst_element_query_position (GST_ELEMENT(m_pipeline), &format, &pos);
    return (pos / GST_MSECOND);
}

/*
 * Internal method to set a new total time for the media object
 */
void MediaObject::setTotalTime(qint64 newTime)
{

    if (newTime == m_totalTime)
        return;

    m_totalTime = newTime;

    emit totalTimeChanged(m_totalTime);
}

/*
 * !reimp
 */
void MediaObject::setSource(const MediaSource &source)
{
    if (!isValid())
        return;

    if (errorType() == FatalError)
        return;

    stop();

    // We have to reset the state completely here, otherwise
    // remnants of the old pipeline can result in strangenes
    // such as failing duration queries etc
    if (gst_element_set_state(m_pipeline, GST_STATE_NULL) != GST_STATE_CHANGE_SUCCESS) {
        setError(tr("Unable to flush pipeline"));
    }

    // Go into to loading state
    changeState(Phonon::LoadingState);
    m_loading = true;

     // Make sure we start out unconnected
    if (gst_element_get_parent(m_audioGraph))
        gst_bin_remove(GST_BIN(m_pipeline), m_audioGraph);
    if (gst_element_get_parent(m_videoGraph))
        gst_bin_remove(GST_BIN(m_pipeline), m_videoGraph);

    // Clear any existing errors
    m_aboutToFinishEmitted = false;
    m_error = NoError;
    m_errorString = QString();
    
    m_bufferPercent = 0;
    m_prefinishMarkReachedNotEmitted = true;
    m_aboutToFinishEmitted = false;
    m_source = source;
    m_hasAudio = false;
    m_hasVideo = false;
    setTotalTime(-1);

    // Clear exising meta tags
    // ### Must verify if it is required to notify that tags change on source change
    m_metaData.clear();
    emit metaDataChanged(m_metaData);

    switch (source.type()) {
    case MediaSource::Url: {
            QString urlString = source.url().toString();
            if (!createPipefromURL(urlString)) {
                setError(tr("Could not decode URL."));
                return;
            }
        }
        break;

    case MediaSource::LocalFile: {
            QString urlString = QUrl::fromLocalFile(source.fileName()).toString();
            if (!createPipefromURL(urlString)) {
                setError(tr("Could not open file."));
                return;
            }
        }
        break;

    case MediaSource::Invalid:
        setError(tr("Source type invalid"), Phonon::NormalError);
        break;

    case MediaSource::Disc: // CD tracks can be specified by setting the url in the following way uri=cdda:4
    case MediaSource::Stream:
    default:
        m_backend->logMessage("Source type not currently supported", Backend::Warning, this);
        setError(tr("Source type not supported"), Phonon::NormalError);
        return;
    }

    // Setting to state paused will trigger fetching meta data and duration
    MediaNodeEvent event(MediaNodeEvent::SourceChanged);
    notify(&event);

    // We need to link this node to ensure that fake sinks are connected
    // before loading, otherwise the stream will be blocked
    link();

    emit currentSourceChanged(m_source);

    // Do not fetch content untill we are in the play or paused states
    changeState(Phonon::StoppedState);
}

// Called when the user triggers pause() or play() on the media source
// while it is still in the loading state. This will block until the
// all stream info has been obtained.

// Currently both the Mac and Windows backends block on setSource, while 
// here we require calling pause() or play() to actually fetch data.
// 
void MediaObject::beginLoad()
{
    if (gst_element_set_state(m_pipeline, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE) {

        QTime timeoutTimer;
        timeoutTimer.start();

        while (m_loading && timeoutTimer.elapsed() < 1000) {
            QApplication::processEvents();
            usleep(20);
        }

        if (m_loading) {
            m_loading = false;
            setError(tr("Timed out while waiting for source"));
        }
    } else {
        setError(tr("Could not load source"));
    }
}

void MediaObject::getStreamInfo()
{
    updateSeekable();
    updateTotalTime();

    static bool hasVideo = false;
    if (hasVideo != m_hasVideo) {
        hasVideo = m_hasVideo;
        emit hasVideoChanged(m_hasVideo);
    }
}

void MediaObject::setPrefinishMark(qint32 newPrefinishMark)
{
    m_prefinishMark = newPrefinishMark;
    if (currentTime() < totalTime() - m_prefinishMark) // not about to finish
        m_prefinishMarkReachedNotEmitted = true;
}

void MediaObject::pause()
{
    m_backend->logMessage("pause()", Backend::Info, this);
    switch (state()) {
    case PlayingState:
    case BufferingState:
    case StoppedState:
        setState(Phonon::PausedState);
        break;
    case PausedState:
    case LoadingState:
    case ErrorState:
        break;
    }
}

void MediaObject::stop()
{
    if (state() == Phonon::PlayingState || state() == Phonon::BufferingState || state() == Phonon::PausedState) {
        setState(Phonon::StoppedState);
        m_prefinishMarkReachedNotEmitted = true;
    }
}

void MediaObject::seek(qint64 time)
{
    if (!isValid())
        return;

    if (isSeekable()) {
        switch (state()) {
        case Phonon::PlayingState:
        case Phonon::StoppedState:
        case Phonon::PausedState:
        case Phonon::BufferingState:
            if (gst_element_seek(m_pipeline, 1.0, GST_FORMAT_TIME,
                                 GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
                                 time * GST_MSECOND,GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
                emit tick(time); // Notifies the MediaObject about the position change.
                                 // (will keep seek sliders in sync)
            break;
        case Phonon::LoadingState:
        case Phonon::ErrorState:
            return;
        }

        if (currentTime() < totalTime() - m_prefinishMark)
            m_prefinishMarkReachedNotEmitted = true;
    }
}

void MediaObject::emitTick()
{
    qint64 currentTime = getPipelinePos();
    qint64 totalTime = m_totalTime;

    if (m_tickInterval > 0)
        emit tick(currentTime);

    if (currentTime >= totalTime - m_prefinishMark) {
        if (m_prefinishMarkReachedNotEmitted) {
            m_prefinishMarkReachedNotEmitted = false;
            emit prefinishMarkReached(totalTime - currentTime);
        }
    }

    //Prepare load of next source
    if (currentTime >= totalTime - 2000) {
        if (!m_aboutToFinishEmitted) {
            m_aboutToFinishEmitted = true; // track is about to finish
            emit aboutToFinish();
        }
    }
}


/*
 * Used to iterate through the gst_tag_list and extract values
 */
void foreach_tag_function(const GstTagList *list, const gchar *tag, gpointer user_data)
{
    TagMap *newData = static_cast<TagMap *>(user_data);
    QString value;
    GType type = gst_tag_get_type(tag);
    switch (type) {
    case G_TYPE_STRING: {
            char *str = 0;
            gst_tag_list_get_string(list, tag, &str);
            value = str;
            delete str;
        }
        break;

    case G_TYPE_BOOLEAN: {
            int bval;
            gst_tag_list_get_boolean(list, tag, &bval);
            value = QString::number(bval);
        }
        break;

    case G_TYPE_INT: {
            int ival;
            gst_tag_list_get_int(list, tag, &ival);
            value = QString::number(ival);
        }
        break;

    case G_TYPE_UINT: {
            unsigned int uival;
            gst_tag_list_get_uint(list, tag, &uival);
            value = QString::number(uival);
        }
        break;

    case G_TYPE_FLOAT: {
            float fval;
            gst_tag_list_get_float(list, tag, &fval);
            value = QString::number(fval);
        }
        break;

    default:
        //qDebug("Unsupported tag type: %s", g_type_name(type));
        break;
    }
    if (!value.isEmpty())
        newData->insert(QString(tag).toUpper(), value);
}

/**
 * Triggers playback after a song has completed in the current media queue
 */
void MediaObject::beginPlay()
{
    if (m_nextSource.type() == MediaSource::Invalid)
        return;

    m_nextSource = MediaSource();

    play();
}

/**
 * Handle GStreamer bus messages
 */
void MediaObject::handleBusMessage(const Message &message)
{

    if (!isValid())
        return;

    GstMessage *gstMessage = message.rawMessage();
    Q_ASSERT(m_pipeline);

    switch (GST_MESSAGE_TYPE (gstMessage)) {

    case GST_MESSAGE_EOS: {
            // Note that this event is the only reliable way of knowing that a track has finished, since
            // with some media streams we never actually reach the duration indicated by the stream duration
            //
            if (m_nextSource.type() != MediaSource::Invalid) {  // We only emit finish when the queue is actually empty

                // ### Due to timing issues, we currently wait for at least 100 ms before playback can start
                //     This is obviously not ideal...
                QTimer::singleShot (qMax(100, transitionTime()), this, SLOT(beginPlay()));
            } else {
                stop(); // Current track completed
                emit finished();
            }
            break;
        }

    case GST_MESSAGE_TAG: {
            GstTagList* tag_list = 0;
            gst_message_parse_tag(gstMessage, &tag_list);
            if (tag_list) {
                // Append any new meta tags to the existing tag list
                gst_tag_list_foreach (tag_list, &foreach_tag_function, &m_metaData);
                m_backend->logMessage("Meta tags found", Backend::Info, this);
                emit metaDataChanged(m_metaData);
                gst_tag_list_free(tag_list);
            }
        }
        break;

    case GST_MESSAGE_STATE_CHANGED : {

            if (gstMessage->src != GST_OBJECT(m_pipeline))
                return;

            GstState oldState;
            GstState newState;
            gst_message_parse_state_changed (gstMessage, &oldState, &newState, 0);

            switch (newState) {

            case GST_STATE_PLAYING :
                m_backend->logMessage("gstreamer: pipeline state set to playing", Backend::Info, this);
                m_tickTimer->start();
                break;

            case GST_STATE_NULL:
                m_backend->logMessage("gstreamer: pipeline state set to null", Backend::Info, this);
                m_tickTimer->stop();
                break;

            case GST_STATE_PAUSED :
                m_backend->logMessage("gstreamer: pipeline state set to paused", Backend::Info, this);
                m_tickTimer->stop();

                if (m_loading) {
                    getStreamInfo();
                    m_loading = false;
                    if (state() == Phonon::LoadingState)
                        setState(Phonon::PausedState);
                    else if (state() == Phonon::PlayingState)
                        setState(Phonon::PlayingState);
                }

                break;

            case GST_STATE_READY :
                m_backend->logMessage("gstreamer: pipeline state set to ready", Backend::Debug, this);
                m_tickTimer->stop();
                break;

            case GST_STATE_VOID_PENDING :
                m_backend->logMessage("gstreamer: pipeline state set to pending (void)", Backend::Debug, this);
                m_tickTimer->stop();
                break;
            }
            break;
        }

    case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *err;
            gst_message_parse_error (gstMessage, &err, &debug);
            g_free (debug);
            QString message;
            message.sprintf("Error: %s", err->message);
            m_backend->logMessage(message, Backend::Warning);
            setError(QString(err->message));
            g_error_free (err);
            break;
        }

    case GST_MESSAGE_WARNING: {
            gchar *debug;
            GError *err;
            gst_message_parse_warning(gstMessage, &err, &debug);
            g_free (debug);
            QString message;
            message.sprintf("Warning: %s", err->message);
            m_backend->logMessage(message, Backend::Warning);
            g_error_free (err);
            break;
        }

    case GST_MESSAGE_ELEMENT: {
            GstMessage *gstMessage = message.rawMessage();
            const GstStructure *gstStruct = gst_message_get_structure(gstMessage); //do not free this
            if (g_strrstr (gst_structure_get_name (gstStruct), "prepare-xwindow-id")) {
                MediaNodeEvent videoHandleEvent(MediaNodeEvent::VideoHandleRequest);
                notify(&videoHandleEvent);
            }
            break;
        }

    case GST_MESSAGE_DURATION: {
            m_backend->logMessage("GST_MESSAGE_DURATION", Backend::Debug, this);
            updateTotalTime();
            break;
        }

    case GST_MESSAGE_BUFFERING: {
            gint percent = 0;
            gst_structure_get_int (gstMessage->structure, "buffer-percent", &percent); //gst_message_parse_buffering was introduced in 0.10.11

            if (m_bufferPercent != percent) {
                emit bufferStatus(percent);
                m_backend->logMessage(QString("Stream buffering %0").arg(percent), Backend::Debug, this);
                m_bufferPercent = percent;
            }

            if (m_state != Phonon::BufferingState)
                emit stateChanged(m_state, Phonon::BufferingState);
            else if (percent == 100)
                emit stateChanged(Phonon::BufferingState, m_state);
            break;
        }
        //case GST_MESSAGE_INFO:
        //case GST_MESSAGE_STREAM_STATUS:
        //case GST_MESSAGE_CLOCK_PROVIDE:
        //case GST_MESSAGE_NEW_CLOCK:
        //case GST_MESSAGE_STEP_DONE:
        //case GST_MESSAGE_LATENCY: only from 0.10.12
        //case GST_MESSAGE_ASYNC_DONE: only from 0.10.13
    default: 
        int type = GST_MESSAGE_TYPE(gstMessage);
        QString message = QString("Bus: %0").arg(gst_message_type_get_name ((GstMessageType)type));
        m_backend->logMessage(message, Backend::Debug, this);
        break; 
    }
}

} // ns Gstreamer
} // ns Phonon
#include "moc_mediaobject.cpp"
