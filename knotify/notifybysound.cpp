/*
   Copyright (c) 1997 Christian Esken (esken@kde.org)
                 2000 Charles Samuels (charles@kde.org)
                 2000 Stefan Schimanski (1Stein@gmx.de)
                 2000 Matthias Ettrich (ettrich@kde.org)
                 2000 Waldo Bastian <bastian@kde.org>
                 2000-2003 Carsten Pfeiffer <pfeiffer@kde.org>
                 2005 Allan Sandfeld Jensen <kde@carewolf.com>
                 2005-2006 by Olivier Goffart <ogoffart at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "notifybysound.h"
#include "knotifyconfig.h"


// QT headers
#include <QHash>
#include <QSignalMapper>
#include <QFileInfo>

// KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <config.h>
#include <kinstance.h>

// Phonon headers
#include <phonon/mediaobject.h>
#include <phonon/audiopath.h>
#include <phonon/audiooutput.h>


class NotifyBySound::Private 
{
	public:
		bool useExternal;
		QString externalPlayer;
		
		QHash<int, KProcess *> processes;
		QHash<int, Phonon::MediaObject*> mediaobjects;
		QSignalMapper *signalmapper;
		
		int volume;

};


NotifyBySound::NotifyBySound(QObject *parent) : KNotifyPlugin(parent),d(new Private)
{
	d->signalmapper = new QSignalMapper(this);
	connect(d->signalmapper, SIGNAL(mapped(int)), this, SLOT(slotSoundFinished(int)));
	
	Phonon::AudioPath* ap = new Phonon::AudioPath( this );
	Phonon::AudioOutput * ao = new Phonon::AudioOutput( this );
	ao->setCategory( Phonon::NotificationCategory  );
	ap->addOutput( ao );

	loadConfig();
}


NotifyBySound::~NotifyBySound()
{
	delete d;
}


void NotifyBySound::loadConfig() 
{
    // load external player settings
	KConfig *kc = KGlobal::config();
	kc->setGroup("Misc");
	d->useExternal = kc->readEntry( "Use external player", false );
	d->externalPlayer = kc->readPathEntry("External player");

	// try to locate a suitable player if none is configured
	if ( d->useExternal && d->externalPlayer.isEmpty() ) {
		QStringList players;
		players << "wavplay" << "aplay" << "auplay" << "artsplay" << "akodeplay";
		QStringList::Iterator it = players.begin();
		while ( d->externalPlayer.isEmpty() && it != players.end() ) {
			d->externalPlayer = KStandardDirs::findExe( *it );
			++it;
		}
	}
	// load default volume
	d->volume = kc->readEntry( "Volume", 100 );
}




void NotifyBySound::notify( int eventId, KNotifyConfig * config )
{
	kDebug() << k_funcinfo << endl;
	
	QString soundFile = config->readEntry( "sound" , true );
	if (soundFile.isEmpty())
	{
		finish( eventId );
		return;
	}

    // get file name
	if ( QFileInfo(soundFile).isRelative() )
	{
		QString search = QString("%1/sounds/%2").arg(config->appname).arg(soundFile);
		search = KGlobal::instance()->dirs()->findResource("data", search);
		if ( search.isEmpty() )
			soundFile = locate( "sound", soundFile );
		else
			soundFile = search;
	}
	if ( soundFile.isEmpty() )
	{
		finish( eventId );
		return;
	}
	
	kDebug() << k_funcinfo << " going to play " << soundFile << endl;

	if(!d->useExternal || d->externalPlayer.isEmpty())
	{
		
		Phonon::MediaObject *media = new Phonon::MediaObject( this );
		connect( media, SIGNAL( finished() ), d->signalmapper, SLOT(map()));
		d->signalmapper->setMapping( media , eventId );
		
		media->setUrl( KUrl::fromPath(soundFile) );
		media->play();
		d->mediaobjects.insert(eventId , media);
	}
	else
	{		
        // use an external player to play the sound
		KProcess *proc = new KProcess( this );
		connect( proc, SIGNAL(processExited(KProcess*)), d->signalmapper,  SLOT(map()));
		d->signalmapper->setMapping( proc , eventId );
		
		proc->clearArguments();
		(*proc) << d->externalPlayer << QFile::encodeName( soundFile );
		proc->start(KProcess::NotifyOnExit);
		return;
	}
}


void NotifyBySound::setVolume( int volume )
{
	if ( volume<0 ) volume=0;
	if ( volume>=100 ) volume=100;
	d->volume = volume;
}


void NotifyBySound::slotSoundFinished(int id)
{
	kDebug() << k_funcinfo << id << endl;
	if(d->mediaobjects.contains(id))
	{
		d->mediaobjects[id]->deleteLater();
		d->mediaobjects.remove(id);
	}
	if(d->processes.contains(id))
	{
		d->processes[id]->deleteLater();
		d->processes.remove(id);
	}
	finish(id);
}


void NotifyBySound::close(int id)
{
	if(d->mediaobjects.contains(id))
	{
		d->mediaobjects[id]->stop();
		d->mediaobjects[id]->deleteLater();
		d->mediaobjects.remove(id);
	}
	if(d->processes.contains(id))
	{
		d->processes[id]->kill();
		d->processes[id]->deleteLater();
		d->processes.remove(id);
	}
}

#include "notifybysound.moc"
