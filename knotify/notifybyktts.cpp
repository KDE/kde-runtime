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
#include <QtDBus/QtDBus>
#include <QHash>
#include <ktoolinvocation.h>
#include <kmessagebox.h>
#include <kmacroexpander.h>
#include <klocale.h>
#include "knotifyconfig.h"

NotifyByKTTS::NotifyByKTTS(QObject *parent) : KNotifyPlugin(parent),kspeech("org.kde.kttsd", "/KSpeech", "org.kde.KSpeech"), tryToStartKttsd( false )
{
    if( kspeech.isValid())
	kspeech.call("setApplicationName", "KNotify");
}


NotifyByKTTS::~NotifyByKTTS()
{
}

void NotifyByKTTS::notify( int id, KNotifyConfig * config )
{
        if( !kspeech.isValid())
        {
            if (  tryToStartKttsd ) //don't try to restart it all the time.
                return;
            // If KTTSD not running, start it.
            if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"))
            {
                QString error;
                if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error))
                {
                    KMessageBox::error(0, i18n( "Starting KTTSD Failed"), error );
                    tryToStartKttsd = true;
                    return;
                }
                kspeech.call("setApplicationName", "KNotify");
            }
        }

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
