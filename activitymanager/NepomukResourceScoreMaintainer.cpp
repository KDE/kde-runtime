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

#include "NepomukResourceScoreMaintainer.h"

#include <QList>
#include <QMutex>
#include <QThread>

#include <KDebug>

#include <time.h>

class NepomukResourceScoreMaintainerPrivate: public QThread {
public:
    QList < Nepomuk::Resource > resources;
    QMutex resources_mutex;

    void run();

    static NepomukResourceScoreMaintainer * s_instance;

};

NepomukResourceScoreMaintainer * NepomukResourceScoreMaintainerPrivate::s_instance = NULL;

void NepomukResourceScoreMaintainerPrivate::run()
{
    forever {
        // initial delay before processing the resources
        sleep(5);

        NepomukResourceScoreMaintainerPrivate::resources_mutex.lock();

        if (resources.count() == 0) {
            kDebug() << "No more resources to process, exiting.";

            NepomukResourceScoreMaintainerPrivate::resources_mutex.unlock();
            return;
        }

        QList < Nepomuk::Resource > currentResources = resources;
        resources.clear();

        NepomukResourceScoreMaintainerPrivate::resources_mutex.unlock();
    }
}

NepomukResourceScoreMaintainer * NepomukResourceScoreMaintainer::self()
{
    if (!NepomukResourceScoreMaintainerPrivate::s_instance) {
        NepomukResourceScoreMaintainerPrivate::s_instance = new NepomukResourceScoreMaintainer();
    }

    return NepomukResourceScoreMaintainerPrivate::s_instance;
}

NepomukResourceScoreMaintainer::NepomukResourceScoreMaintainer()
    : d(new NepomukResourceScoreMaintainerPrivate())
{
}

NepomukResourceScoreMaintainer::~NepomukResourceScoreMaintainer()
{
    delete d;
}

void NepomukResourceScoreMaintainer::processResource(const Nepomuk::Resource & resource)
{
    d->resources_mutex.lock();

    if (!d->resources.contains(resource)) {
        d->resources.append(resource);
    }

    d->resources_mutex.unlock();

    d->start();
}


