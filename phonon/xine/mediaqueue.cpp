/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

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

#include "mediaqueue.h"
#include <kdebug.h>

namespace Phonon
{
namespace Xine
{

/*
 * ok, here is one thing that has been bugging me for a long time:
 * gapless playback. i guess i have finally thought of a nice way of
 * implementing it with minimum effort from ui.
 *
 * the magic involves two new stream parameters:
 *
 * XINE_PARAM_EARLY_FINISHED_EVENT
 * XINE_PARAM_GAPLESS_SWITCH
 *
 * if UI supports/wants gapless playback it should then set
 * XINE_PARAM_EARLY_FINISHED_EVENT. setting it causes libxine to deliver
 * the XINE_EVENT_UI_PLAYBACK_FINISHED as soon as it can, that is, it
 * will disable internal code that waits for the output audio and video
 * fifos to empty. set it once and forget.
 *
 * when UI receives XINE_EVENT_UI_PLAYBACK_FINISHED, it should set
 * XINE_PARAM_GAPLESS_SWITCH and then perform usual xine_open(),
 * xine_play() sequence. the gapless parameter will cause libxine to
 * disable a couple of code so it won't stop current playback, it won't
 * close audio driver and the new stream should play seamless.
 *
 * but here is the catch: XINE_PARAM_GAPLESS_SWITCH must _only_ be set
 * just before the desired gapless switch. it will be reset automatically
 * as soon as the xine_play() returns. setting it during playback will
 * cause bad seek behaviour.
 *
 * take care to reset the gapless switch if xine_open() fails.
 *
 * btw, frontends should check for version >= 1.1.1 before using this feature.
 */

MediaQueue::MediaQueue( QObject *parent )
	: MediaObject( parent )
	, m_doCrossfade( false )
{
    stream().useGaplessPlayback(true);
    connect(&stream(), SIGNAL(needNextUrl()), SLOT(streamNeedsUrl()));
}

MediaQueue::~MediaQueue()
{
}

void MediaQueue::streamNeedsUrl()
{
    if (m_nextUrl.isEmpty()) {
        emit needNextUrl();
    }
    // if there's no "answer": clean up with an empty URL (m_nextUrl is cleared)
    stream().gaplessSwitchTo(m_nextUrl);
    m_url = m_nextUrl;
    m_nextUrl.clear();
}

void MediaQueue::setNextUrl( const KUrl& url )
{
    m_nextUrl = url;
}

void MediaQueue::setDoCrossfade( bool xfade )
{
    if (xfade) {
		kWarning( 610 ) << "crossfades with Xine are not implemented yet";
	}
	//m_doCrossfade = xfade;
}

void MediaQueue::setTimeBetweenMedia( qint32 time )
{
	m_timeBetweenMedia = time;
}

} // namespace Xine
} // namespace Phonon

#include "mediaqueue.moc"
// vim: sw=4 ts=4
