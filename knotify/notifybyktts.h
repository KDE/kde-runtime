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



#ifndef NOTIFYBYKTTS_H
#define NOTIFYBYKTTS_H

#include "knotifyplugin.h"


#include <QDBusInterface>

class NotifyByKTTS : public KNotifyPlugin
{ Q_OBJECT
	public:
		NotifyByKTTS(QObject *parent=0l);
		virtual ~NotifyByKTTS();
		
		virtual QString optionName() { return "KTTS"; }
		virtual void notify(int id , KNotifyConfig *config);
	private:
		QDBusInterface kspeech;
};

#endif
