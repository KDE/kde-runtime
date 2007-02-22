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



#ifndef KNOTIFYPLUGIN_H
#define KNOTIFYPLUGIN_H

#include <QtCore/QObject>

class KNotifyConfig;


/**
 * abstract class for action
 * @author Olivier Goffart <ogoffart at kde.org>
*/
class KNotifyPlugin : public QObject
{ Q_OBJECT
	public:
		KNotifyPlugin(QObject *parent=0l);
		virtual ~KNotifyPlugin();
		
		virtual QString optionName() =0;
		/**
                   notify or re-notify.

                   finished(int) MUST be emitted, even for re-notify
                */
		virtual void notify(int id , KNotifyConfig *config )=0;
		virtual void update(int /*id*/, KNotifyConfig * /*config*/) {}
		virtual void close(int id) { emit finished(id);}
	
	protected:
		void finish(int id) { emit finished(id); }
	
	Q_SIGNALS:
		void finished(int id);
		void actionInvoked(int id , int action);
};

#endif
