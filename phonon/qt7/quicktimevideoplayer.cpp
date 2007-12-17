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

#include "quicktimevideoplayer.h"
#include "backendheader.h"
#include "mediaobject.h"
#include "videowidget.h"
#include "audiodevice.h"
#include <private/qcore_mac_p.h>

#include <QGLContext>
#include <QtOpenGL/private/qgl_p.h>

namespace Phonon
{
namespace QT7
{

QuickTimeVideoPlayer::QuickTimeVideoPlayer() : QObject(0)
{
    m_state = NoMedia;
    m_mediaSource = MediaSource();
    m_visualContext = 0;
    m_movieRef = 0;
    m_mediaUpdateTimer = 0;
    m_playbackRate = 1.0f;
    m_playbackRateSat = false;
    createVisualContext();
}

QuickTimeVideoPlayer::~QuickTimeVideoPlayer()
{
    unsetVideo();
}

void QuickTimeVideoPlayer::createVisualContext()
{
    AGLContext aglContext = VideoRenderWidget::sharedOpenGLContext();
    AGLPixelFormat aglPixelFormat = VideoRenderWidget::sharedOpenGLPixelFormat();

    CGLContextObj cglContext = 0;
    BACKEND_ASSERT2(aglGetCGLContext(aglContext, (void **)&cglContext), "Could not get cgl context from agl context (OpenGL)", FATAL_ERROR)

    CGLPixelFormatObj cglPixelFormat = 0;
    BACKEND_ASSERT2(aglGetCGLPixelFormat(aglPixelFormat, (void **)&cglPixelFormat), "Could not get cgl pixel format from agl pixel format (OpenGL)", FATAL_ERROR)

    CFTypeRef keys[] = { kQTVisualContextWorkingColorSpaceKey };
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CFDictionaryRef textureContextAttributes = CFDictionaryCreate(kCFAllocatorDefault,
                                                                  (const void **)keys,
                                                                  (const void **)&colorSpace, 1,
                                                                  &kCFTypeDictionaryKeyCallBacks,
                                                                  &kCFTypeDictionaryValueCallBacks);
	OSStatus err = QTOpenGLTextureContextCreate(kCFAllocatorDefault, cglContext,
        cglPixelFormat, textureContextAttributes, &m_visualContext);
    BACKEND_ASSERT2(err == noErr, "Could not create visual context (OpenGL)", FATAL_ERROR)

    CFRelease(textureContextAttributes);
}

bool QuickTimeVideoPlayer::videoFrameChanged(const CVTimeStamp &timeStamp) const
{
    if (!m_movieRef || m_state != Playing)
        return false;

    QTVisualContextTask(m_visualContext);
    if (QTVisualContextIsNewImageAvailable(m_visualContext, &timeStamp))
        return true;
    return false;
}

void QuickTimeVideoPlayer::doRegularTasks()
{
    if (!m_movieRef)
        return;

    if (m_mediaUpdateTimer == 0){
        MoviesTask(m_movieRef, 1);
        long ms = 0;
        QTGetTimeUntilNextTask(&ms, 1000);
        m_mediaUpdateTimer = startTimer(ms);
    }
}

CVOpenGLTextureRef QuickTimeVideoPlayer::createCvTexture(const CVTimeStamp &timeStamp)
{
    CVOpenGLTextureRef texture = 0;
    OSStatus err = QTVisualContextCopyImageForTime(m_visualContext, 0, &timeStamp, &texture);
    BACKEND_ASSERT3(err == noErr, "Could not copy image for time in QuickTime player", FATAL_ERROR, 0)
    return texture;
}

void QuickTimeVideoPlayer::setVolume(float volume)
{
    if (!m_movieRef)
        return;
        
    if (volume == 0)
        SetMovieAudioMute(m_movieRef, true, 0);
    else {
        SetMovieAudioMute(m_movieRef, false, 0);
        SetMovieAudioGain(m_movieRef, volume, 0);
    }
}

bool QuickTimeVideoPlayer::setAudioDevice(int id)
{
    if (!m_movieRef)
        return false;

    // The following code is obviously only meant to be working on Windows.
    // So, sadly, it will change nothing at the moment: 
    CFStringRef idString = QCFString::toCFStringRef(AudioDevice::deviceUID(id));        
    QTAudioContextRef context;
    QTAudioContextCreateForAudioDevice(kCFAllocatorDefault, idString, 0, &context);
    OSStatus err = SetMovieAudioContext(m_movieRef, context);
    CFRelease(context);
    if (err != noErr)
        return false;
    return true;
}

void QuickTimeVideoPlayer::setColors(qreal brightness, qreal contrast, qreal hue, qreal saturation)
{
    if (!m_movieRef)
        return;

    // 0 is default value for the colors
    // in phonon, so adjust scale:
    contrast += 1;
    saturation += 1;

    Float32 value;
    value = brightness;
    SetMovieVisualBrightness(m_movieRef, value, 0);
    value = contrast;
    SetMovieVisualContrast(m_movieRef, value, 0);
    value = hue;
    SetMovieVisualHue(m_movieRef, value, 0);
    value = saturation;
    SetMovieVisualSaturation(m_movieRef, value, 0);
}

QRect QuickTimeVideoPlayer::videoRect() const
{
    if (!m_movieRef)
        return QRect();

    Rect rect;
    GetMovieBox(m_movieRef, &rect);
    return QRect(rect.left, rect.top, rect.left + rect.right, rect.top + rect.bottom);
}

void QuickTimeVideoPlayer::unsetVideo()
{
    if (!m_movieRef)
        return;

    if (m_mediaUpdateTimer){
        killTimer(m_mediaUpdateTimer);
        m_mediaUpdateTimer = 0;
    }

    StopMovie(m_movieRef);
    SetMovieVisualContext(m_movieRef, 0);
    DisposeMovie(m_movieRef);
	m_movieRef = 0;
    m_state = NoMedia;
    m_mediaSource = MediaSource();
}

QuickTimeVideoPlayer::State QuickTimeVideoPlayer::state() const
{
    return m_state;
}

void QuickTimeVideoPlayer::setMediaSource(const MediaSource &mediaSource)
{
    unsetVideo();
    m_mediaSource = mediaSource;
    if (mediaSource.type() == MediaSource::Invalid){
        m_state = NoMedia;
        return;
    } else
        m_state = Paused;

    switch (mediaSource.type()){
    case MediaSource::LocalFile:
        BACKEND_ASSERT2(openMovieFromFile(mediaSource), "Could not create new movie from file.", NORMAL_ERROR)
        break;
    case MediaSource::Url:
        BACKEND_ASSERT2(openMovieFromUrl(mediaSource), "Could not create new movie from URL.", NORMAL_ERROR)
        break;
    case MediaSource::Disc:
        CASE_UNSUPPORTED("Backend does not yet support loading media from disc", NORMAL_ERROR)
        break;
    case MediaSource::Stream:
        CASE_UNSUPPORTED("Backend does not yet support loading media from stream", NORMAL_ERROR)
        break;
    case MediaSource::Invalid:
        break;
    }

    SetTimeBaseFlags(GetMovieTimeBase(m_movieRef), 0/*loopTimeBase*/);
    SetMoviePlayHints(m_movieRef, hintsLoop | hintsHighQuality, hintsLoop | hintsHighQuality);
    SetMovieAudioMute(m_movieRef, true, 0);

    if (!m_playbackRateSat)
        m_playbackRate = prefferedPlaybackRate();
    preRollMovie();
}

bool QuickTimeVideoPlayer::openMovieFromFile(const MediaSource &mediaSource)
{
	Boolean active = true;
    DataReferenceRecord dataRef = {0, 0};
    QFileInfo fileInfo(mediaSource.fileName());
    QCFString filename = fileInfo.isSymLink() ? fileInfo.symLinkTarget() : fileInfo.canonicalFilePath();

    QTNewDataReferenceFromFullPathCFString(filename, kQTPOSIXPathStyle, 0, &dataRef.dataRef, &dataRef.dataRefType);
	QTNewMoviePropertyElement props[] = {
		{kQTPropertyClass_DataLocation,     kQTDataLocationPropertyID_DataReference, sizeof(dataRef),       &dataRef,       0},
		{kQTPropertyClass_NewMovieProperty, kQTNewMoviePropertyID_Active,            sizeof(active),        &active,        0},
		{kQTPropertyClass_Context,          kQTContextPropertyID_VisualContext,      sizeof(m_visualContext), &m_visualContext, 0},
	};

    int propCount = sizeof(props) / sizeof(props[0]);
    OSStatus err = NewMovieFromProperties(propCount, props, 0, 0, &m_movieRef);
    DisposeHandle(dataRef.dataRef);
    
    if (err != noErr)
        m_movieRef = 0;
    return (err == noErr);
}

bool QuickTimeVideoPlayer::openMovieFromUrl(const MediaSource &mediaSource)
{
	Boolean active = true;
    QCFString urlString = QCFString(mediaSource.url().toString());
//    QCFString urlString("http://www.cbc.ca/livemedia/cbcr1-calgary.asx");
 
    DataReferenceRecord dataRef = {0, 0};    
    CFURLRef url = CFURLCreateWithString(0, urlString, 0);
    QTNewDataReferenceFromCFURL(url, 0, &dataRef.dataRef, &dataRef.dataRefType);

	QTNewMoviePropertyElement props[] = {
		{kQTPropertyClass_DataLocation,     kQTDataLocationPropertyID_DataReference, sizeof(dataRef),       &dataRef,       0},
		{kQTPropertyClass_NewMovieProperty, kQTNewMoviePropertyID_Active,            sizeof(active),        &active,        0},
		{kQTPropertyClass_MovieInstantiation, kQTMovieInstantiationPropertyID_DontResolveDataRefs, sizeof(active), &active, 0},
		{kQTPropertyClass_MovieInstantiation, kQTMovieInstantiationPropertyID_AsyncOK, sizeof(active), &active, 0},
		{kQTPropertyClass_Context,          kQTContextPropertyID_VisualContext,      sizeof(m_visualContext), &m_visualContext, 0},
	};

    int propCount = sizeof(props) / sizeof(props[0]);
    OSStatus err = NewMovieFromProperties(propCount, props, 0, 0, &m_movieRef);
    DisposeHandle(dataRef.dataRef);
    
    if (err != noErr)
        m_movieRef = 0;
    return (err == noErr);
}

MediaSource QuickTimeVideoPlayer::mediaSource() const
{
    return m_mediaSource;
}

Movie QuickTimeVideoPlayer::movieRef() const
{
    return m_movieRef;
}

void QuickTimeVideoPlayer::setPlaybackRate(float rate)
{
    m_playbackRateSat = true;
    m_playbackRate = rate;
    if (m_movieRef)
        SetMovieRate(m_movieRef, FloatToFixed(m_playbackRate));
}

float QuickTimeVideoPlayer::playbackRate() const
{
    return m_playbackRate;
}

qint64 QuickTimeVideoPlayer::currentTime() const
{
    if (!m_movieRef)
        return 0;

    TimeRecord timeRec;
    GetMovieTime (m_movieRef, &timeRec);
    return static_cast<qint64>(float(timeRec.value.lo) / float(timeRec.scale) * 1000.0f);
}

qint64 QuickTimeVideoPlayer::duration() const
{
    if (!m_movieRef)
        return 0;

    return static_cast<quint64>(float(GetMovieDuration(m_movieRef)) / float(GetMovieTimeScale(m_movieRef)) * 1000.0f);
}

void QuickTimeVideoPlayer::play()
{
    if (!m_movieRef)
        return;

    m_state = Playing;
    SetMovieRate(m_movieRef, FloatToFixed(m_playbackRate));
    if (!m_mediaUpdateTimer)
        m_mediaUpdateTimer = startTimer(0);
}

bool QuickTimeVideoPlayer::canPlayMedia() const
{
    return m_movieRef != 0;
}

bool QuickTimeVideoPlayer::isPlaying()
{
    return m_movieRef && m_state == Playing;
}

void QuickTimeVideoPlayer::pause()
{
    if (!m_movieRef)
        return;

    m_state = Paused;
    SetMovieRate(m_movieRef, FloatToFixed(0.0f));
}

void QuickTimeVideoPlayer::seek(qint64 milliseconds)
{
    if (!m_movieRef)
        return;

    if (milliseconds > duration())
        milliseconds = duration();

    TimeRecord timeRec;
    GetMovieTime(m_movieRef, &timeRec);
	timeRec.value.lo = 0;
    SetMovieTime(m_movieRef, &timeRec);
	timeRec.value.lo = (milliseconds / 1000.0f) * timeRec.scale;
    preRollMovie(timeRec.value.lo);
    SetMovieTime(m_movieRef, &timeRec);
}

float QuickTimeVideoPlayer::prefferedPlaybackRate() const
{
    if (!m_movieRef)
        return -1;

    return FixedToFloat(GetMoviePreferredRate(m_movieRef));
}

void MoviePrePrerollCompleteCallBack(Movie /*theMovie*/, OSErr /*thePrerollErr*/, void */*mediaSourcePlayer*/)
{
    /*
    QuickTimeVideoPlayer *player = static_cast<QuickTimeVideoPlayer *>(mediaSourcePlayer);
    player->moviePreRolled(1.0f);
    */
}

void QuickTimeVideoPlayer::moviePreRolled(float /*percentage*/)
{
    // Not in use for the moment!
    // IMPLEMENTED << percentage;
}

bool QuickTimeVideoPlayer::preRollMovie(qint64 startTime)
{
    if (!m_movieRef)
        return false;

    if (PrePrerollMovie(m_movieRef, startTime, FloatToFixed(m_playbackRate),
        0/*MoviePrePrerollCompleteCallBack*/, this) != noErr) // No callback means wait (synch)
        return false;

    if (PrerollMovie(m_movieRef, startTime, FloatToFixed(m_playbackRate)) != noErr)
        return false;

    return true;
}

bool QuickTimeVideoPlayer::hasAudio() const
{
    if (!m_movieRef)
        return false;

    Track track = GetMovieIndTrackType(
        m_movieRef, 1, AudioMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly);
    return (track != 0);
}

bool QuickTimeVideoPlayer::hasVideo() const
{
    if (!m_movieRef)
        return false;

    Track track = GetMovieIndTrackType(
        m_movieRef, 1, VisualMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly);
    return (track != 0);
}

bool QuickTimeVideoPlayer::event(QEvent *event)
{
    bool result = QObject::event(event);

    switch (event->type()){
    case QEvent::Timer:{
        QTimerEvent *timerEvent = static_cast<QTimerEvent *>(event);
        if (timerEvent->timerId() == m_mediaUpdateTimer){
            killTimer(m_mediaUpdateTimer);
            m_mediaUpdateTimer = 0;
        }
        break;}
    default:
        break;
    }
    return result;
}

}}


