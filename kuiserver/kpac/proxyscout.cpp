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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include <cstdlib>
#include <ctime>

#include <dcopclient.h>
#include <kapplication.h>
#include <klocale.h>
#include <knotifyclient.h>
#include <kprotocolmanager.h>

#include "proxyscout.moc"
#include "discovery.h"
#include "script.h"

namespace KPAC
{
    ProxyScout::QueuedRequest::QueuedRequest( const KURL& u )
        : transaction( kapp->dcopClient()->beginTransaction() ),
          url( u )
    {
    }

    ProxyScout::ProxyScout( const QCString& name )
        : KDEDModule( name ),
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

    QString ProxyScout::proxyForURL( const KURL& url )
    {
        if ( m_suspendTime )
        {
            if ( std::time( 0 ) - m_suspendTime < 300 ) return "DIRECT";
            m_suspendTime = 0;
        }

        // Never use a proxy for the script itself
        if ( m_downloader && url == m_downloader->scriptURL() ) return "DIRECT";

        if ( m_script ) return handleRequest( url );

        m_requestQueue.append( url );
        if ( !m_downloader ) startDownload();
        return QString::null;
    }

    ASYNC ProxyScout::blackListProxy( const QString& proxy )
    {
        m_blackList[ proxy ] = std::time( 0 );
    }

    ASYNC ProxyScout::reset()
    {
        delete m_script;
        m_script = 0;
        delete m_downloader;
        m_downloader = 0;
        m_blackList.clear();
        m_suspendTime = 0;
        KProtocolManager::reparseConfiguration();
    }

    void ProxyScout::startDownload()
    {
        switch ( KProtocolManager::proxyType() )
        {
            case KProtocolManager::WPADProxy:
                m_downloader = new Discovery( this );
                break;
            case KProtocolManager::PACProxy:
                m_downloader = new Downloader( this );
                m_downloader->download( KProtocolManager::proxyConfigScript() );
                break;
            default:
                downloadResult( false );
                return;
        }
        connect( m_downloader, SIGNAL( result( bool ) ),
                 SLOT( downloadResult( bool ) ) );
    }

    void ProxyScout::downloadResult( bool success )
    {
        KNotifyClient::Instance notifyInstance( m_instance );
        if ( success )
            try
            {
                m_script = new Script( m_downloader->script() );
            }
            catch ( const Script::Error& e )
            {
                KNotifyClient::event( "script-error", i18n( 
                    "There proxy configuration script is invalid:\n%1" )
                    .arg( e.message() ) );
                success = false;
            }
        else KNotifyClient::event( "download-error", m_downloader->error() );

        for ( RequestQueue::ConstIterator it = m_requestQueue.begin();
              it != m_requestQueue.end(); ++it )
        {
            QCString type = "QString";
            QByteArray data;
            QDataStream ds( data, IO_WriteOnly );
            if ( success ) ds << handleRequest( ( *it ).url );
            else ds << QString( "DIRECT" );
            kapp->dcopClient()->endTransaction( ( *it ).transaction, type, data );
        }
        m_requestQueue.clear();
        m_downloader->deleteLater();
        m_downloader = 0;
        // Suppress further attempts for 5 minutes
        if ( !success ) m_suspendTime = std::time( 0 );
    }

    QString ProxyScout::handleRequest( const KURL& url )
    {
        try
        {
            QString result = m_script->evaluate( url );
            QStringList proxies = QStringList::split( ';', result );
            for ( QStringList::ConstIterator it = proxies.begin();
                  it != proxies.end(); ++it )
            {
                QString proxy = ( *it ).stripWhiteSpace();
                if ( proxy.left( 5 ) == "PROXY" )
                {
                    KURL proxyURL( proxy = proxy.mid( 5 ).stripWhiteSpace() );
                    // If the URL is invalid or the URL is valid but in opaque
                    // format which indicates a port number being present in
                    // this particular case, simply calling setProtocol() on
                    // it trashes the whole URL.
                    int len = proxyURL.protocol().length();
                    if ( !proxyURL.isValid() || proxy.find( ":/", len ) != len )
                        proxy.prepend("http://");
                    BlackList::Iterator it = m_blackList.find( proxy );
                    if ( it == m_blackList.end() ) return proxy;
                    else if ( std::time( 0 ) - *it > 1800 ) // 30 minutes
                    {
                        // black listing expired
                        m_blackList.remove( it );
                        return proxy;
                    }
                }
                else return "DIRECT";
            }
            // FIXME: blacklist
        }
        catch ( const Script::Error& e )
        {
            KNotifyClient::Instance notifyInstance( m_instance );
            KNotifyClient::event( "evaluation-error", i18n( 
                "The proxy configuration script returned an error:\n%1" )
                    .arg( e.message() ) );
        }
        return "DIRECT";
    }

    extern "C" KDEDModule* create_proxyscout( const QCString& name )
    {
        return new ProxyScout( name );
    }
}

// vim: ts=4 sw=4 et
