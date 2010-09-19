/*
    localdomainfilter.cpp

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>
    Copyright (C) 2010 Dawit Alemayehu <adawit@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "localdomainurifilter.h"

#include <KDE/KProtocolInfo>
#include <KDE/KDebug>

#include <QtCore/QStringBuilder>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QHostInfo>

#define QL1C(x)   QLatin1Char(x)
#define QL1S(x)   QLatin1String(x)

#define HOSTPORT_PATTERN "[a-zA-Z0-9][a-zA-Z0-9+-]*(?:\\:[0-9]{1,5})?(?:/[\\w:@&=+$,-.!~*'()]*)*"

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * ../tests/kurifiltertest
 */
LocalDomainUriFilter::LocalDomainUriFilter( QObject *parent, const QVariantList & /*args*/ )
                     :KUriFilterPlugin( "localdomainurifilter", parent ),
                      m_hostPortPattern(QL1S(HOSTPORT_PATTERN))
{
}

bool LocalDomainUriFilter::filterUri(KUriFilterData& data) const
{
    kDebug(7023) << data.typedString();

    const KUrl url = data.uri();
    const QString protocol = url.scheme();

    // When checking for local domain just validate it is indeed a local domain,
    // but do not modify the hostname! See bug#
    if ((protocol.isEmpty() || !KProtocolInfo::isKnownProtocol(protocol))  &&
        m_hostPortPattern.exactMatch(data.typedString())) {

        QString host (data.typedString().left(data.typedString().indexOf(QL1C('/'))));
        const int pos = host.indexOf(QL1C(':'));
        if (pos > -1)
            host.truncate(pos); // Remove port number

        kDebug(7023) << "Checking local domain for" <<  host;
        if (exists(host)) {
            QString scheme (data.defaultUrlScheme());
            if (scheme.isEmpty())
                scheme = QL1S("http://");
            setFilteredUri(data, KUrl(scheme % data.typedString()));
            setUriType(data, KUriFilterData::NetProtocol);
            return true;
        }
    }

    return false;
}

void LocalDomainUriFilter::lookedUp(const QHostInfo &hostInfo)
{
    if (hostInfo.lookupId() == m_lookupId) {
        m_hostExists = (hostInfo.error() == QHostInfo::NoError);
        if (m_eventLoop)
            m_eventLoop->exit();
    }
}

bool LocalDomainUriFilter::exists(const QString& host) const
{
    QEventLoop eventLoop;

    m_hostExists = false;
    m_eventLoop = &eventLoop;

    LocalDomainUriFilter *self = const_cast<LocalDomainUriFilter*>( this );
    m_lookupId = QHostInfo::lookupHost( host, self, SLOT(lookedUp(QHostInfo)) );

    QTimer t;
    connect( &t, SIGNAL(timeout()), &eventLoop, SLOT(quit()) );
    t.start(1000);

    eventLoop.exec();
    m_eventLoop = 0;

    t.stop();

    QHostInfo::abortHostLookup( m_lookupId );

    return m_hostExists;
}


K_PLUGIN_FACTORY(LocalDomainUriFilterFactory, registerPlugin<LocalDomainUriFilter>();)
K_EXPORT_PLUGIN(LocalDomainUriFilterFactory("kcmkurifilt"))

#include "localdomainurifilter.moc"
