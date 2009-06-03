/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

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

#include "kio_remote.h"

#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>

#include <QFile>

extern "C" {
	int KDE_EXPORT kdemain( int argc, char **argv )
	{
        // necessary to use other kio slaves
        KComponentData componentData("kio_remote" );
        QCoreApplication app(argc, argv);

        // start the slave
        RemoteProtocol slave( argv[1], argv[2], argv[3] );
        slave.dispatchLoop();
        return 0;
	}
}


RemoteProtocol::RemoteProtocol(const QByteArray &protocol,
                               const QByteArray &pool, const QByteArray &app)
	: SlaveBase(protocol, pool, app)
{
}

RemoteProtocol::~RemoteProtocol()
{
}

void RemoteProtocol::listDir(const KUrl &url)
{
	kDebug(1220) << "RemoteProtocol::listDir: " << url;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	int second_slash_idx = url.path().indexOf( '/', 1 );
	const QString root_dirname = url.path().mid( 1, second_slash_idx-1 );

	KUrl target = m_impl.findBaseURL( root_dirname );
	kDebug(1220) << "possible redirection target : " << target;
	if( target.isValid() )
	{
		target.addPath( url.path().remove(0, second_slash_idx) );
		redirection(target);
		finished();
		return;
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
}

void RemoteProtocol::listRoot()
{
	KIO::UDSEntry entry;

	KIO::UDSEntryList remote_entries;
        m_impl.listRoot(remote_entries);

	totalSize(remote_entries.count()+2);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);

	m_impl.createWizardEntry(entry);
	listEntry(entry, false);

	KIO::UDSEntryList::ConstIterator it = remote_entries.constBegin();
	const KIO::UDSEntryList::ConstIterator end = remote_entries.constEnd();
	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void RemoteProtocol::stat(const KUrl &url)
{
	kDebug(1220) << "RemoteProtocol::stat: " << url;

	QString path = url.path();
	if ( path.isEmpty() || path == "/" )
	{
		// The root is "virtual" - it's not a single physical directory
		KIO::UDSEntry entry;
		m_impl.createTopLevelEntry( entry );
		statEntry( entry );
		finished();
		return;
	}

	if (m_impl.isWizardURL(url))
	{
		KIO::UDSEntry entry;
		if (m_impl.createWizardEntry(entry))
		{
			statEntry(entry);
			finished();
		}
		else
		{
			error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
		}
		return;
	}

	int second_slash_idx = url.path().indexOf( '/', 1 );
	const QString root_dirname = url.path().mid( 1, second_slash_idx-1 );

	if ( second_slash_idx==-1 || ( (int)url.path().length() )==second_slash_idx+1 )
	{
		KIO::UDSEntry entry;
		if (m_impl.statNetworkFolder(entry, root_dirname))
		{
			statEntry(entry);
			finished();
			return;
		}
	}
	else
	{
		KUrl target = m_impl.findBaseURL(  root_dirname );
		kDebug( 1220 ) << "possible redirection target : " << target;
		if (  target.isValid() )
		{
			target.addPath( url.path().remove( 0, second_slash_idx ) );
			redirection( target );
			finished();
			return;
		}
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
}

void RemoteProtocol::del(const KUrl &url, bool /*isFile*/)
{
	kDebug(1220) << "RemoteProtocol::del: " << url;

	if (!m_impl.isWizardURL(url)
	 && m_impl.deleteNetworkFolder(url.fileName()))
	{
		finished();
		return;
	}

	error(KIO::ERR_CANNOT_DELETE, url.prettyUrl());
}

void RemoteProtocol::get(const KUrl &url)
{
	kDebug(1220) << "RemoteProtocol::get: " << url;

	const QString file = m_impl.findDesktopFile( url.fileName() );
	kDebug(1220) << "desktop file : " << file;

	if (!file.isEmpty())
	{
		KUrl desktop;
		desktop.setPath(file);

		redirection(desktop);
		finished();
		return;
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
}

void RemoteProtocol::rename(const KUrl &src, const KUrl &dest,
                            KIO::JobFlags flags)
{
	if (src.protocol()!="remote" || dest.protocol()!="remote"
         || m_impl.isWizardURL(src) || m_impl.isWizardURL(dest))
	{
		error(KIO::ERR_UNSUPPORTED_ACTION, src.prettyUrl());
		return;
	}

	if (m_impl.renameFolders(src.fileName(), dest.fileName(), flags & KIO::Overwrite))
	{
		finished();
		return;
	}

	error(KIO::ERR_CANNOT_RENAME, src.prettyUrl());
}
