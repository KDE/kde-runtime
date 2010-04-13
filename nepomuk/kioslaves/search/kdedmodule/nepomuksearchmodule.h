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

#ifndef _NEPOMUK_SEARCH_KDED_MODULE_H_
#define _NEPOMUK_SEARCH_KDED_MODULE_H_

#include <kdedmodule.h>
#include <kurl.h>

#include <QtDBus/QDBusContext>
#include <QtCore/QMultiHash>

class QDBusServiceWatcher;

namespace Nepomuk {

    class SearchUrlListener;

    class SearchModule : public KDEDModule,  protected QDBusContext
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.kio.NepomukSearchBridge")

    public:
        SearchModule( QObject* parent, const QList<QVariant>& );
        ~SearchModule();

    public Q_SLOTS:
        Q_SCRIPTABLE void registerSearchUrl( const QString& url );
        Q_SCRIPTABLE void unregisterSearchUrl( const QString& url );
        Q_SCRIPTABLE QStringList watchedSearchUrls();

    private Q_SLOTS:
        void slotServiceUnregistered( const QString& serviceName );

    private:
        void unrefUrl( const KUrl& url );

        QHash<KUrl, SearchUrlListener*> m_queryHash;
        QMultiHash<QString, KUrl> m_dbusServiceUrlHash;

        QDBusServiceWatcher *m_watcher;
    };
}

#endif
