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

#ifndef NULLSINK_H
#define NULLSINK_H

#include "sinknode.h"
#include <QtCore/QObject>

namespace Phonon
{
namespace Xine
{

class NullSinkXT : public SinkNodeXT
{
    public:
        NullSinkXT() : SinkNodeXT("NullSink") {}
        virtual void rewireTo(SourceNodeXT *);
        virtual AudioPort audioPort() const;
        virtual xine_video_port_t *videoPort() const;
};

class NullSinkPrivate;
class NullSink : public QObject, public SinkNode
{
    friend class NullSinkPrivate;
    Q_OBJECT
    private:
        NullSink(QObject *parent = 0) : QObject(parent), SinkNode(new NullSinkXT) {}
    public:
        static NullSink *instance();
        MediaStreamTypes inputMediaStreamTypes() const { return Phonon::Xine::Audio | Phonon::Xine::Video; }
};
} // namespace Xine
} // namespace Phonon
#endif // NULLSINK_H
