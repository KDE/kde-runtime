/*
   Copyright (C) 2005-2009 by Olivier Goffart <ogoffart at kde.org>
   Copyright (C) 2008 by Dmitry Suzdalev <dimsuz@gmail.com>

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

#include "notifybypopup.h"
#include "knotifyconfig.h"
#include "imageconverter.h"

#include <kdebug.h>
#include <kpassivepopup.h>
#include <kiconloader.h>
#include <kdialog.h>
#include <khbox.h>
#include <kvbox.h>

#include <QBuffer>
#include <QImage>
#include <QLabel>
#include <QTextDocument>
#include <QApplication>
#include <QDesktopWidget>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <kconfiggroup.h>

static const char dbusServiceName[] = "org.freedesktop.Notifications";
static const char dbusInterfaceName[] = "org.freedesktop.Notifications";
static const char dbusPath[] = "/org/freedesktop/Notifications";

NotifyByPopup::NotifyByPopup(QObject *parent) 
  : KNotifyPlugin(parent) , m_animationTimer(0), m_dbusServiceExists(false)
{
	QRect screen = QApplication::desktop()->availableGeometry();
	m_nextPosition = screen.top();

	// check if service already exists on plugin instantiation
	QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();
	m_dbusServiceExists = interface && interface->isServiceRegistered(dbusServiceName);

	if( m_dbusServiceExists )
		slotServiceOwnerChanged(dbusServiceName, QString(), "_"); //connect signals

	// to catch register/unregister events from service in runtime
	connect(interface, SIGNAL(serviceOwnerChanged(const QString&, const QString&, const QString&)),
			SLOT(slotServiceOwnerChanged(const QString&, const QString&, const QString&)));
}


NotifyByPopup::~NotifyByPopup()
{
	foreach(KPassivePopup *p,m_popups)
		p->deleteLater();
}

void NotifyByPopup::notify( int id, KNotifyConfig * config )
{
	kDebug(300) << id << "  active notifications:" << m_popups.keys() << m_idMap.keys();

	if(m_popups.contains(id) || m_idMap.contains(id))
	{
		kDebug(300) << "the popup is already shown";
		finish(id);
		return;
	}

	// if Notifications DBus service exists on bus,
	// it'll be used instead
	if(m_dbusServiceExists)
	{
		if(!sendNotificationDBus(id, 0, config))
			finish(id); //an error ocurred.
		return;
	}

	KPassivePopup *pop = new KPassivePopup( config->winId );
	m_popups[id]=pop;
	fillPopup(pop,id,config);
	QRect screen = QApplication::desktop()->availableGeometry();
	pop->setAutoDelete( true );
	connect(pop, SIGNAL(destroyed()) , this, SLOT(slotPopupDestroyed()) );

	// Default to 6 seconds if no timeout has been defined
	int timeout = config->timeout == -1 ? 6000 : config->timeout;
	pop->setTimeout(timeout);

	pop->adjustSize();
	pop->show(QPoint(screen.left() + screen.width()/2 - pop->width()/2  , m_nextPosition));
	m_nextPosition+=pop->height();
}

void NotifyByPopup::slotPopupDestroyed( )
{
	const QObject *s=sender();
	if(!s)
		return;
	QMap<int,KPassivePopup*>::iterator it;
	for(it=m_popups.begin() ; it!=m_popups.end(); ++it   )
	{
		QObject *o=it.value();
		if(o && o == s)
		{
			finish(it.key());
			m_popups.remove(it.key());
			break;
		}
	}

	//relocate popup
	if(!m_animationTimer)
		m_animationTimer = startTimer(10);
}

void NotifyByPopup::timerEvent(QTimerEvent * event)
{
	if(event->timerId() != m_animationTimer)
		return KNotifyPlugin::timerEvent(event);
	
	bool cont=false;
	QRect screen = QApplication::desktop()->availableGeometry();
	m_nextPosition = screen.top();
	foreach(KPassivePopup *pop,m_popups)
	{
		int posy=pop->pos().y();
		if(posy > m_nextPosition)
		{
			posy=qMax(posy-5,m_nextPosition);
			m_nextPosition = posy + pop->height();
			cont = cont || posy != m_nextPosition;
			pop->move(pop->pos().x(),posy);
		}
		else
			m_nextPosition += pop->height();
	}
	if(!cont)
	{
		killTimer(m_animationTimer);
		m_animationTimer = 0;
	}
}

void NotifyByPopup::slotLinkClicked( const QString &adr )
{
	unsigned int id=adr.section('/' , 0 , 0).toUInt();
	unsigned int action=adr.section('/' , 1 , 1).toUInt();

//	kDebug(300) << id << " " << action;
        
	if(id==0 || action==0)
		return;
		
	emit actionInvoked(id,action);
}

void NotifyByPopup::close( int id )
{
	delete m_popups.take(id);

	if( m_dbusServiceExists)
	{
		closeNotificationDBus(id);
	}
}

void NotifyByPopup::update(int id, KNotifyConfig * config)
{
	if (m_popups.contains(id))
	{
		KPassivePopup *p=m_popups[id];
		fillPopup(p, id, config);
		return;
	}

	// if Notifications DBus service exists on bus,
	// it'll be used instead
	if( m_dbusServiceExists)
	{
		sendNotificationDBus(id, id, config);
		return;
	}
}

void NotifyByPopup::fillPopup(KPassivePopup *pop,int id,KNotifyConfig * config)
{
	QString appCaption, iconName;
	getAppCaptionAndIconName(config, &appCaption, &iconName);

	KIconLoader iconLoader(iconName);
	QPixmap appIcon = iconLoader.loadIcon( iconName, KIconLoader::Small );

	KVBox *vb = pop->standardView( appCaption , config->image.isNull() ? config->text : QString() , appIcon );
	KVBox *vb2 = vb;

	if(!config->image.isNull())
	{
		QPixmap pix = QPixmap::fromImage(config->image.toImage());
		KHBox *hb = new KHBox(vb);
		hb->setSpacing(KDialog::spacingHint());
		QLabel *pil=new QLabel(hb);
		pil->setPixmap( pix );
		pil->setScaledContents(true);
		if(pix.height() > 80 && pix.height() > pix.width() )
		{
			pil->setMaximumHeight(80);
			pil->setMaximumWidth(80*pix.width()/pix.height());
		}
		else if(pix.width() > 80 && pix.height() <= pix.width())
		{
			pil->setMaximumWidth(80);
			pil->setMaximumHeight(80*pix.height()/pix.width());
		}
		vb=new KVBox(hb);
		QLabel *msg = new QLabel( config->text, vb );
		msg->setAlignment( Qt::AlignLeft );
	}


	if ( !config->actions.isEmpty() )
	{
		QString linkCode=QString::fromLatin1("<p align=\"right\">");
		int i=0;
		foreach ( const QString & it , config->actions ) 
		{
			i++;
			linkCode+=QString::fromLatin1("&nbsp;<a href=\"%1/%2\">%3</a> ").arg( id ).arg( i ).arg( Qt::escape(it) );
		}
		linkCode+=QString::fromLatin1("</p>");
		QLabel *link = new QLabel(linkCode , vb );
		link->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		link->setOpenExternalLinks(false);
		//link->setAlignment( AlignRight );
		QObject::connect(link, SIGNAL(linkActivated(const QString &)), this, SLOT(slotLinkClicked(const QString& ) ) );
		QObject::connect(link, SIGNAL(linkActivated(const QString &)), pop, SLOT(hide()));
	}

	pop->setView( vb2 );
}

void NotifyByPopup::slotServiceOwnerChanged( const QString & serviceName,
		const QString & oldOwner, const QString & newOwner )
{
	if(serviceName == dbusServiceName)
	{
		kDebug(300) << serviceName << oldOwner << newOwner;
		// tell KNotify that all existing notifications which it sent
		// to DBus had been closed
		foreach (int id, m_idMap.keys())
			finished(id);
		m_idMap.clear();

		if(newOwner.isEmpty())
		{
			m_dbusServiceExists = false;
		}
		else if(oldOwner.isEmpty())
		{
			m_dbusServiceExists = true;

			// connect to action invocation signals
			bool connected = QDBusConnection::sessionBus().connect(QString(), // from any service
					dbusPath,
					dbusInterfaceName,
					"ActionInvoked",
					this,
					SLOT(slotDBusNotificationActionInvoked(uint,const QString&)));
			if (!connected) {
				kWarning(300) << "warning: failed to connect to ActionInvoked dbus signal";
			}

			connected = QDBusConnection::sessionBus().connect(QString(), // from any service
					dbusPath,
					dbusInterfaceName,
					"NotificationClosed",
					this,
					SLOT(slotDBusNotificationClosed(uint,uint)));
			if (!connected) {
				kWarning(300) << "warning: failed to connect to NotificationClosed dbus signal";
			}
		}
	}
}

void NotifyByPopup::slotDBusNotificationActionInvoked(uint dbus_id, const QString& actKey)
{
	// find out knotify id
	int id = m_idMap.key(dbus_id, 0);
	if (id == 0) {
		kDebug(300) << "failed to find knotify id for dbus_id" << dbus_id;
		return;
	}
	kDebug(300) << "action" << actKey << "invoked for notification " << id;
	// emulate link clicking
	slotLinkClicked( QString("%1/%2").arg(id).arg(actKey) );
    // now close notification - similar to popup behaviour
    // (popups are hidden after link activation - see 'connects' of linkActivated signal above)
    closeNotificationDBus(id);
}

void NotifyByPopup::slotDBusNotificationClosed(uint dbus_id, uint reason)
{
	Q_UNUSED(reason)
	// find out knotify id
	int id = m_idMap.key(dbus_id, 0);
	kDebug(300) << dbus_id << "  -> " << id;
	if (id == 0) {
		kDebug(300) << "failed to find knotify id for dbus_id" << dbus_id;
		return;
	}
	// tell KNotify that this notification has been closed
	m_idMap.remove(id);
	finished(id);
}

void NotifyByPopup::getAppCaptionAndIconName(KNotifyConfig *config, QString *appCaption, QString *iconName)
{
	KConfigGroup globalgroup(&(*config->eventsfile), "Global");
	*appCaption = globalgroup.readEntry("Name", globalgroup.readEntry("Comment", config->appname));
	*iconName = globalgroup.readEntry("IconName", config->appname);
}

bool NotifyByPopup::sendNotificationDBus(int id, int replacesId, KNotifyConfig* config)
{
	// figure out dbus id to replace if needed
	uint dbus_replaces_id = 0;
	if (replacesId != 0 ) {
		dbus_replaces_id = m_idMap.value(replacesId, 0);
		if (!dbus_replaces_id)
			return false;  //the popup has been closed, there is nothing to replace.
	}

	QDBusMessage m = QDBusMessage::createMethodCall( dbusServiceName, dbusPath, dbusInterfaceName, "Notify" );

	QList<QVariant> args;

	QString appCaption, iconName;
	getAppCaptionAndIconName(config, &appCaption, &iconName);

	args.append( appCaption ); // app_name
	args.append( dbus_replaces_id ); // replaces_id
	args.append( iconName ); // app_icon
	args.append( config->title.isEmpty() ? appCaption : config->title ); // summary
	args.append( config->text ); // body
	// galago spec defines action list to be list like
	// (act_id1, action1, act_id2, action2, ...)
	//
	// assign id's to actions like it's done in fillPopup() method
	// (i.e. starting from 1)
	QStringList actionList;
	int actId = 0;
	foreach (const QString& actName, config->actions) {
		actId++;
		actionList.append(QString::number(actId));
		actionList.append(actName);
	}

	args.append( actionList ); // actions

	QVariantMap map;
	// let's see if we've got an image, and store the image in the hints map
	if (!config->image.isNull()) {
		QImage image = config->image.toImage();
		map["image_data"] = ImageConverter::variantForImage(image);
	}

	args.append( map ); // hints
	args.append( config->timeout ); // expire timout

	m.setArguments( args );
	QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
	if(replyMsg.type() == QDBusMessage::ReplyMessage) {
		if (!replyMsg.arguments().isEmpty()) {
			uint dbus_id = replyMsg.arguments().at(0).toUInt();
			if (dbus_id == 0)
			{
				kDebug(300) << "error: dbus_id is null";
				return false;
			}
			if (dbus_replaces_id && dbus_id == dbus_replaces_id) 
				return true;
#if 1
			int oldId = m_idMap.key(dbus_id, 0);
			if (oldId != 0) {
				kWarning(300) << "Received twice the same id "<< dbus_id << "( previous notification: " << oldId << ")";
				m_idMap.remove(oldId);
				finish(oldId);
			}
#endif
			m_idMap.insert(id, dbus_id);
			kDebug(300) << "mapping knotify id to dbus id:"<< id << "=>" << dbus_id;

			return true;
		} else {
			kDebug(300) << "error: received reply with no arguments";
		}
	} else if (replyMsg.type() == QDBusMessage::ErrorMessage) {
		kDebug(300) << "error: failed to send dbus message";
	} else {
		kDebug(300) << "unexpected reply type";
	}
	return false;
}

void NotifyByPopup::closeNotificationDBus(int id)
{
	uint dbus_id = m_idMap.take(id);
	if (dbus_id == 0) {
		kDebug(300) << "not found dbus id to close" << id;
	    return;
	}

	QDBusMessage m = QDBusMessage::createMethodCall( dbusServiceName, dbusPath, 
            dbusInterfaceName, "CloseNotification" );
	QList<QVariant> args;
	args.append( dbus_id );
	m.setArguments( args );
	bool queued = QDBusConnection::sessionBus().send(m);
	if(!queued)
	{
		kDebug(300) << "warning: failed to queue dbus message";
	}
	
}

#include "notifybypopup.moc"
