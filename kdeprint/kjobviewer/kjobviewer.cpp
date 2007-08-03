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

#include "kjobviewer.h"
#include <kdeprint/kmjobviewer.h>
#include <kdeprint/kmtimer.h>
#include <kdeprint/kmmanager.h>

#include <stdlib.h>
#include <QEvent>
#include <QPixmap>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <ksystemtrayicon.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kwindowsystem.h>
#include <QHash>
#include <QMouseEvent>
#include <kstartupinfo.h>

class JobTray : public KSystemTrayIcon
{
public:
	JobTray(KJobViewerApp *parent)
		: KSystemTrayIcon(0), m_app(parent) { connect( this, SIGNAL( quitSelected() ), kapp, SLOT( quit() ) ); }
protected:
	void mousePressEvent(QMouseEvent*);
private:
	KJobViewerApp	*m_app;
};

void KJobViewerApp::trayActivated( QSystemTrayIcon::ActivationReason reason )
{
/* not sure what to do with this for other platforms */
#ifdef Q_WS_X11
    if ( reason != QSystemTrayIcon::Trigger )
        return;

    if (m_views.count() > 0)
    {
        QMenu	menu;
        Q3DictIterator<KMJobViewer>	it(m_views);
        QHash<QAction*, KMJobViewer*>	hash;
        KMJobViewer *first = 0;
        for (; it.current(); ++it)
        {
            QAction *act = menu.addAction(QIcon(KWindowSystem::icon(it.current()->winId(), 16, 16)), it.currentKey());
            if (it.current()->isVisible())
                act->setChecked( true );
            hash.insert( act, it.current() );
            if ( !first )
                first = it.current();
        }

        if (hash.count() == 1 && first)
        {
            // special case, old behavior
            if (first->isVisible())
                first->hide();
            else
                first->show();
        }
        else
        {
            QAction *act = menu.exec();
            if (act)
            {
                KMJobViewer	*view = hash[act];
                if (view->isVisible())
                    KWindowSystem::activateWindow(view->winId());
                else
                    view->show();
            }
        }
    }
#endif
}

//-------------------------------------------------------------

KJobViewerApp::KJobViewerApp() : KUniqueApplication()
{
	m_views.setAutoDelete(true);
	m_tray = 0;
	m_timer = 0;
}

KJobViewerApp::~KJobViewerApp()
{
    delete m_tray;
}

int KJobViewerApp::newInstance()
{
	initialize();
	return 0;
}

void KJobViewerApp::initialize()
{
	KCmdLineArgs	*args = KCmdLineArgs::parsedArgs();
	bool 	showIt = args->isSet("show");
	bool	all = args->isSet("all");
	QString	prname = args->getOption("d");
	KMJobViewer	*view(0);

	if (!m_timer)
	{
		m_timer = KMTimer::self();
		connect(m_timer,SIGNAL(timeout()),SLOT(slotTimer()));
	}

	if (prname.isEmpty() && all)
		prname = i18n("All Printers");

        if (prname.isEmpty()) {
            KMPrinter *prt =  KMManager::self()->defaultPrinter();
            if (prt)
                prname = prt->printerName();
            else {
                KMessageBox::error(0, i18n("There is no default printer. Start with --all to see all printers."), i18n("Print Error"));
                exit(1);
                return;
            }
        }

        if (!m_tray)
        {
            m_tray = new KSystemTrayIcon(QLatin1String( "document-print" ), 0);
            connect( m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT( trayActivated(QSystemTrayIcon::ActivationReason) ) );
        }

        view = m_views.find(prname);
        if (!view)
        {
            kDebug() << "creating new view: " << prname;
            view = new KMJobViewer();
            connect(view, SIGNAL(jobsShown(KMJobViewer*,bool)), SLOT(slotJobsShown(KMJobViewer*,bool)));
            connect(view, SIGNAL(printerChanged(KMJobViewer*,const QString&)), SLOT(slotPrinterChanged(KMJobViewer*,const QString&)));
            connect(view, SIGNAL(refreshClicked()), SLOT(slotTimer()));
            connect(view, SIGNAL(viewerDestroyed(KMJobViewer*)), SLOT(slotViewerDestroyed(KMJobViewer*)));
            m_views.insert(prname, view);
        }

        if (showIt)
        {
            view->show();
            m_tray->show();
        }
        view->setPrinter(prname);

	//m_timer->release(true);
	m_timer->delay(10);
}

void KJobViewerApp::slotJobsShown(KMJobViewer *view, bool hasJobs)
{
	if (!hasJobs && !view->isVisible() && !view->isSticky())
	{
		kDebug() << "removing view: " << view->printer();
		// the window is hidden and doesn't have any job shown -> destroy it
		// closing won't have any effect as the KMJobViewer overload closeEvent()
		m_views.remove(view->printer());
	}

	if (m_views.count() > 0)
	{
		if (!m_tray->isVisible())
			m_tray->show();
	}
	else {
#ifdef Q_WS_X11
		KStartupInfo::appStarted();
#endif
		kapp->quit();
	}
}

void KJobViewerApp::slotTimer()
{
	// Update printer list
	KMManager::self()->printerList(true);

	// Refresh views. The first time, job list is reloaded,
	// other views will simply use reloaded job list
	bool	trigger(true);
	Q3DictIterator<KMJobViewer>	it(m_views);
	for (; it.current(); ++it, trigger=false)
		it.current()->refresh(trigger);
}

void KJobViewerApp::slotPrinterChanged(KMJobViewer *view, const QString& prname)
{
	KMJobViewer	*other = m_views.find(prname);
	if (other)
	{
		if (other->isVisible())
			KWindowSystem::activateWindow(other->winId());
		else
			other->show();
	}
	else
	{
		m_views.take(view->printer());
		m_views.insert(prname, view);
		view->setPrinter(prname);
	}
}

void KJobViewerApp::reload()
{
	// trigger delayed refresh in all views
	m_timer->delay(10);
}

void KJobViewerApp::slotViewerDestroyed(KMJobViewer *view)
{
	if (view)
		m_views.take(view->printer());
	if (m_views.count() == 0)
		kapp->quit();
}

#include "kjobviewer.moc"
