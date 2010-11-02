/*
   Copyright (C) 2005-2009 by Olivier Goffart <ogoffart at kde.org>


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

#include "knotify.h"

// KDE headers
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <config-runtime.h>

#include "knotifyconfig.h"
#include "ksolidnotify.h"
#include "notifybysound.h"
#include "notifybypopup.h"
#include "notifybyexecute.h"
#include "notifybylogfile.h"
#include "notifybytaskbar.h"
#include "notifybyktts.h"



KNotify::KNotify( QObject *parent )
    : QObject( parent ),
    m_counter(0)
{
	loadConfig();
	(void)new KNotifyAdaptor(this);
	(void)new KSolidNotify(this);
	QDBusConnection::sessionBus().registerObject("/Notify", this, QDBusConnection::ExportAdaptors
	 /*|  QDBusConnection::ExportScriptableSlots |  QDBusConnection::ExportScriptableSignals*/ );
}

KNotify::~KNotify()
{
	qDeleteAll(m_notifications);
}


void KNotify::loadConfig()
{
	qDeleteAll(m_plugins);
	m_plugins.clear();
	addPlugin(new NotifyBySound(this));
	addPlugin(new NotifyByPopup(this));
	addPlugin(new NotifyByExecute(this));
	addPlugin(new NotifyByLogfile(this));
        //TODO reactivate on Mac/Win when KWindowSystem::demandAttention will implemented on this system.
#ifndef Q_WS_MAC
	addPlugin(new NotifyByTaskbar(this));
#endif
	addPlugin(new NotifyByKTTS(this));
}

void KNotify::addPlugin( KNotifyPlugin * p )
{
	m_plugins[p->optionName()]=p;
	connect(p,SIGNAL(finished( int )) , this , SLOT(slotPluginFinished( int ) ));
	connect(p,SIGNAL(actionInvoked( int , int )) , this , SIGNAL(notificationActivated( int , int ) ));
}



void KNotify::reconfigure()
{
	KGlobal::config()->reparseConfiguration();
	KNotifyConfig::reparseConfiguration();
	loadConfig();
}

void KNotify::closeNotification(int id)
{
	if(!m_notifications.contains(id))
		return;
	Event *e=m_notifications[id];

	kDebug() << e->id << " ref=" << e->ref;

	//this has to be called before  plugin->close or we will get double deletion because of slotPluginFinished
	m_notifications.remove(id);

	if(e->ref>0)
	{
		e->ref++;
		KNotifyPlugin *plugin;
		foreach(plugin , m_plugins)
		{
			plugin->close( id );
		}
	}
	notificationClosed(id);
	delete e;
}

int KNotify::event( const QString & event, const QString & appname, const ContextList & contexts, const QString & title, const QString & text, const KNotifyImage & image, const QStringList & actions, int timeout, WId winId )
{
	m_counter++;
	Event *e=new Event(appname , contexts , event );
	e->id = m_counter;
	e->ref = 1;

	e->config.title=title;
	e->config.text=text;
	e->config.actions=actions;
	e->config.image=image;
	e->config.timeout=timeout;
	e->config.winId=(WId)winId;

	m_notifications.insert(m_counter,e);
	emitEvent(e);

	e->ref--;
	kDebug() << e->id << " ref=" << e->ref;
	if(e->ref==0)
	{
		m_notifications.remove(e->id);
		delete e;
		return 0;
	}
	return m_counter;
}

void KNotify::update(int id, const QString &title, const QString &text, const KNotifyImage& image,  const QStringList& actions)
{
	if(!m_notifications.contains(id))
		return;

	Event *e=m_notifications[id];

	e->config.title=title;
	e->config.text=text;
	e->config.image = image;
	e->config.actions = actions;

	foreach(KNotifyPlugin *p, m_plugins)
	{
		p->update(id, &e->config);
	}
}
void KNotify::reemit(int id, const ContextList& contexts)
{
	if(!m_notifications.contains(id))
		return;
	Event *e=m_notifications[id];
	e->config.contexts=contexts;

	emitEvent(e);
}

