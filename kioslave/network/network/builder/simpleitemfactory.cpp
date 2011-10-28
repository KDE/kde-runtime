/*
    This file is part of the Mollet network library, part of the KDE project.

    Copyright 2009-2010 Friedrich W. H. Kossebau <kossebau@kde.org>

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
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "simpleitemfactory.h"

// lib
#include "netservice_p.h"
// #include "service.h"
#include "upnp/cagibidevice.h"

// Qt
#include <QIcon>

// KDE
#include <KUrl>

#include <KDebug>

namespace Mollet
{

struct DNSSDServiceDatum {
    const char* dnssdTypeName;
    const char* typeName;
    const char* fallbackIconName;
    bool isFilesystem;
    const char* protocol; // KDE can forward to it
    const char* pathField;
    const char* iconField;
    const char* userField;
    const char* passwordField;

    void feedUrl( KUrl* url, const DNSSD::RemoteService* remoteService ) const;
};

static const DNSSDServiceDatum DNSSDServiceData[] =
{
    // file services
    { "_ftp._tcp",         "ftp",        "folder-remote",  true, "ftp",     "path", 0, "u", "p" },
    { "_sftp-ssh._tcp",    "sftp-ssh",   "folder-remote",  true, "sftp",    0,      0, "u", "p" },
    { "_ftps._tcp",        "ftps",       "folder-remote",  true, "ftps",    "path", 0, "u", "p" },
    { "_nfs._tcp",         "nfs",        "folder-remote",  true, "nfs",     "path", 0, 0, 0 },
    { "_afpovertcp._tcp",  "afpovertcp", "folder-remote",  true, "afp",     "path", 0, 0, 0 },
    { "_smb._tcp",         "smb",        "folder-remote",  true, "smb",     "path", 0, "u", "p" },
    { "_webdav._tcp",      "webdav",     "folder-remote",  true, "webdav",  "path", 0, "u", "p" },
    { "_webdavs._tcp",     "webdavs",    "folder-remote",  true, "webdavs", "path", 0, "u", "p" },

    { "_svn._tcp",    "svn",   "folder-sync",  true, 0, 0, 0, 0, 0 },
    { "_rsync._tcp",  "rsync", "folder-sync",  true, 0, 0, 0, 0, 0 },

    // email
    { "_imap._tcp",   "imap",   "email",  false, 0, 0, 0, 0, 0 },
    { "_pop3._tcp",   "pop3",   "email",  false, "pop3", 0, 0, 0, 0 },

    // shell services
    { "_ssh._tcp",    "ssh",    "terminal",  false, "ssh",    0,      0, "u", "p" },
    { "_telnet._tcp", "telnet", "terminal",  false, "telnet", 0,      0, "u", "p" },
    { "_rfb._tcp",    "rfb",    "krfb",      false, "vnc",    "path", 0, "u", "p" },
    { "_rdp._tcp",    "rdp",    "krfb",      false, "rdp", 0, 0, 0, 0 },

    // other standard services
    { "_http._tcp",   "http",   "folder-html",            false, "http", "path", 0, "u", "p" },
    { "_ntp._udp",    "ntp",    "xclock",                 false, 0, 0, 0, 0, 0 },
    { "_ldap._tcp",   "ldap",   "user-group-properties",  false, "ldap", 0, 0, 0, 0 },

    // user2user (chat, collaboration)
    { "_xmpp-server._tcp", "xmpp-server", "xchat",         false, "jabber", 0, 0, 0, 0 },
    { "_presence._tcp",    "presence",    "im-user",       false, "presence", 0, 0, 0, 0 },
    { "_lobby._tcp",       "lobby",       "document-edit", false, 0, 0, 0, 0, 0 },
    { "_giver._tcp",       "giver",       "folder-remote", false, 0, 0, 0, 0, 0 },
    { "_sip._udp",         "sip",         "phone",         false, 0, 0, 0, 0, 0 },
    { "_h323._tcp",        "h323",        "phone",         false, 0, 0, 0, 0, 0 },
    { "_skype._tcp",       "skype",       "phone",         false, 0, 0, 0, 0, 0 },

    // printing
    { "_ipp._tcp",            "ipp",            "printer", false, "ipp",  "path", 0, "u", "p" },
    { "_printer._tcp",        "printer",        "printer", false, 0, 0, 0, 0, 0 },
    { "_pdl-datastream._tcp", "pdl-datastream", "printer", false, 0, 0, 0, 0, 0 },

    // KDE workspace
    { "_plasma._tcp",       "plasma",      "plasma",       false, "plasma", "name", "icon", 0, 0 },

    // KDE games
    { "_kbattleship._tcp",  "kbattleship", "kbattleship",  false, "kbattleship", 0, 0, 0, 0 },
    { "_lskat._tcp",        "lskat",       "lskat",        false, 0, 0, 0, 0, 0 },
    { "_kfourinline._tcp",  "kfourinline", "kfourinline",  false, 0, 0, 0, 0, 0 },
    { "_ksirk._tcp",        "ksirk",       "ksirk",        false, 0, 0, 0, 0, 0 },

    // hardware
    { "_pulse-server._tcp","pulse-server","audio-card",          false, 0, 0, 0, 0, 0 },
    { "_pulse-source._tcp","pulse-source","audio-input-line",    false, 0, 0, 0, 0, 0 },
    { "_pulse-sink._tcp",  "pulse-sink",  "speaker",             false, 0, 0, 0, 0, 0 },
    { "_udisks-ssh._tcp",  "udisks-ssh",  "drive-harddisk",      false, 0, 0, 0, 0, 0 },
    { "_libvirt._tcp",     "libvirt",     "computer",            false, 0, 0, 0, 0, 0 },
    { "_airmouse._tcp",    "airmouse",    "input-mouse",         false, 0, 0, 0, 0, 0 },

    // database
    { "_postgresql._tcp",       "postgresql",       "server-database",  false, 0, 0, 0, 0, 0 },
    { "_couchdb_location._tcp", "couchdb_location", "server-database",  false, 0, 0, 0, 0, 0 },

    // else
    { "_realplayfavs._tcp","realplayfavs","favorites",           false, 0, 0, 0, 0, 0 },
    { "_acrobatSRV._tcp",  "acrobat-server","application-pdf",   false, 0, 0, 0, 0, 0 },
    { "_adobe-vc._tcp",    "adobe-vc",    "services",   false, 0, 0, 0, 0, 0 },
    { "_ggz._tcp",         "ggz",         "applications-games",  false, "ggz", 0, 0, 0, 0 },

    { "_pgpkey-ldap._tcp",  "pgpkey-ldap",  "application-pgp-keys",  false, 0, 0, 0, 0, 0 },
    { "_pgpkey-hkp._tcp",   "pgpkey-hkp",   "application-pgp-keys",  false, 0, 0, 0, 0, 0 },
    { "_pgpkey-https._tcp", "pgpkey-https", "application-pgp-keys",  true, "https", "path", 0, 0, 0 },
    
    // Maemo
    { "_maemo-inf._tcp",    "maemo-inf",    "pda",  false, 0, 0, 0, 0, 0 },
    // TODO: _maemo-inf._tcp seems to be not a service, just some about info, use to identify device and hide

    // Apple
    { "_airport._tcp",       "airport",       "network-wireless",  false, 0, 0, 0, 0, 0 },
    { "_daap._tcp",          "daap",          "folder-sound",      false, 0, 0, 0, 0, 0 },
    { "_dacp._tcp",          "dacp",          "folder-sound",      false, 0, 0, 0, 0, 0 },
    { "_eppc._tcp",          "eppc",          "network-connect",   false, 0, 0, 0, 0, 0 },
    { "_net-assistant._udp", "net-assistant", "services",          false, 0, 0, 0, 0, 0 },
    { "_odisk._tcp",         "odisk",         "media-optical",     false, 0, 0, 0, 0, 0 },
    { "_raop._tcp",          "raop",          "speaker",           false, 0, 0, 0, 0, 0 },
    { "_touch-able._tcp",    "touch-able",    "input-tablet",      false, 0, 0, 0, 0, 0 },
    { "_workstation._tcp",   "workstation",   "network-workgroup", false, 0, 0, 0, 0, 0 },
    { "_sleep-proxy._udp",   "sleep-proxy",   "services",          false, 0, 0, 0, 0, 0 },
    { "_nssocketport._tcp",  "nssocketport",  "services",          false, 0, 0, 0, 0, 0 },
    { "_home-sharing._tcp",  "home-sharing",  "services",          false, 0, 0, 0, 0, 0 },
    { "_appletv-itunes._tcp","appletv-itunes","services",          false, 0, 0, 0, 0, 0 },
    { "_appletv-pair._tcp",  "appletv-pair",  "services",          false, 0, 0, 0, 0, 0 }
};
//     result["_ssh._tcp"]=      DNSSDNetServiceBuilder(i18n("Remote disk (fish)"),     "fish",   "service/ftp", QString(), "u", "p");
// network-server-database icon
// see             // SubEthaEdit 2
//                 Defined TXT keys: txtvers=1, name=<Full Name>, userid=<User ID>, version=2

static const int DNSSDServiceDataSize = sizeof( DNSSDServiceData ) / sizeof( DNSSDServiceData[0] );

static const DNSSDServiceDatum UnknownServiceDatum = { "", "unknown", "unknown", false, 0, 0, 0, 0, 0 };

// TODO:
// * find out how ws (webservices, _ws._tcp), upnp (_upnp._tcp) are exactly meant to be used
// * learn about uddi
// * see how the apple specific protocols can be used for devicetype identification (printer, scanner)


void DNSSDServiceDatum::feedUrl( KUrl* url, const DNSSD::RemoteService* remoteService ) const
{
    const QMap<QString,QByteArray> serviceTextData = remoteService->textData();

    url->setProtocol( QString::fromLatin1(protocol) );
    if( userField )
        url->setUser( QLatin1String(serviceTextData.value(QLatin1String(userField)).constData()) );
    if( passwordField )
        url->setPass( QLatin1String(serviceTextData.value(QLatin1String(passwordField)).constData()) );
    if( pathField )
        url->setPath( QLatin1String(serviceTextData.value(QLatin1String(pathField)).constData()) );

    url->setHost( remoteService->hostName() );
    url->setPort( remoteService->port() );
}


SimpleItemFactory::SimpleItemFactory()
  : AbstractNetSystemFactory()
{
}


bool SimpleItemFactory::canCreateNetSystemFromDNSSD( const QString& serviceType ) const
{
Q_UNUSED( serviceType )
    return true;
}

QString SimpleItemFactory::dnssdId( const DNSSD::RemoteService::Ptr& dnssdService ) const
{
    return dnssdService->type() + QLatin1Char('_') + dnssdService->serviceName();
}

NetServicePrivate* SimpleItemFactory::createNetService( const DNSSD::RemoteService::Ptr& dnssdService, const NetDevice& device ) const
{
    NetServicePrivate* result;

    const QString dnssdServiceType = dnssdService->type();

    const DNSSDServiceDatum* serviceDatum = &UnknownServiceDatum;
    for( int i = 0; i< DNSSDServiceDataSize; ++i )
    {
        const DNSSDServiceDatum* datum = &DNSSDServiceData[i];
        if( dnssdServiceType == QLatin1String(datum->dnssdTypeName) )
        {
            serviceDatum = datum;
            break;
        }
    }

    KUrl url;
    if( serviceDatum->protocol )
        serviceDatum->feedUrl( &url, dnssdService.data() );

    const bool isUnknown = ( serviceDatum == &UnknownServiceDatum );
    const QString typeName = isUnknown ?
        dnssdServiceType.mid( 1, dnssdServiceType.lastIndexOf(QLatin1Char('.'))-1 ) :
        QString::fromLatin1( serviceDatum->typeName );

    QString iconName = QString::fromLatin1(serviceDatum->fallbackIconName);
    if ( serviceDatum->iconField ) {
        const QMap<QString,QByteArray> serviceTextData = dnssdService->textData();
        QString serviceIconName = QString::fromUtf8(serviceTextData[QString::fromLatin1(serviceDatum->iconField)]);
        if ( QIcon::hasThemeIcon(serviceIconName) ) {
            iconName = serviceIconName;
        }
    }

    result = new NetServicePrivate( dnssdService->serviceName(), iconName,
        typeName, device, url.url(), SimpleItemFactory::dnssdId(dnssdService) );

    return result;
}

bool SimpleItemFactory::canCreateNetSystemFromUpnp( const Cagibi::Device& upnpDevice ) const
{
Q_UNUSED( upnpDevice )
    return true;
}

QString SimpleItemFactory::upnpId( const Cagibi::Device& upnpDevice ) const
{
    return upnpDevice.udn();
}

// TODO: add KIcon with specialiced KIconLoader (fetches Icons via D-Bus)
NetServicePrivate* SimpleItemFactory::createNetService( const Cagibi::Device& upnpDevice, const NetDevice& device ) const
{
    NetServicePrivate* result;

    QString url = upnpDevice.presentationUrl();
    if( url.isEmpty() )
    {
        url = QString::fromLatin1( "upnp://" );
        url.append( upnpDevice.udn() );
    }
    result = new NetServicePrivate( upnpDevice.friendlyName(),
        QString::fromLatin1("unknown"),
        QString::fromLatin1("upnp.")+upnpDevice.type(), device, url, SimpleItemFactory::upnpId(upnpDevice) );

    return result;
}

SimpleItemFactory::~SimpleItemFactory() {}

}
