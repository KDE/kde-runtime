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

#ifndef SINKNODE_H
#define SINKNODE_H

#include <Phonon/Global>
#include <xine.h>
#include "audioport.h"
#include "xineengine.h"

namespace Phonon
{
namespace Xine
{
class SourceNode;
class SourceNodeXT;
class Event;

class SinkNodeXT : virtual public QSharedData
{
    public:
        SinkNodeXT() : deleted(false) {}
        virtual ~SinkNodeXT();
        virtual void rewireTo(SourceNodeXT *) = 0;
        virtual AudioPort audioPort() const;
        virtual xine_video_port_t *videoPort() const;
        void assert() { Q_ASSERT(!deleted); }
    private:
        bool deleted;
};

class SinkNode
{
    friend class WireCall;
    friend class XineStream;
    public:
        SinkNode(SinkNodeXT *_xt);
        virtual ~SinkNode();
        virtual MediaStreamTypes inputMediaStreamTypes() const = 0;
        void setSource(SourceNode *s);
        void unsetSource(SourceNode *s);
        SourceNode *source() const;
        virtual SourceNode *sourceInterface();

        virtual void upstreamEvent(Event *);
        virtual void downstreamEvent(Event *);

        QExplicitlySharedDataPointer<SinkNodeXT> threadSafeObject() const { return m_threadSafeObject; }

    private:
        QExplicitlySharedDataPointer<SinkNodeXT> m_threadSafeObject;
        SourceNode *m_source;
};

} // namespace Xine
} // namespace Phonon

Q_DECLARE_INTERFACE(Phonon::Xine::SinkNode, "XineSinkNode.phonon.kde.org")

#endif // SINKNODE_H
