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

#ifndef Phonon_QT7_BACKENDINFO_H
#define Phonon_QT7_BACKENDINFO_H

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include <phonon/mediasource.h>
#include <Carbon/Carbon.h>
#include <QtCore>

namespace Phonon
{
namespace QT7
{
    class BackendInfo
    {
        public:
            enum Scope {In, Out};

            static QMap<QString, QString> quickTimeMimeTypes(Scope scope);
            static QStringList quickTimeCompressionFormats();
            static QStringList coreAudioCodecs(Scope scope);
            static QStringList coreAudioFileTypes(Scope scope);

        private:
            static QString getMimeTypeTag(QTAtomContainer mimeList, int index, OSType type);

    };

}} // namespace Phonon::QT7

#endif // Phonon_QT7_BACKENDINFO_H
