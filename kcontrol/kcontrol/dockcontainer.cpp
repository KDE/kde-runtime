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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <qlabel.h>
#include <qvbox.h>
#include <qpixmap.h>
#include <qfont.h>
#include <qwhatsthis.h>
#include <qapplication.h>

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kiconloader.h>

#include "dockcontainer.h"
#include "dockcontainer.moc"

#include "global.h"
#include "modules.h"
#include "proxywidget.h"

class ModuleTitle : public QHBox
{
  public:
    ModuleTitle( QWidget *parent, const char *name=0 );
    ~ModuleTitle() {}

    void showTitleFor( ConfigModule *module );
    void clear();

  protected:
    QLabel *m_icon;
    QLabel *m_name;
};

ModuleTitle::ModuleTitle( QWidget *parent, const char *name )
    : QHBox( parent, name )
{
  QWidget *spacer = new QWidget( this );
  spacer->setFixedWidth( KDialog::marginHint()-KDialog::spacingHint() );
  m_icon = new QLabel( this );
  m_name = new QLabel( this );

  QFont font = m_name->font();
  font.setPointSize( font.pointSize()+1 );
  font.setBold( true );
  m_name->setFont( font );

  setSpacing( KDialog::spacingHint() );
  if ( QApplication::reverseLayout() )
  {
      spacer = new QWidget( this );
      setStretchFactor( spacer, 10 );
  }
  else
      setStretchFactor( m_name, 10 );
}

void ModuleTitle::showTitleFor( ConfigModule *config )
{
  if ( !config )
    return;

  QWhatsThis::remove( this );
  QWhatsThis::add( this, config->comment() );
  KIconLoader *loader = KGlobal::instance()->iconLoader();
  QPixmap icon = loader->loadIcon( config->icon(), KIcon::NoGroup, 22 );
  m_icon->setPixmap( icon );
  m_name->setText( config->moduleName() );

  show();
}

void ModuleTitle::clear()
{
  m_icon->setPixmap( QPixmap() );
  m_name->setText( QString::null );
  kapp->processEvents();
}

class ModuleWidget : public QVBox
{
  public:
    ModuleWidget( QWidget *parent, const char *name );
    ~ModuleWidget() {}

    ProxyWidget* load( ConfigModule *module );

  protected:
    ModuleTitle *m_title;
    QVBox *m_body;
};

ModuleWidget::ModuleWidget( QWidget *parent, const char *name )
    : QVBox( parent, name )
{
  m_title = new ModuleTitle( this, "m_title" );
  m_body = new QVBox( this, "m_body" );
  setStretchFactor( m_body, 10 );
}

ProxyWidget *ModuleWidget::load( ConfigModule *module )
{
  m_title->clear();
  ProxyWidget *proxy = module->module();

  if ( proxy )
  {
    proxy->reparent(m_body, 0, QPoint(0,0), false);
    proxy->show();
    m_title->showTitleFor( module );
  }

  return proxy;
}

DockContainer::DockContainer(QWidget *parent)
  : QWidgetStack(parent, "DockContainer")
  , _basew(0L)
  , _module(0L)
{
  _busyw = new QLabel(i18n("<big><b>Loading...</b></big>"), this);
  _busyw->setAlignment(AlignCenter);
  _busyw->setTextFormat(RichText);
  _busyw->setGeometry(0,0, width(), height());
  addWidget( _busyw );

  _modulew = new ModuleWidget( this, "_modulew" );
  addWidget( _modulew );
}

DockContainer::~DockContainer()
{
  deleteModule();
}

void DockContainer::setBaseWidget(QWidget *widget)
{
  removeWidget( _basew );
  delete _basew;
  _basew = 0;
  if (!widget) return;

  _basew = widget;

  addWidget( _basew );
  raiseWidget( _basew );

  emit newModule(widget->caption(), "", "");
}

ProxyWidget* DockContainer::loadModule( ConfigModule *module )
{
  QApplication::setOverrideCursor( waitCursor );

  ProxyWidget *widget = _modulew->load( module );

  if (widget)
  {
    _module = module;
    connect(_module, SIGNAL(childClosed()), SLOT(removeModule()));
    connect(_module, SIGNAL(changed(ConfigModule *)),
            SIGNAL(changedModule(ConfigModule *)));
    connect(widget, SIGNAL(quickHelpChanged()), SLOT(quickHelpChanged()));

    raiseWidget( _modulew );
    emit newModule(widget->caption(), module->docPath(), widget->quickHelp());
  }
  else
  {
    raiseWidget( _basew );
    emit newModule(_basew->caption(), "", "");
  }

  QApplication::restoreOverrideCursor();

  return widget;
}

bool DockContainer::dockModule(ConfigModule *module)
{
  if (module == _module) return true;

  if (_module && _module->isChanged())
    {

      int res = KMessageBox::warningYesNoCancel(this,
module ?
i18n("There are unsaved changes in the active module.\n"
     "Do you want to apply the changes before running "
     "the new module or discard the changes?") :
i18n("There are unsaved changes in the active module.\n"
     "Do you want to apply the changes before exiting "
     "the Control Center or discard the changes?"),
                                          i18n("Unsaved Changes"),
                                          i18n("&Apply"),
                                          i18n("&Discard"));
      if (res == KMessageBox::Yes)
        _module->module()->applyClicked();
      if (res == KMessageBox::Cancel)
        return false;
    }

  raiseWidget( _busyw );
  kapp->processEvents();

  deleteModule();
  if (!module) return true;

  ProxyWidget *widget = loadModule( module );

  KCGlobal::repairAccels( topLevelWidget() );
  return ( widget!=0 );
}

void DockContainer::removeModule()
{
  raiseWidget( _basew );
  deleteModule();

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

void DockContainer::quickHelpChanged()
{
  if (_module && _module->module())
	emit newModule(_module->module()->caption(),  _module->docPath(), _module->module()->quickHelp());
}
