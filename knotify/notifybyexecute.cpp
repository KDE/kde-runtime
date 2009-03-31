/*
   Copyright (C) 2005-2006 by Olivier Goffart <ogoffart at kde.org>


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

#include "notifybyexecute.h"

#include <QHash>
#include <KProcess>

#include "knotifyconfig.h"

#include <kdebug.h>
#include <kmacroexpander.h>




NotifyByExecute::NotifyByExecute(QObject *parent) : KNotifyPlugin(parent)
{
}


NotifyByExecute::~NotifyByExecute()
{
}



void NotifyByExecute::notify( int id, KNotifyConfig * config )
{
	QString command=config->readEntry( "Execute" );
	
	kDebug(300) << command;
	
	if (!command.isEmpty()) {
// 	kDebug(300) << "executing command '" << command << "'";
		QHash<QChar,QString> subst;
		subst.insert( 'e', config->eventid );
		subst.insert( 'a', config->appname );
		subst.insert( 's', config->text );
		subst.insert( 'w', QString::number( (int)config->winId ));
		subst.insert( 'i', QString::number( id ));
		QString execLine = KMacroExpander::expandMacrosShellQuote( command, subst );
		if ( execLine.isEmpty() )
			execLine = command; // fallback
                KProcess proc;
		proc.setShellCommand(execLine.trimmed());
		if(!proc.startDetached())
			kDebug(300)<<"KNotify: Could not start process!";
	}
	
	finish( id );
}

#include "notifybyexecute.moc"
