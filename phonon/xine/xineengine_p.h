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

#ifndef XINEENGINE_P_H
#define XINEENGINE_P_H

#include "xineengine.h"
#include <QObject>
#include <phonon/audiodevice.h>
#include <phonon/objectdescription.h>
#include <QTimer>

namespace Phonon
{
namespace Xine
{
class XineEnginePrivate : public QObject
{
    Q_OBJECT
    public:
        XineEnginePrivate();

        QTimer signalTimer;

    public slots:
        void devicePlugged(const AudioDevice &);
        void deviceUnplugged(const AudioDevice &);

    signals:
        void objectDescriptionChanged(ObjectDescriptionType);

    private slots:
        void emitAudioDeviceChange();
};
} // namespace Xine
} // namespace Phonon

#endif // XINEENGINE_P_H
// vim: sw=4 sts=4 et tw=100
