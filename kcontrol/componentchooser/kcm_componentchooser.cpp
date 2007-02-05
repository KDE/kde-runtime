/***************************************************************************
                          kcm_componentchooser.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/

#include <QLayout>
//Added by qt3to4:
#include <QVBoxLayout>
#include <klocale.h>

#include <kaboutdata.h>
#include <kglobal.h>
#include <kcomponentdata.h>

#include "kcm_componentchooser.h"
#include <kgenericfactory.h>
#include "kcm_componentchooser.moc"

typedef KGenericFactory<KCMComponentChooser> KCMComponentChooserFactory;
K_EXPORT_COMPONENT_FACTORY(componentchooser, KCMComponentChooserFactory("kcmcomponentchooser"))

KCMComponentChooser::KCMComponentChooser(QWidget *parent, const QStringList &):
	KCModule(KCMComponentChooserFactory::componentData(), parent) {

	(new QVBoxLayout(this))->setAutoAdd(true);
	m_chooser=new ComponentChooser(this);
	connect(m_chooser,SIGNAL(changed(bool)),this,SIGNAL(changed(bool)));
	setButtons( Help | Apply );

	KAboutData *about =
	new KAboutData(I18N_NOOP("kcmcomponentchooser"), I18N_NOOP("Component Chooser"),
			0, 0, KAboutData::License_GPL,
			I18N_NOOP("(c), 2002 Joseph Wenninger"));

	about->addAuthor("Joseph Wenninger", 0 , "jowenn@kde.org");
	setAboutData( about );

}

void KCMComponentChooser::load(){
	m_chooser->load();
}

void KCMComponentChooser::save(){
	m_chooser->save();
}

void KCMComponentChooser::defaults(){
	m_chooser->restoreDefault();
}
