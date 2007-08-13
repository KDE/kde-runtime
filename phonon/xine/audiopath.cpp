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

#include "audiopath.h"
#include "audioeffect.h"
#include "abstractaudiooutput.h"
#include "audiooutput.h"
#include <QVector>
#include "mediaobject.h"

namespace Phonon
{
namespace Xine
{

AudioPath::AudioPath( QObject* parent )
	: QObject( parent )
	, m_output( 0 )
{
}

AudioPath::~AudioPath()
{
    if (m_output) {
        m_output->removePath(this);
    }
    foreach (AbstractAudioOutput *aao, m_outputs) {
        aao->removePath(this);
    }
}

bool AudioPath::hasOutput() const
{
    return m_output;
}

/*AudioPort AudioPath::audioPort(XineStream* forStream) const
{
    if (m_output) {
        return m_output->audioPort(forStream);
    }
    return AudioPort();
}*/

void AudioPath::audioPortChanged(const AudioPort &port)
{
    m_effects.setAudioPort(port);
}

bool AudioPath::addOutput( QObject* audioOutput )
{
	AudioOutput *ao = qobject_cast<AudioOutput*>( audioOutput );
	if( ao )
	{
		if( m_output )
			return false;
		m_output = ao;
		m_output->addPath( this );
        m_effects.setAudioPort(ao->audioPort());
        connect(m_output, SIGNAL(audioPortChanged(AudioPort)), SLOT(audioPortChanged(AudioPort)));
		return true;
	}

	AbstractAudioOutput *aao = qobject_cast<AbstractAudioOutput*>( audioOutput );
	Q_ASSERT( aao );
	Q_ASSERT( !m_outputs.contains( aao ) );
	m_outputs.append( aao );
	aao->addPath( this );
	return true;
}

bool AudioPath::removeOutput( QObject* audioOutput )
{
	AudioOutput *ao = qobject_cast<AudioOutput*>( audioOutput );
	if( ao && m_output == ao )
	{
        disconnect(m_output, SIGNAL(audioPortChanged(AudioPort)), this, SLOT(audioPortChanged(AudioPort)));
		m_output->removePath( this );
		m_output = 0;
        m_effects.setAudioPort(AudioPort());
		return true;
	}
	AbstractAudioOutput* aao = qobject_cast<AbstractAudioOutput*>( audioOutput );
	Q_ASSERT( aao );
    const int removed = m_outputs.removeAll(aao);
    Q_ASSERT(removed == 1);
	aao->removePath( this );
	return true;
}

bool AudioPath::insertEffect( QObject* newEffect, QObject* insertBefore )
{
	Q_ASSERT( newEffect );
	AudioEffect* ae = qobject_cast<AudioEffect*>( newEffect );
	Q_ASSERT( ae );
    if (!ae->isValid()) {
        return false;
    }
	AudioEffect* before = 0;
	if( insertBefore )
	{
		before = qobject_cast<AudioEffect*>( insertBefore );
		Q_ASSERT( before );
		if( !m_effects.contains( before ) )
			return false;
		m_effects.insert( m_effects.indexOf( before ), ae );
	}
	else
		m_effects.append( ae );

	return true;
}

bool AudioPath::removeEffect( QObject* effect )
{
	Q_ASSERT( effect );
	AudioEffect* ae = qobject_cast<AudioEffect*>( effect );
	Q_ASSERT( ae );
	if( m_effects.removeAll( ae ) > 0 )
		return true;
	return false;
}

void AudioPath::addMediaObject(MediaObject *mp)
{
    m_mediaObjects.append(mp);
    if (m_output) {
        m_output->updateVolume( mp );
    }
}

void AudioPath::removeMediaObject(MediaObject *mp)
{
    m_mediaObjects.removeAll(mp);
}

void AudioPath::updateVolume(MediaObject *mp) const
{
	if( m_output )
		m_output->updateVolume( mp );
}

}}

#include "audiopath.moc"
// vim: sw=4 ts=4
