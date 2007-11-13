/*
   Copyright (C) 2007 by Olivier Goffart <ogoffart at kde.org>


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
#include "notifybyktts.h" 
#include <QtDBus>
#include <QHash>

#include <kmacroexpander.h>

#include "knotifyconfig.h"

NotifyByKTTS::NotifyByKTTS(QObject *parent) : KNotifyPlugin(parent) , kspeech("org.kde.kttsd", "/KSpeech", "org.kde.KSpeech")
{
	kspeech.call("setApplicationName", "KNotify");
}


NotifyByKTTS::~NotifyByKTTS()
{
}

void NotifyByKTTS::notify( int id, KNotifyConfig * config )
{
	QString say = config->readEntry( "KTTS" );
	
	if (!say.isEmpty()) {
		QHash<QChar,QString> subst;
		subst.insert( 'e', config->eventid );
		subst.insert( 'a', config->appname );
		subst.insert( 's', config->text );
		subst.insert( 'w', QString::number( (int)config->winId ));
		subst.insert( 'i', QString::number( id ));
		subst.insert( 'm', config->text );
		say = KMacroExpander::expandMacrosShellQuote( say, subst );
	}
	
	if ( say.isEmpty() )
		say = config->text; // fallback
	
	kspeech.call(QDBus::NoBlock, "say", say, 0);

	finished(id);
}

#include "notifybyktts.moc"
