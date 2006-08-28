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


#include <cstdlib>
#include <ctime>

#include <kapplication.h>
#include <klocale.h>
#include <knotification.h>
#include <kprotocolmanager.h>
#include <QtDBus/QtDBus>

#include "proxyscout.moc"
#include "discovery.h"
#include "script.h"

namespace KPAC
{
    ProxyScout::QueuedRequest::QueuedRequest( const QDBusMessage &reply, const KUrl& u )
        : transaction( reply ), url( u )
    {
    }

    ProxyScout::ProxyScout()
        : KDEDModule(),
          m_instance( new KInstance( "proxyscout" ) ),
          m_downloader( 0 ),
          m_script( 0 ),
          m_suspendTime( 0 )
    {
    }

    ProxyScout::~ProxyScout()
    {
        delete m_script;
        delete m_instance;
    }

    QString ProxyScout::proxyForUrl( const KUrl& url, const QDBusMessage &msg )
    {
        if ( m_suspendTime )
        {
            if ( std::time( 0 ) - m_suspendTime < 300 ) return "DIRECT";
            m_suspendTime = 0;
        }

        // Never use a proxy for the script itself
        if ( m_downloader && url.equals( m_downloader->scriptUrl(), KUrl::CompareWithoutTrailingSlash ) ) return "DIRECT";

        if ( m_script ) return handleRequest( url );

        if ( m_downloader || startDownload() )
        {
            msg.setDelayedReply(true);
            m_requestQueue.append( QueuedRequest( msg, url ) );
            return QString();   // return value will be ignored
        }
        else return "DIRECT";
    }

    void ProxyScout::blackListProxy( const QString& proxy )
    {
        m_blackList[ proxy ] = std::time( 0 );
    }

    void ProxyScout::reset()
    {
        delete m_script;
        m_script = 0;
        delete m_downloader;
        m_downloader = 0;
        m_blackList.clear();
        m_suspendTime = 0;
        KProtocolManager::reparseConfiguration();
    }

    bool ProxyScout::startDownload()
    {
        switch ( KProtocolManager::proxyType() )
        {
            case KProtocolManager::WPADProxy:
                m_downloader = new Discovery( this );
                break;
            case KProtocolManager::PACProxy:
                m_downloader = new Downloader( this );
                m_downloader->download( KUrl( KProtocolManager::proxyConfigScript() ) );
                break;
            default:
                return false;
        }
        connect( m_downloader, SIGNAL( result( bool ) ),
                 SLOT( downloadResult( bool ) ) );
        return true;
    }

    void ProxyScout::downloadResult( bool success )
    {
        if ( success )
            try
            {
                m_script = new Script( m_downloader->script() );
            }
            catch ( const Script::Error& e )
            {
                KNotification *notify= new KNotification ( "script-error" );
                notify->setText( i18n("The proxy configuration script is invalid:\n%1" , e.message() ) );
                notify->setInstance( m_instance );
                notify->sendEvent();
                success = false;
            }
        else
        {
		KNotification *notify = new KNotification ("download-error");
		notify->setText( m_downloader->error() );
		notify->setInstance( m_instance);
		notify->sendEvent();
        }

        for ( RequestQueue::Iterator it = m_requestQueue.begin();
              it != m_requestQueue.end(); ++it )
        {
            if ( success )
                QDBusConnection::sessionBus().send( ( *it ).transaction.createReply( handleRequest( ( *it ).url ) ) );
            else
                QDBusConnection::sessionBus().send( ( *it ).transaction.createReply( QString( "DIRECT" ) ) );
        }
        m_requestQueue.clear();
        m_downloader->deleteLater();
        m_downloader = 0;
        // Suppress further attempts for 5 minutes
        if ( !success ) m_suspendTime = std::time( 0 );
    }

    QString ProxyScout::handleRequest( const KUrl& url )
    {
        try
        {
            QString result = m_script->evaluate( url );
            QStringList proxies = result.split( ';', QString::SkipEmptyParts );
            for ( QStringList::ConstIterator it = proxies.begin();
                  it != proxies.end(); ++it )
            {
                QString proxy = ( *it ).trimmed();
                if ( proxy.startsWith( QLatin1String( "PROXY" ) ) )
                {
                    KUrl proxyURL( proxy = proxy.mid( 5 ).trimmed() );
                    // If the URL is invalid or the URL is valid but in opaque
                    // format which indicates a port number being present in
                    // this particular case, simply calling setProtocol() on
                    // it trashes the whole URL.
                    int len = proxyURL.protocol().length();
                    if ( !proxyURL.isValid() || proxy.indexOf( ":/", len ) != len )
                        proxy.prepend("http://");
                    if ( !m_blackList.contains( proxy ) )
                        return proxy;
                    if ( std::time( 0 ) - m_blackList[ proxy ] > 1800 ) // 30 minutes
                    {
                        // black listing expired
                        m_blackList.remove( proxy );
                        return proxy;
                    }
                }
                else return "DIRECT";
            }
            // FIXME: blacklist
        }
        catch ( const Script::Error& e )
        {
		KNotification *n=new KNotification( "evaluation-error" );
		n->setText( i18n( "The proxy configuration script returned an error:\n%1" , e.message() ) );
		n->setInstance(m_instance);
		n->sendEvent();
        }
        return "DIRECT";
    }

    extern "C" KDE_EXPORT KDEDModule* create_proxyscout()
    {
        return new ProxyScout();
    }
}

// vim: ts=4 sw=4 et
