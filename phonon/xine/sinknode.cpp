/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

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

#include "sinknode.h"
#include "sourcenode.h"
#include "events.h"

namespace Phonon
{
namespace Xine
{

SinkNodeXT::~SinkNodeXT()
{
    deleted = true;
}

AudioPort SinkNodeXT::audioPort() const
{
    return AudioPort();
}

xine_video_port_t *SinkNodeXT::videoPort() const
{
    return 0;
}

SinkNode::SinkNode(SinkNodeXT *_xt)
    : m_threadSafeObject(_xt), m_source(0)
{
    Q_ASSERT(_xt);
}

SinkNode::~SinkNode()
{
    if (m_source) {
        m_source->removeSink(this);
    }
}

void SinkNode::setSource(SourceNode *s)
{
    Q_ASSERT(m_source == 0);
    m_source = s;
}

void SinkNode::unsetSource(SourceNode *s)
{
    Q_ASSERT(m_source == s);
    m_source = 0;
}

SourceNode *SinkNode::source() const
{
    return m_source;
}

SourceNode *SinkNode::sourceInterface()
{
    return 0;
}

void SinkNode::upstreamEvent(Event *e)
{
    Q_ASSERT(e);
    if (m_source) {
        m_source->upstreamEvent(e);
    } else {
        if (!--e->ref) {
            delete e;
        }
    }
}

void SinkNode::downstreamEvent(Event *e)
{
    Q_ASSERT(e);
    SourceNode *iface = sourceInterface();
    if (iface) {
        iface->downstreamEvent(e);
    } else {
        if (!--e->ref) {
            delete e;
        }
    }
}

} // namespace Xine
} // namespace Phonon
