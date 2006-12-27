/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "medianotifier.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QByteArray>
#include <QLinkedList>
#include <kjobuidelegate.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>
#include <kstandarddirs.h>

#include "notificationdialog.h"
#include "notifiersettings.h"
#include "notifieraction.h"
#include "mediamanagersettings.h"
#include "../mediamanageriface.h"

MediaNotifier::MediaNotifier() : KDEDModule()
{
	m_mediamanager = new OrgKdeMediaManagerInterface( "org.kde.kded", "/modules/mediamanager", QDBusConnection::sessionBus() );
	m_mediamanager->setParent( this );

	connect( m_mediamanager, SIGNAL(mediumAdded(QString,bool)),
		 SLOT(onMediumChange(QString,bool)));
	connect( m_mediamanager, SIGNAL(mediumChanged(QString,bool)),
		 SLOT(onMediumChange(QString,bool)));
}

MediaNotifier::~MediaNotifier()
{
}

void MediaNotifier::onMediumChange( const QString &name, bool allowNotification )
{
	kDebug() << "MediaNotifier::onMediumChange( " << name << ", "
	          << allowNotification << ")" << endl;

// Update user activity timestamp, otherwise the notification dialog will be shown
// in the background due to focus stealing prevention. Entering a new media can
// be seen as a kind of user activity after all. It'd be better to update the timestamp
// as soon as the media is entered, but it apparently takes some time to get here.
	kapp->updateUserTimestamp();

	KUrl url(  "system:/media/"+name );

	KIO::SimpleJob *job = KIO::stat( url, false );
	job->setUiDelegate(0);

	m_allowNotificationMap[job] = allowNotification;

	connect( job, SIGNAL( result(KJob*) ),
	         this, SLOT( slotStatResult(KJob*) ) );
}

void MediaNotifier::slotStatResult( KJob *job )
{
	bool allowNotification = m_allowNotificationMap[job];
	m_allowNotificationMap.remove( job );

	if ( job->error() != 0 ) return;

	KIO::StatJob *stat_job = static_cast<KIO::StatJob *>( job );

	KIO::UDSEntry entry = stat_job->statResult();
	KUrl url = stat_job->url();

	KFileItem medium( entry, url );

	if ( autostart( medium ) ) return;

	if ( allowNotification ) notify( medium );
}

bool MediaNotifier::autostart( const KFileItem &medium )
{
	QString mimetype = medium.mimetype();

	bool is_cdrom = mimetype.contains( "cd" ) || mimetype.contains( "dvd" );
	bool is_mounted = mimetype.endsWith( "_mounted" );

	// We autorun only on CD/DVD or removable disks (USB, Firewire)
	if ( !( is_cdrom && is_mounted )
	  && mimetype!="media/removable_mounted" )
	{
		return false;
	}


	// Here starts the 'Autostart Of Applications After Mount' implementation

	// The desktop environment MAY ignore Autostart files altogether
	// based on policy set by the user, system administrator or vendor.
	MediaManagerSettings::self()->readConfig();
	if ( !MediaManagerSettings::self()->autostartEnabled() )
	{
		return false;
	}

	// From now we're sure the medium is already mounted.
	// We can use the local path for stating, no need to use KIO here.
	bool local;
	QString path = medium.mostLocalUrl( local ).path(); // local is always true here...

	// When a new medium is mounted the root directory of the medium should
	// be checked for the following Autostart files in order of precedence:
	// .autorun, autorun, autorun.sh
	QStringList autorun_list;
	autorun_list << ".autorun" << "autorun" << "autorun.sh";

	QStringList::iterator it = autorun_list.begin();
	QStringList::iterator end = autorun_list.end();

	for ( ; it!=end; ++it )
	{
		if ( QFile::exists( path + '/' + *it ) )
		{
			return execAutorun( medium, path, *it );
		}
	}

	// When a new medium is mounted the root directory of the medium should
	// be checked for the following Autoopen files in order of precedence:
	// .autoopen, autoopen
	QStringList autoopen_list;
	autoopen_list << ".autoopen" << "autoopen";

	it = autoopen_list.begin();
	end = autoopen_list.end();

	for ( ; it!=end; ++it )
	{
		if ( QFile::exists( path + '/' + *it ) )
		{
			return execAutoopen( medium, path, *it );
		}
	}

	return false;
}

