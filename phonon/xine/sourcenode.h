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

#ifndef SOURCENODE_H
#define SOURCENODE_H

#include <Phonon/Global>
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QSet>
#include <xine.h>
#include "xineengine.h"

namespace Phonon
{
namespace Xine
{
class SinkNode;
class Event;

class SourceNodeXT : virtual public QSharedData
{
    public:
        SourceNodeXT() : deleted(false) {}
        virtual ~SourceNodeXT();
        virtual xine_post_out_t *audioOutputPort() const;
        virtual xine_post_out_t *videoOutputPort() const;
        void assert() { Q_ASSERT(!deleted); }

    private:
        bool deleted;
};

class SourceNode
{
    friend class WireCall;
    public:
        SourceNode(SourceNodeXT *_xt);
        virtual ~SourceNode();
        virtual MediaStreamTypes outputMediaStreamTypes() const = 0;
        void addSink(SinkNode *s);
        void removeSink(SinkNode *s);
        QSet<SinkNode *> sinks() const;
        virtual SinkNode *sinkInterface();

        virtual void upstreamEvent(Event *);
        virtual void downstreamEvent(Event *);

        QExplicitlySharedDataPointer<SourceNodeXT> threadSafeObject() const { return m_threadSafeObject; }

    protected:
        QExplicitlySharedDataPointer<SourceNodeXT> m_threadSafeObject;
    private:
        QSet<SinkNode *> m_sinks;
};
} // namespace Xine
} // namespace Phonon

Q_DECLARE_INTERFACE(Phonon::Xine::SourceNode, "XineSourceNode.phonon.kde.org")

#endif // SOURCENODE_H