void KNotify::emitEvent(Event *e)
{
	QString presentstring=e->config.readEntry("Action");
	QStringList presents=presentstring.split ('|');
	
	if (!e->config.contexts.isEmpty() && !presents.first().isEmpty()) 
	{
		//Check whether the present actions are absolute, relative or invalid
		bool relative = presents.first().startsWith('+') || presents.first().startsWith('-');
		bool valid = true;
		foreach (const QString & presentAction, presents)
			valid &=  ((presentAction.startsWith('+') || presentAction.startsWith('-')) == relative);
		if (!valid) 
		{
			kDebug() << "Context " << e->config.contexts << "present actions are invalid! Fallback to default present actions";
			Event defaultEvent = Event(e->config.appname, ContextList(), e->config.eventid);
			QString defaultPresentstring=defaultEvent.config.readEntry("Action");
			presents = defaultPresentstring.split ('|');
		} else if (relative) 
		{
			// Obtain the list of present actions without context
			Event noContextEvent = Event(e->config.appname, ContextList(), e->config.eventid);
			QString noContextPresentstring = noContextEvent.config.readEntry("Action");
			QSet<QString> noContextPresents = noContextPresentstring.split ('|').toSet();
			foreach (const QString & presentAction, presents)
			{
				if (presentAction.startsWith('+'))
					noContextPresents << presentAction.mid(1);
				else
					noContextPresents.remove(presentAction.mid(1));
			}
			presents = noContextPresents.toList();
		}
	}

	foreach(const QString & action , presents)
	{
		if(!m_plugins.contains(action))
			continue;
		KNotifyPlugin *p=m_plugins[action];
		e->ref++;
		p->notify(e->id,&e->config);
	}
}

void KNotify::slotPluginFinished( int id )
{
	if(!m_notifications.contains(id))
		return;
	Event *e=m_notifications[id];
	kDebug() << e->id << " ref=" << e->ref ;
	e->ref--;
	if(e->ref==0)
		closeNotification( id );
}

KNotifyAdaptor::KNotifyAdaptor(QObject *parent)
	: QDBusAbstractAdaptor(parent)
{
	setAutoRelaySignals(true);
}

void KNotifyAdaptor::reconfigure()
{
	static_cast<KNotify *>(parent())->reconfigure();
}

void KNotifyAdaptor::closeNotification(int id)
{
	static_cast<KNotify *>(parent())->closeNotification(id);
}

int KNotifyAdaptor::event(const QString &event, const QString &fromApp, const QVariantList& contexts,
						const QString &title, const QString &text, const QByteArray& image,  const QStringList& actions,
						int timeout, qlonglong winId)
//						  const QDBusMessage & , int _return )

{
	/* I'm not sure this is the right way to read a a(ss) type,  but it seems to work */
	ContextList contextlist;
	QString context_key;
	foreach( const QVariant &v , contexts)
	{
		/* this code doesn't work
		QVariantList vl=v.toList();
		if(vl.count() != 2)
		{
			kWarning() << "Bad structure passed as argument" ;
			continue;
		}
		contextlist << qMakePair(vl[0].toString() , vl[1].toString());*/
		QString s=v.toString();
		if(context_key.isEmpty())
			context_key=s;
		else
			contextlist << qMakePair(context_key , s);
	}

	return static_cast<KNotify *>(parent())->event(event, fromApp, contextlist, title, text, image, actions, timeout, WId(winId));
}

void KNotifyAdaptor::reemit(int id, const QVariantList& contexts)
{
	ContextList contextlist;
	QString context_key;
	foreach( const QVariant &v , contexts)
	{
		QString s=v.toString();
		if(context_key.isEmpty())
			context_key=s;
		else
			contextlist << qMakePair(context_key , s);
	}
	static_cast<KNotify *>(parent())->reemit(id, contextlist);
}


void KNotifyAdaptor::update(int id, const QString &title, const QString &text, const QByteArray& image,  const QStringList& actions )
{
	static_cast<KNotify *>(parent())->update(id, title, text, image, actions);
}

#include "knotify.moc"

// vim: sw=4 sts=4 ts=8 et


