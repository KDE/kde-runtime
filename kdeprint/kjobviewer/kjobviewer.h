/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#ifndef KJOBVIEWER_H
#define KJOBVIEWER_H

#include <kuniqueapplication.h>
#include <kdeprint/kpreloadobject.h>
#include <Qt3Support/Q3Dict>

class KMJobViewer;
class KMTimer;
class KSystemTrayIcon;

class KJobViewerApp : public KUniqueApplication, public KPReloadObject
{
	Q_OBJECT
	friend class JobTray;
public:
	KJobViewerApp();
	virtual ~KJobViewerApp();

	virtual int newInstance();

protected:
	void initialize();
	void reload();

protected Q_SLOTS:
	void slotJobsShown(KMJobViewer*, bool);
	void slotTimer();
	void slotPrinterChanged(KMJobViewer*, const QString&);
	void slotViewerDestroyed(KMJobViewer*);
	void trayActivated( int reason );

private:
	Q3Dict<KMJobViewer>	m_views;
	KSystemTrayIcon *m_tray;
	KMTimer	*m_timer;
};

#endif
