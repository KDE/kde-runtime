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


#ifndef WIRECALL_H
#define WIRECALL_H

#include "sinknode.h"
#include "sourcenode.h"

namespace Phonon
{
namespace Xine
{

class WireCall
{
    public:
        WireCall() : src(0), snk(0) {}
        WireCall(SourceNode *a, SinkNode *b) : source(a->threadSafeObject), sink(b->threadSafeObject), src(a), snk(b) {}
        /**
         * If the two WireCalls are in separate graphs returns false
         *
         * If the two WireCalls are in the same graph:
         * returns true if this wire is a source for \p rhs
         * returns false if \p rhs is a source for this wire
         */
        bool operator<(const WireCall &rhs) const {
            SourceNode *s = rhs.src;
            if (src == s) {
                return true;
            }
            while (s->sinkInterface() && s->sinkInterface()->source()) {
                s = s->sinkInterface()->source();
                if (src == s) {
                    return true;
                }
            }
            return false;
        }

        bool operator==(const WireCall &rhs) const
        {
            return source == rhs.source && sink == rhs.sink;
        }

        QExplicitlySharedDataPointer<SourceNodeXT> source;
        QExplicitlySharedDataPointer<SinkNodeXT> sink;

    private:
        SourceNode *src;
        SinkNode *snk;
};
} // namespace Xine
} // namespace Phonon

/*
inline uint qHash(const Phonon::Xine::WireCall &w)
{
    const uint h1 = qHash(w.source);
    const uint h2 = qHash(w.sink);
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}
*/

#endif // WIRECALL_H
