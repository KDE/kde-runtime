/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qlayout.h>

#include <kgenericfactory.h>
#include <kaboutdata.h>

#include "icons.h"
#include "iconthemes.h"
#include "main.h"

/**** DLL Interface ****/
typedef KGenericFactory<IconModule, QWidget> IconsFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_icons, IconsFactory("kcmicons") );

/**** IconModule ****/

IconModule::IconModule(QWidget *parent, const char *name, const QStringList &)
  : KCModule(IconsFactory::instance(), parent, name)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  tab1 = new IconThemesConfig(this, "themes");
  tab->addTab(tab1, i18n("&Theme"));
  connect(tab1, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  tab2 = new KIconConfig(this, "effects");
  tab->addTab(tab2, i18n("&Advanced"));
  connect(tab2, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));
}


void IconModule::load()
{
  tab1->load();
  tab2->load();
}


void IconModule::save()
{
  tab1->save();
  tab2->save();
}


void IconModule::defaults()
{
  tab1->defaults();
  tab2->defaults();
}


void IconModule::moduleChanged(bool state)
{
  emit changed(state);
}

QString IconModule::quickHelp() const
{
  return i18n("<h1>Icons</h1>\n"
    "This module allows you to choose the icons for your desktop.\n"
    "You can also specify effects that should be applied to the icons.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}


const KAboutData *IconModule::aboutData() const
{
    static KAboutData* ab = 0;

    if (!ab)
    {
        ab = new KAboutData("kcmicons", I18N_NOOP("Icons"), "3.0",
                            I18N_NOOP("Icons Control Panel Module"),
                            KAboutData::License_GPL,
                            I18N_NOOP("(c) 2000-2003 Geert Jansen"), 0, 0);
        ab->addAuthor("Geert Jansen", 0, "jansen@kde.org");
        ab->addAuthor("Antonio Larrosa Jimenez", 0, "larrosa@kde.org");
        ab->addCredit("Torsten Rahn", 0, "torsten@kde.org");
    }

    return ab;
}


#include "main.moc"