bool MediaNotifier::execAutorun( const KFileItem &medium, const QString &path,
                                 const QString &autorunFile )
{
	// The desktop environment MUST prompt the user for confirmation
	// before automatically starting an application.
	QString mediumType = medium.mimeTypePtr()->name();
	QString text = i18n( "An autorun file as been found on your '%1'."
	                     " Do you want to execute it?\n"
	                     "Note that executing a file on a medium may compromise"
	                     " your system's security", mediumType );
	QString caption = i18n( "Autorun - %1", medium.url().prettyUrl() );
	KGuiItem yes = KStandardGuiItem::yes();
	KGuiItem no = KStandardGuiItem::no();

	int answer = KMessageBox::warningYesNo( 0L, text, caption, yes, no,
	                                        QString(), KMessageBox::Notify | KMessageBox::Dangerous );

	if ( answer == KMessageBox::Yes )
	{
		// When an Autostart file has been detected and the user has
		// confirmed its execution the autostart file MUST be executed
		// with the current working directory ( CWD ) set to the root
		// directory of the medium.
		KProcess proc;
		proc << "sh" << autorunFile;
		proc.setWorkingDirectory( path );
		proc.start();
		proc.detach();
	}

	return true;
}

bool MediaNotifier::execAutoopen( const KFileItem &medium, const QString &path,
                                  const QString &autoopenFile )
{
	// An Autoopen file MUST contain a single relative path that points
	// to a non-executable file contained on the medium. [...]
	QFile file( path + '/' + autoopenFile );
	file.open( QIODevice::ReadOnly );
	QTextStream stream( &file );

	QString relative_path = stream.readLine().trimmed();

	// The relative path MUST NOT contain path components that
	// refer to a parent directory ( ../ )
	if ( relative_path.startsWith( "/" ) || relative_path.contains( "../" ) )
	{
		return false;
	}

	// The desktop environment MUST verify that the relative path points
	// to a file that is actually located on the medium [...]
	QString resolved_path
		= KStandardDirs::realFilePath( path + '/' + relative_path );

	if ( !resolved_path.startsWith( path ) )
	{
		return false;
	}


	QFile document( resolved_path );

	// TODO: What about FAT all files are executable...
	// If the relative path points to an executable file then the desktop
	// environment MUST NOT execute the file.
	if ( !document.exists() /*|| QFileInfo(document).isExecutable()*/ )
	{
		return false;
	}

	KUrl url = medium.url();
	url.addPath( relative_path );

	// The desktop environment MUST prompt the user for confirmation
	// before opening the file.
	QString mediumType = medium.mimeTypePtr()->name();
	QString filename = url.fileName();
	QString text = i18n( "An autoopen file as been found on your '%1'."
	                     " Do you want to open '%2'?\n"
	                     "Note that opening a file on a medium may compromise"
	                     " your system's security", mediumType, filename );
	QString caption = i18n( "Autoopen - %1", medium.url().prettyUrl() );
	KGuiItem yes = KStandardGuiItem::yes();
	KGuiItem no = KStandardGuiItem::no();

	int answer = KMessageBox::warningYesNo( 0L, text, caption, yes, no,
	                                        QString(), KMessageBox::Notify | KMessageBox::Dangerous );

	// TODO: Take case of the "UNLESS" part?
	// When an Autoopen file has been detected and the user has confirmed
	// that the file indicated in the Autoopen file should be opened then
	// the file indicated in the Autoopen file MUST be opened in the
	// application normally preferred by the user for files of its kind
	// UNLESS the user instructed otherwise.
	if ( answer == KMessageBox::Yes )
	{
		( void ) new KRun( url, 0L );
	}

	return true;
}

void MediaNotifier::notify( KFileItem &medium )
{
	kDebug() << "Notification triggered." << endl;

	NotifierSettings *settings = new NotifierSettings();

	if ( settings->autoActionForMimetype( medium.mimetype() )==0L )
	{
		QList<NotifierAction*> actions
			= settings->actionsForMimetype( medium.mimetype() );

		// If only one action remains, it's the "do nothing" action
		// no need to popup in this case.
		if ( actions.size()>1 )
		{
			NotificationDialog *dialog
				= new NotificationDialog( medium, settings );
			dialog->show();
		}
	}
	else
	{
		NotifierAction *action = settings->autoActionForMimetype( medium.mimetype() );
		action->execute( medium );
		delete settings;
	}
}

extern "C"
{
	KDE_EXPORT KDEDModule *create_medianotifier()
	{
		return new MediaNotifier();
	}
}

#include "medianotifier.moc"
