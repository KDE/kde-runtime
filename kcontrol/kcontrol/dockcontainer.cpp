/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <qapplication.h>
#include <qlabel.h>

#include <kmessagebox.h>
#include <kglobal.h>
#include <klocale.h>
#include <kcmodule.h>
#include <kdebug.h>

#include "dockcontainer.h"
#include "dockcontainer.moc"

#include "global.h"
#include "modules.h"
#include "proxywidget.h"

DockContainer::DockContainer(QWidget *parent, const char *name)
  : QWidget(parent, name)
  , _basew(0L)
  , _module(0L)
{
  _busy = new QLabel(i18n("<big>Loading ...</big>"), this);
  _busy->setAlignment(AlignCenter);
  _busy->setTextFormat(RichText);
  _busy->setGeometry(0,0, width(), height());
  _busy->hide();
  _rootOnly = new QLabel(i18n("<big>You need super user privileges to run this control module.</big><br>"
							  "Click on the \"Run as root\" button below."), this);
  _rootOnly->setAlignment(AlignCenter);
  _rootOnly->setTextFormat(RichText);
  _rootOnly->setGeometry(0,0, width(), height());
  _rootOnly->hide();
}

DockContainer::~DockContainer()
{
  deleteModule();
}

void DockContainer::setBaseWidget(QWidget *widget)
{
  if (!widget) return;

  _basew = widget;
  _basew->reparent(this, 0 , QPoint(0,0), true);
  resize(_basew->sizeHint());
  emit newModule(widget->caption(), "", "");
}

void DockContainer::dockModule(ConfigModule *module)
{
  if (!module) return;
  if (_module == module) return;

  if (_module && _module->isChanged())
    {	  	
      int res = KMessageBox::warningYesNo(this,i18n("There are unsaved changes in the "
                                                 "active module.\n"
                                                 "Do you want to apply the changes "
                                                 "before running\n"
                                                 "the new module or forget the changes?"),
                                          i18n("Unsaved changes"),
                                          i18n("&Apply"),
                                          i18n("&Forget"));
      if (res == KMessageBox::Yes)
        _module->module()->applyClicked();
    }

  deleteModule();

  if (module->needsRootPrivileges() && !KCGlobal::root() && !module->hasReadOnlyMode())
	{
	  _rootOnly->raise();
	  _rootOnly->show();
	  _rootOnly->repaint();
	  return;
	}

  _busy->raise();
  _busy->show();
  _busy->repaint();
  QApplication::setOverrideCursor( waitCursor );

  ProxyWidget *widget = module->module();

  if (widget)
    {
	  _module = module;
      connect(_module, SIGNAL(childClosed()),
              this, SLOT(removeModule()));
      
      //####### this doesn't work anymore, what was it supposed to do? The signal is gone.
//       connect(_module, SIGNAL(quickHelpChanged()),
//               this, SLOT(quickHelpChanged()));

      widget->reparent(this, 0 , QPoint(0,0), false);
      widget->resize(size());

      emit newModule(widget->caption(), module->docPath(), widget->quickHelp());
      QApplication::restoreOverrideCursor();
    }
  else
    QApplication::restoreOverrideCursor();

  if (widget) widget->show();
  _busy->hide();
}

void DockContainer::removeModule()
{
  deleteModule();

  resizeEvent(0L);

  if (_basew)
	emit newModule(_basew->caption(), "", "");
  else
	emit newModule("", "", "");
}

void DockContainer::deleteModule()
{
  if(_module) {
	_module->deleteClient();
	_module = 0;
  }
}

void DockContainer::resizeEvent(QResizeEvent *)
{
  _busy->resize(width(), height());
  _rootOnly->resize(width(), height());
  if (_module)
	{
	  _module->module()->resize(size());
	  _basew->hide();
	}
  else
	{
	  _basew->resize(size());
	  _basew->show();
	}
}

void DockContainer::quickHelpChanged()
{
  if (_module && _module->module())
	emit newModule(_module->module()->caption(),  _module->docPath(), _module->module()->quickHelp());
}
