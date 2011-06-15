/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef NEPOMUK_RESOURCE_SCORE_MAINTAINER_H_
#define NEPOMUK_RESOURCE_SCORE_MAINTAINER_H_

#include <QThread>
#include <Nepomuk/Resource>

#include "Event.h"

class NepomukResourceScoreMaintainerPrivate;

/**
 * Thread to process desktop/usage events
 */
class NepomukResourceScoreMaintainer {
public:
    static NepomukResourceScoreMaintainer * self();

    virtual ~NepomukResourceScoreMaintainer();

    void processResource(const Nepomuk::Resource & resource);

private:
    NepomukResourceScoreMaintainer();

    class NepomukResourceScoreMaintainerPrivate * const d;
};

#endif // NEPOMUK_RESOURCE_SCORE_MAINTAINER_H_
