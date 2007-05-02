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

#include "kcmprintmgr.h"
#include "kdeprint/kmmainview.h"

#include <QLayout>

#include <kgenericfactory.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <klocale.h>

typedef KGenericFactory<KCMPrintMgr, QWidget> KPrintMgrFactory;
K_EXPORT_COMPONENT_FACTORY( printmgr, KPrintMgrFactory("kcmprintmgr") )

KCMPrintMgr::KCMPrintMgr(QWidget *parent, const QStringList &)
: KCModule(KPrintMgrFactory::componentData(), parent)
{
	setButtons(KCModule::Buttons());
	setRootOnlyMessage( i18n(
		"Print management as normal user\n"
		"Some print management operations may need administrator privileges. Use the\n"
		"\"Administrator Mode\" button below to start this print management tool with\n"
		"administrator privileges.") );
	setUseRootOnlyMessage(false);

	m_mainview = new KMMainView(this);
	m_mainview->setObjectName("MainView");

	QVBoxLayout	*main_ = new QVBoxLayout(this);
	main_->setMargin( 0 );
	main_->setSpacing( 0 );
	main_->addWidget(m_mainview);
	main_->activate();
	
	KAboutData *about =
	  new KAboutData(I18N_NOOP("kcmprintmgr"), I18N_NOOP("KDE Printing Management"),
                  0, 0, KAboutData::License_GPL,
                  I18N_NOOP("(c) 2000 - 2002 Michael Goffioul"));
	about->addAuthor("Michael Goffioul", 0, "kdeprint@swing.be");
	setAboutData(about);
}

QString KCMPrintMgr::quickHelp() const
{
	return i18n("<h1>Printers</h1>The KDE printing manager is part of KDEPrint which "
               "is the interface to the real print subsystem of your Operating System (OS). "
               "Although it does add some additional functionality of its own to those subsystems, "
               "KDEPrint depends on them for its functionality. Spooling and filtering tasks, especially, "
               "are still done by your print subsystem, or the administrative tasks (adding or "
               "modifying printers, setting access rights, etc.)<br/> "
               "What print features KDEPrint supports is therefore heavily dependent on your chosen print "
               "subsystem. For the best support in modern printing, the KDE Printing Team recommends "
               "a CUPS based printing system.");
}

KCMPrintMgr::~KCMPrintMgr()
{
}
