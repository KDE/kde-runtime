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

#ifndef AUDIOPORT_P_H
#define AUDIOPORT_P_H

#include "audioport.h"
#include <QObject>

namespace Phonon
{
namespace Xine
{
class AudioPortDeleter : public QObject
{
    Q_OBJECT
    public:
        AudioPortDeleter(AudioPortData *);
        ~AudioPortDeleter();

    protected:
        void timerEvent(QTimerEvent *);

    private:
        QExplicitlySharedDataPointer<AudioPortData> d;
};
} // namespace Xine
} // namespace Phonon

#endif // AUDIOPORT_P_H
// vim: sw=4 sts=4 et tw=100
