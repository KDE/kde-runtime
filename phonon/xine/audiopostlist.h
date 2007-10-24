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

#include <QtCore/QExplicitlySharedDataPointer>

#ifndef PHONON_AUDIOPOSTLIST_H
#define PHONON_AUDIOPOSTLIST_H

typedef struct xine_post_out_s xine_post_out_t;

namespace Phonon
{
namespace Xine
{

class XineStream;
class AudioPostListData;
class Effect;
class AudioPort;

class AudioPostList
{
    public:
        AudioPostList();
        AudioPostList(const AudioPostList &);
        AudioPostList &operator=(const AudioPostList &);
        ~AudioPostList();
        bool operator==(const AudioPostList &rhs) const { return d == rhs.d; }

        // QList interface
        bool contains(Effect *) const;
        int indexOf(Effect *) const;
        void insert(int index, Effect *);
        void append(Effect *);
        int removeAll(Effect *);

        void setAudioPort(const AudioPort &);
        const AudioPort &audioPort() const;

        // called from the xine thread
        void setXineStream(XineStream *);
        void unsetXineStream(XineStream *);
        void wireStream();

    private:
        void wireStream(xine_post_out_t *audioSource);

        QExplicitlySharedDataPointer<AudioPostListData> d;
};

} // namespace Xine
} // namespace Phonon

#endif // PHONON_AUDIOPOSTLIST_H
