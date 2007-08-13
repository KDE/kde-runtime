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

#ifndef PHONON_XINE_EVENTS_H
#define PHONON_XINE_EVENTS_H

#include "audiopostlist.h"
#include "wirecall.h"
#include "xinestream.h"

#include <QtCore/QEvent>
#include <QtCore/QPair>
#include <QtCore/QList>

namespace Phonon
{
namespace Xine
{

namespace Events
{
    enum {
        GetStreamInfo = 2001,
        UpdateVolume = 2002,
        RewireVideoToNull = 2003,
        PlayCommand = 2004,
        PauseCommand = 2005,
        StopCommand = 2006,
        SeekCommand = 2007,
        MrlChanged = 2008,
        GaplessPlaybackChanged = 2009,
        GaplessSwitch = 2010,
        UpdateTime = 2011,
        SetTickInterval = 2012,
        SetPrefinishMark = 2013,
        SetParam = 2014,
        EventSend = 2015,
        AudioRewire = 2016,
        //ChangeAudioPostList = 2017,
        QuitLoop = 2018,
        PauseForBuffering = 2019,  // XXX numerically used in bytestream.cpp
        UnpauseForBuffering = 2020, // XXX numerically used in bytestream.cpp
        Error = 2021,

        //NeedRewire = 4800,
        NewStream = 4801,

        NewMetaData = 5400,
        MediaFinished = 5401,
        Progress = 5402,
        NavButtonIn = 5403,
        NavButtonOut = 5404,
        AudioDeviceFailed = 5405,
        FrameFormatChange = 5406,
        UiChannelsChanged = 5407,
        Reference = 5408,

        Rewire = 5409
    };
} // namespace Events

class SourceNode;
class SinkNode;

class RewireEvent : public QEvent
{
    public:
        RewireEvent(QList<WireCall> _wireCalls)
            : QEvent(static_cast<QEvent::Type>(Events::Rewire)), wireCalls(_wireCalls) {}
        const QList<WireCall> wireCalls;
};

/*
class NeedRewireEvent : public QEvent
{
    public:
        NeedRewireEvent(AudioPostList *a) : QEvent(static_cast<QEvent::Type>(Events::NeedRewire)), audioPostList(a) {}
        AudioPostList *const audioPostList;
};
*/

class XineReferenceEvent : public QEvent
{
    public :
        XineReferenceEvent(bool _alternative, const QByteArray &_mrl)
            : QEvent(static_cast<QEvent::Type>(Events::Reference)),
            alternative(_alternative), mrl(_mrl) {}

        const bool alternative;
        const QByteArray mrl;
};

class XineProgressEvent : public QEvent
{
    public:
        XineProgressEvent(const QString &d, int p)
            : QEvent(static_cast<QEvent::Type>(Xine::Events::Progress)),
            description(d), percent(p) {}

        const QString description;
        const int percent;
};

class XineFrameFormatChangeEvent : public QEvent
{
    public:
        XineFrameFormatChangeEvent(int w, int h, int a, bool ps)
            : QEvent(static_cast<QEvent::Type>(Events::FrameFormatChange)),
            size(w, h), aspect(a), panScan(ps) {}

        const QSize size;
        const int aspect;
        const bool panScan;
};

class ErrorEvent : public QEvent
{
    public:
        ErrorEvent(Phonon::ErrorType t, const QString &r)
            : QEvent(static_cast<QEvent::Type>(Events::Error)), type(t), reason(r) {}
        const Phonon::ErrorType type;
        const QString reason;
};

/*
class ChangeAudioPostListEvent : public QEvent
{
    public:
        enum AddOrRemove { Add, Remove };
        ChangeAudioPostListEvent(const AudioPostList &x, AddOrRemove y)
            : QEvent(static_cast<QEvent::Type>(Events::ChangeAudioPostList)), postList(x), what(y) {}

        AudioPostList postList;
        const AddOrRemove what;
};
*/

class AudioRewireEvent : public QEvent
{
    public:
        AudioRewireEvent(AudioPostList *x) : QEvent(static_cast<QEvent::Type>(Events::AudioRewire)), postList(x) {}
        AudioPostList *const postList;
};

class EventSendEvent : public QEvent
{
    public:
        EventSendEvent(xine_event_t *e) : QEvent(static_cast<QEvent::Type>(Events::EventSend)), event(e) {}
        const xine_event_t *const event;
};

class SetParamEvent : public QEvent
{
    public:
        SetParamEvent(int p, int v) : QEvent(static_cast<QEvent::Type>(Events::SetParam)), param(p), value(v) {}
        const int param;
        const int value;
};

class MrlChangedEvent : public QEvent
{
    public:
        MrlChangedEvent(const QByteArray &_mrl, XineStream::StateForNewMrl _s)
            : QEvent(static_cast<QEvent::Type>(Events::MrlChanged)), mrl(_mrl), stateForNewMrl(_s) {}
        const QByteArray mrl;
        const XineStream::StateForNewMrl stateForNewMrl;
};

class GaplessSwitchEvent : public QEvent
{
    public:
        GaplessSwitchEvent(const QByteArray &_mrl) : QEvent(static_cast<QEvent::Type>(Events::GaplessSwitch)), mrl(_mrl) {}
        const QByteArray mrl;
};

class SeekCommandEvent : public QEvent
{
    public:
        SeekCommandEvent(qint64 t) : QEvent(static_cast<QEvent::Type>(Events::SeekCommand)), valid(true), time(t) {}
        bool valid;
        const qint64 time;
};

class SetTickIntervalEvent : public QEvent
{
    public:
        SetTickIntervalEvent(qint32 i) : QEvent(static_cast<QEvent::Type>(Events::SetTickInterval)), interval(i) {}
        const qint32 interval;
};

class SetPrefinishMarkEvent : public QEvent
{
    public:
        SetPrefinishMarkEvent(qint32 i) : QEvent(static_cast<QEvent::Type>(Events::SetPrefinishMark)), time(i) {}
        const qint32 time;
};

} // namespace Xine
} // namespace Phonon

#endif // PHONON_XINE_EVENTS_H
