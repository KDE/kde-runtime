/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "activefilequeue.h"

#include <QtCore/QQueue>
#include <QtCore/QTimer>


namespace {
    class Entry {
    public:
        Entry(const KUrl& url, int c);
        bool operator==(const Entry& other) const;

        /// The file url
        KUrl url;
        /// The seconds left in this entry
        int cnt;
    };

    Entry::Entry(const KUrl &u, int c)
        : url(u),
          cnt(c)
    {
    }

    bool Entry::operator==(const Entry &other) const
    {
        // we ignore the counter since we need this for the search in queueUrl only
        return url == other.url;
    }
}

Q_DECLARE_TYPEINFO(Entry, Q_MOVABLE_TYPE);


class ActiveFileQueue::Private
{
public:
    QQueue<Entry> m_queue;
    QTimer m_queueTimer;
    int m_timeout;
};


ActiveFileQueue::ActiveFileQueue(QObject *parent)
    : QObject(parent),
      d(new Private())
{
    // we default to 5 seconds
    d->m_timeout = 5;

    // setup the timer
    connect(&d->m_queueTimer, SIGNAL(timeout()),
            this, SLOT(slotTimer()));

    // we check in 1 sec intervals
    d->m_queueTimer.setInterval(1000);
}

ActiveFileQueue::~ActiveFileQueue()
{
    delete d;
}

void ActiveFileQueue::enqueueUrl(const KUrl &url)
{
    Entry defaultEntry(url, d->m_timeout);
    QQueue<Entry>::iterator it = qFind(d->m_queue.begin(), d->m_queue.end(), defaultEntry);
    if(it == d->m_queue.end()) {
        // if this is a new url add it with the default timeout
        d->m_queue.enqueue(defaultEntry);
    }
    else {
        // If the url is already in the queue update its timestamp
        it->cnt = d->m_timeout;
    }

    // make sure the timer is running
    if(!d->m_queueTimer.isActive()) {
        d->m_queueTimer.start();
    }
}

void ActiveFileQueue::setTimeout(int seconds)
{
    d->m_timeout = seconds;
}

void ActiveFileQueue::slotTimer()
{
    // we run through the queue, decrease each counter and emit each entry which has a count of 0
    QQueue<Entry>::iterator it = d->m_queue.begin();
    while(it != d->m_queue.end()) {
        it->cnt--;
        if(it->cnt == 0) {
            emit urlTimeout(it->url);
            it = d->m_queue.erase(it);
        }
        else {
            ++it;
        }
    }

    // stop the timer in case we have nothing left to do
    if(d->m_queue.isEmpty()) {
        d->m_queueTimer.stop();
    }
}

#include "activefilequeue.moc"
