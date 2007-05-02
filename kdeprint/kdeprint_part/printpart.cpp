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

#include "printpart.h"

#include <kdeprint/kmmainview.h>
#include <kdeprint/kiconselectaction.h>
#include <kaction.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kiconloader.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kparts/genericfactory.h>
#include <QWidget>

typedef KParts::GenericFactory<PrintPart> PrintPartFactory;
K_EXPORT_COMPONENT_FACTORY( libkdeprint_part, PrintPartFactory )

PrintPart::PrintPart(QWidget *parentWidget,
	             QObject *parent,
		     const QStringList & /*args*/ )
: KParts::ReadOnlyPart(parent)
{
        setComponentData(PrintPartFactory::componentData());
        m_iconLoader = new KIconLoader(componentData().componentName(), componentData().dirs());
        m_iconLoader->addAppDir("kdeprint");
	m_extension = new PrintPartExtension(this);

	m_view = new KMMainView(parentWidget, actionCollection());
	m_view->setObjectName("MainView");
	m_view->setFocusPolicy(Qt::ClickFocus);
	m_view->enableToolbar(false);
	setWidget(m_view);

	initActions();
}

PrintPart::~PrintPart()
{
	delete m_iconLoader;
}

KAboutData *PrintPart::createAboutData()
{
	return new KAboutData(I18N_NOOP("kdeprint_part"), I18N_NOOP("A Konqueror Plugin for Print Management"), "0.1");
}

bool PrintPart::openFile()
{
	return true;
}

void PrintPart::initActions()
{
	setXMLFile("kdeprint_part.rc");
}

PrintPartExtension::PrintPartExtension(PrintPart *parent)
: KParts::BrowserExtension(parent)
{
   setObjectName("PrintPartExtension");
}

PrintPartExtension::~PrintPartExtension()
{
}

#include "printpart.moc"
