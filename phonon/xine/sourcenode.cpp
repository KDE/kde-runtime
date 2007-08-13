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

#include "sourcenode.h"
#include "sinknode.h"

namespace Phonon
{
namespace Xine
{

SourceNodeXT::~SourceNodeXT()
{
}

xine_post_out_t *SourceNodeXT::audioOutputPort() const
{
    return 0;
}

xine_post_out_t *SourceNodeXT::videoOutputPort() const
{
    return 0;
}

SourceNode::SourceNode(SourceNodeXT *_xt)
    : threadSafeObject(_xt)
{
    Q_ASSERT(_xt);
}

SourceNode::~SourceNode()
{
    if (!m_sinks.isEmpty()) {
        foreach (SinkNode *s, m_sinks) {
            s->unsetSource(this);
        }
    }
}

void SourceNode::addSink(SinkNode *s)
{
    Q_ASSERT(!m_sinks.contains(s));
    m_sinks << s;
}

void SourceNode::removeSink(SinkNode *s)
{
    Q_ASSERT(m_sinks.contains(s));
    m_sinks.remove(s);
}

QSet<SinkNode *> SourceNode::sinks() const
{
    return m_sinks;
}

SinkNode *SourceNode::sinkInterface()
{
    return 0;
}

} // namespace Xine
} // namespace Phonon
