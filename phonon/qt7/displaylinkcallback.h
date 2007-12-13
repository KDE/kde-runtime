/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Phonon_QT7_DISPLAYLINKCALLBACK_H
#define Phonon_QT7_DISPLAYLINKCALLBACK_H

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include <QObject>
#include <QPointer>
#include <QMutex>

namespace Phonon
{
namespace QT7
{
    class MediaObject;

    class DisplayLinkCallback : public QObject
    {
        public:
            DisplayLinkCallback(MediaObject *mediaObject);
            virtual ~DisplayLinkCallback();
            void handleCallback(const CVTimeStamp *outputTime);

        private:
            MediaObject *mediaObject;
    };

}} // namespace Phonon::QT7

#endif // Phonon_QT7_DISPLAYLINKCALLBACK_H
