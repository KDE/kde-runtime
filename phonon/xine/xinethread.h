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

#ifndef PHONON_XINE_XINETHREAD_H
#define PHONON_XINE_XINETHREAD_H

#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

namespace Phonon
{
namespace Xine
{
class XineStream;

class XineThread : public QThread
{
    Q_OBJECT
    public:
        XineThread();
        ~XineThread();

        void waitForEventLoop();
        static XineStream *newStream();
        void quit();

    protected:
        void run();
        bool event(QEvent *e);

    private slots:
        void eventLoopReady();

    private:
        QMutex m_mutex;
        QWaitCondition m_waitingForEventLoop;
        QWaitCondition m_waitingForNewStream;
        XineStream *m_newStream;
        bool m_eventLoopReady;
};

} // namespace Xine
} // namespace Phonon
#endif // PHONON_XINE_XINETHREAD_H
