/*
 *   kdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "configdlg.h"
#include "confgeneral.h"
#include "conffax.h"
#include "confsystem.h"
#include "conffilters.h"

#include <kvbox.h>
#include <klocale.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kpagewidgetmodel.h>

ConfigDlg::ConfigDlg(QWidget *parent, const char *name)
: KPageDialog( parent )
{
  setFaceType( List );
  setCaption( i18n("Configuration") );
  setButtons( Ok|Cancel );
  setObjectName( name );
  setModal( true );

	KVBox	*page1 = new KVBox( this );
	m_general = new ConfGeneral(page1, "Personal");

  KPageWidgetItem *item = addPage( page1, i18n("Personal") );
  item->setHeader( i18n("Personal Settings") );
  item->setIcon( KIcon("kdmconfig") );

	KVBox	*page2 = new KVBox( this );
	m_fax = new ConfFax(page2, "Fax");

  item = addPage( page2, i18n("Page setup") );
  item->setHeader( i18n("Page Setup") );
  item->setIcon( KIcon("edit-copy") );

	KVBox	*page3 = new KVBox( this );
	m_system = new ConfSystem(page3, "System");

  item = addPage( page3, i18n("System") );
  item->setHeader( i18n("Fax System Selection") );
  item->setIcon( KIcon("kdeprintfax") );

	KVBox	*page4 = new KVBox( this );
	m_filters = new ConfFilters(page4, "Filters");

  item = addPage( page4, i18n("Filters") );
  item->setHeader( i18n("Filters Configuration") );
  item->setIcon( KIcon("search-filter") );

	resize(450, 300);
}

void ConfigDlg::load()
{
	m_general->load();
	m_fax->load();
	m_system->load();
	m_filters->load();
}

void ConfigDlg::save()
{
	m_general->save();
	m_fax->save();
	m_system->save();
	m_filters->save();
}

bool ConfigDlg::configure(QWidget *parent)
{
	ConfigDlg	dlg(parent);
	dlg.load();
	if (dlg.exec())
	{
		dlg.save();
		return true;
	}
	return false;
}
