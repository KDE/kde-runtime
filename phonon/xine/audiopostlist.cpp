/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "audiopostlist.h"
#include "audioport.h"
#include "effect.h"
#include "xinestream.h"
#include "xineengine.h"
#include "xinethread.h"

#include <QtCore/QList>
#include <QtCore/QSharedData>
#include <QtCore/QThread>

#include <xine.h>
#include <kdebug.h>

namespace Phonon
{
namespace Xine
{

static inline xine_post_out_t *outputFor(xine_post_t *post)
{
    xine_post_out_t *x = xine_post_output(post, "audio out");
    Q_ASSERT(x);
    return x;
}

static inline xine_post_in_t *inputFor(xine_post_t *post)
{
    xine_post_in_t *x = xine_post_input(post, "audio in");
    Q_ASSERT(x);
    return x;
}

class ListData
{
    public:
        inline ListData(Effect *e, xine_post_t *x)
            : m_effect(e),
            m_post(x),
            m_next(0)
        {
        }

        // compare list items only by looking at m_effect
        bool operator==(const ListData &rhs) { return m_effect == rhs.m_effect; }

        Effect *effect() const { return m_effect; }
        xine_post_t *post() const { return m_post; }

        // called from the xine thread
        void setOutput(xine_post_in_t *audioSink, const AudioPort &audioPort)
        {
            if (m_next == audioSink) {
                return;
            }
            m_next = audioSink;
            if (!m_post) {
                m_post = m_effect->newInstance(audioPort);
                Q_ASSERT(m_post);
            } else {
                kDebug(610) << "xine_post_wire(" << outputFor(m_post) << ", " << audioSink << ")";
                int err = xine_post_wire(outputFor(m_post), audioSink);
                Q_ASSERT(err == 1);
            }
        }
        // called from the xine thread
        void setOutput(const AudioPort &audioPort)
        {
            // XXX hack
            if (m_next == reinterpret_cast<xine_post_in_t *>(-1)) {
                return;
            }
            m_next = reinterpret_cast<xine_post_in_t *>(-1);
            if (!m_post) {
                m_post = m_effect->newInstance(audioPort);
                Q_ASSERT(m_post);
            } else {
                kDebug(610) << "xine_post_wire_audio_port(" << outputFor(m_post) << ", " << audioPort << ")";
                int err = xine_post_wire_audio_port(outputFor(m_post), audioPort);
                Q_ASSERT(err == 1);
            }
        }

    private:
        Effect *m_effect;
        xine_post_t *m_post;
        xine_post_in_t *m_next;
};

class AudioPostListData : public QSharedData
{
    public:
        QList<ListData> effects;
        AudioPort output;
        AudioPort newOutput;
        XineStream *stream;
};

// called from the xine thread: safe to call xine functions that call back to the ByteStream input
// plugin
void AudioPostList::wireStream()
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    Q_ASSERT(d->stream);
    wireStream(d->stream->audioSource());
}

void AudioPostList::wireStream(xine_post_out_t *audioSource)
{
    Q_ASSERT(QThread::currentThread() == XineEngine::thread());
    if (d->newOutput.isValid()) {
        int err;
        if (!d->effects.isEmpty()) {
            xine_post_t *next = 0;

            const QList<ListData>::Iterator begin = d->effects.begin();
            QList<ListData>::Iterator it = d->effects.end();
            --it;
            {
                ListData &effect = *it;
                effect.setOutput(d->newOutput);
                next = effect.post();
            }
            while (it != begin) {
                --it;
                ListData &effect = *it;
                effect.setOutput(inputFor(next), d->newOutput);
                next = effect.post();
            }
            kDebug(610) << "xine_post_wire(" << audioSource << ", " << inputFor(next) << ")";
            err = xine_post_wire(audioSource, inputFor(next));
        } else {
            kDebug(610) << "xine_post_wire_audio_port(" << audioSource << ", " << d->newOutput << ")";
            err = xine_post_wire_audio_port(audioSource, d->newOutput);
        }
        Q_ASSERT(err == 1);
        d->output.waitALittleWithDying(); // xine still accesses the port after a rewire :(
        d->output = d->newOutput;
    } else {
        kDebug(610) << "no valid audio output given, no audio";
        //xine_set_param(stream, XINE_PARAM_IGNORE_AUDIO, 1);
    }
}

void AudioPostList::setAudioPort(const AudioPort &port)
{
    d->newOutput = port;
    if (d->stream) {
        XineThread::needRewire(this);
    }
}

const AudioPort &AudioPostList::audioPort() const
{
    return d->output;
}

bool AudioPostList::contains(Effect *effect) const
{
    return d->effects.contains(ListData(effect, 0));
}

int AudioPostList::indexOf(Effect *effect) const
{
    return d->effects.indexOf(ListData(effect, 0));
}

void AudioPostList::insert(int index, Effect *effect)
{
    d->effects.insert(index, ListData(effect, 0));
    if (d->stream) {
        XineThread::needRewire(this);
    }
}

void AudioPostList::append(Effect *effect)
{
    d->effects.append(ListData(effect, 0));
    if (d->stream) {
        XineThread::needRewire(this);
    }
}

int AudioPostList::removeAll(Effect *effect)
{
    int removed = d->effects.removeAll(ListData(effect, 0));
    if (d->stream) {
        XineThread::needRewire(this);
    }
    return removed;
}

AudioPostList::AudioPostList()
    : d(new AudioPostListData)
{
}

AudioPostList::AudioPostList(const AudioPostList &rhs)
    :d (rhs.d)
{
}

AudioPostList &AudioPostList::operator=(const AudioPostList &rhs)
{
    d = rhs.d;
    return *this;
}

AudioPostList::~AudioPostList()
{
}

void AudioPostList::setXineStream(XineStream *xs)
{
    Q_ASSERT(d->stream == 0);
    d->stream = xs;
}

void AudioPostList::unsetXineStream(XineStream *xs)
{
    Q_ASSERT(d->stream == xs);
    d->stream = 0;
}

} // namespace Xine
} // namespace Phonon

