/*
   Copyright (C) 2005-2006 by Olivier Goffart <ogoffart at kde.org>
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

#ifndef NOTIFYBYPOPUP_H
#define NOTIFYBYPOPUP_H

#include "knotifyplugin.h"
#include <QMap>
#include <QHash>

class KPassivePopup;

class NotifyByPopup : public KNotifyPlugin
{ Q_OBJECT
	public:
		NotifyByPopup(QObject *parent=0l);
		virtual ~NotifyByPopup();
		
		virtual QString optionName() { return "Popup"; }
		virtual void notify(int id , KNotifyConfig *config);
		virtual void close( int id );
		virtual void update(int id, KNotifyConfig *config);
	private:
		QMap<int, KPassivePopup * > m_popups;
		// the y coordinate of the next position popup should appears
		int m_nextPosition;
		int m_animationTimer;
		void fillPopup(KPassivePopup *,int id,KNotifyConfig *config);
		/**
		 * Sends notification to DBus "/Notifications" interface.
		 * @param id knotify-sid identifier of notification
		 * @param replacesId knotify-side notification identifier. If not 0, will
		 * request DBus service to replace existing notification with data in config
		 * @param config notification data
		 * @return true for success or false if there was an error.
		 */
		bool sendNotificationDBus(int id, int replacesId, KNotifyConfig* config);
		/**
		 * Sends request to close Notification with id to DBus "/Notification" interface
		 *  @param id knotify-side notification ID to close
		 */
		void closeNotificationDBus(int id);
		/**
		 * Specifies if DBus Notifications interface exists on session bus
		 */
		bool m_dbusServiceExists;
		/**
		 * Find the caption and the icon name of the application
		 */
		void getAppCaptionAndIconName(KNotifyConfig *config, QString *appCaption, QString *iconName);
		
	protected:
		void timerEvent(QTimerEvent *event);

	private Q_SLOTS:
		void slotPopupDestroyed();
		void slotLinkClicked(const QString & );
		// slot to catch appearance or dissapearance of Notifications DBus service
		void slotServiceOwnerChanged(const QString &, const QString &, const QString &);
		// slot which gets called when DBus signals that some notification action was invoked
		void slotDBusNotificationActionInvoked(uint, const QString&);
		// slot which gets called when DBus signals that some notification was closed
		void slotDBusNotificationClosed(uint, uint);

        private:
		/**
		 * Maps knotify notification IDs to DBus notifications IDs
		 */
		QHash<int,uint> m_idMap;
};

#endif
