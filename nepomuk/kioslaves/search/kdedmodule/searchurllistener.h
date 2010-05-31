/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SEARCH_URL_LISTENER_H_
#define _NEPOMUK_SEARCH_URL_LISTENER_H_

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtDBus/QDBusObjectPath>

#include <kurl.h>
#include <nepomuk/result.h>

#include "queryinterface.h"


namespace Nepomuk {
    class SearchUrlListener : public QObject
    {
        Q_OBJECT

    public:
        SearchUrlListener( const KUrl& queryUrl, const KUrl& notifyUrl = KUrl() );
        ~SearchUrlListener();

        int ref();
        int unref();

    private Q_SLOTS:
        void slotNewEntries( const QList<Nepomuk::Query::Result>& entries );
        void slotEntriesRemoved( const QStringList& entries );
        void slotQueryServiceInitialized( bool success );

    private:
        void createInterface();

        int m_ref;
        KUrl m_queryUrl;
        KUrl m_notifyUrl;
        org::kde::nepomuk::Query* m_queryInterface;
    };
}

#endif
