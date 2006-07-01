/*
   Copyright (c) 2003 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
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


#ifndef KPAC_PROXYSCOUT_H
#define KPAC_PROXYSCOUT_H


#include <qmap.h>

#include <kdedmodule.h>
#include <kurl.h>
#include <QtDBus/QtDBus>

#include <time.h>

class KInstance;

namespace KPAC
{
    class Downloader;
    class Script;

    class ProxyScout : public KDEDModule
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.KPAC.ProxyScout")
    public:
        ProxyScout();
        virtual ~ProxyScout();

    public Q_SLOTS:
        Q_SCRIPTABLE QString proxyForURL( const KUrl& url, const QDBusMessage & );
        Q_SCRIPTABLE Q_NOREPLY void blackListProxy( const QString& proxy );
        Q_SCRIPTABLE Q_NOREPLY void reset();

    private Q_SLOTS:
        void downloadResult( bool );

    private:
        bool startDownload();
        QString handleRequest( const KUrl& url );

        KInstance* m_instance;
        Downloader* m_downloader;
        Script* m_script;

        struct QueuedRequest
        {
            QueuedRequest() {}
            QueuedRequest( const QDBusMessage&, const KUrl& );

            QDBusMessage transaction;
            KUrl url;
        };
        typedef QList< QueuedRequest > RequestQueue;
        RequestQueue m_requestQueue;

        typedef QMap< QString, time_t > BlackList;
        BlackList m_blackList;
        time_t m_suspendTime;
    };
}

#endif // KPAC_PROXYSCOUT_H

// vim: ts=4 sw=4 et
