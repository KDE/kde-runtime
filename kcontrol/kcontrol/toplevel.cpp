/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston,
  MA  02111-1307, USA.
*/

#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kbugreport.h>
#include <kaboutapplication.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <kiconloader.h>

#include <qtabwidget.h>
#include <qwhatsthis.h>
#include <qpixmap.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qfont.h>

#include <kaction.h>
#include <kkeydialog.h>
#include <kglobalsettings.h>

#include "indexwidget.h"
#include "searchwidget.h"
#include "helpwidget.h"
#include "aboutwidget.h"
#include "proxywidget.h"
#include "moduletreeview.h"
#include <stdio.h>

#include "toplevel.h"
#include "toplevel.moc"

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

  QFont font = KGlobalSettings::generalFont();
  font.setPointSize( font.pointSize()+2 );
  font.setBold( true );
  m_name->setFont( font );

  setSpacing( KDialog::spacingHint() );
  setStretchFactor( m_name, 10 );
}

void ModuleTitle::showTitleFor( ConfigModule *config )
{
  if ( !config )
    return;

  QWhatsThis::add( this, QString::null );
  QWhatsThis::add( this, config->comment() );
  m_icon->setPixmap( BarIcon( config->icon() ) );
  m_name->setText( config->moduleName() );

  show();
}

void ModuleTitle::clear()
{
  m_icon->setPixmap( QPixmap() );
  m_name->setText( QString::null );
}

TopLevel::TopLevel(const char* name)
  : KMainWindow( 0, name, WStyle_ContextHelp  )
  , _active(0), dummyAbout(0)
{
  setCaption(QString::null);

  report_bug = 0;

  // read settings
  KConfig *config = KGlobal::config();
  config->setGroup("Index");
  QString viewmode = config->readEntry("ViewMode", "Tree");

  if (viewmode == "Tree")
    KCGlobal::setViewMode(Tree);
  else
    KCGlobal::setViewMode(Icon);

  QString size = config->readEntry("IconSize", "Medium");
  if (size == "Small")
    KCGlobal::setIconSize(Small);
  else if (size == "Large")
    KCGlobal::setIconSize(Large);
  else
    KCGlobal::setIconSize(Medium);

  // initialize the entries
  _modules = new ConfigModuleList();
  _modules->readDesktopEntries();

  for ( ConfigModule* m = _modules->first(); m; m = _modules->next() )
      connect( m, SIGNAL( helpRequest() ), this, SLOT( slotHelpRequest() ) );

  // create the layout box
  _splitter = new QSplitter( QSplitter::Horizontal, this );

  // create the left hand side (the tab view)
  _tab = new QTabWidget( _splitter );

  QWhatsThis::add( _tab, i18n("Choose between Index, Search and Quick Help") );

  // index tab
  _indextab = new IndexWidget(_modules, _tab);
  connect(_indextab, SIGNAL(moduleActivated(ConfigModule*)),
                  this, SLOT(activateModule(ConfigModule*)));
  _tab->addTab(_indextab, i18n("&Index"));

  connect(_indextab, SIGNAL(categorySelected(QListViewItem*)),
                  this, SLOT(categorySelected(QListViewItem*)));

  // search tab
  _searchtab = new SearchWidget(_tab);
  _searchtab->populateKeywordList(_modules);
  connect(_searchtab, SIGNAL(moduleSelected(ConfigModule *)),
                  this, SLOT(activateModule(ConfigModule *)));

  _tab->addTab(_searchtab, i18n("Sear&ch"));

  // help tab
  _helptab = new HelpWidget(_tab);
  _tab->addTab(_helptab, i18n("Hel&p"));

  _tab->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ) );

 // Restore sizes
  config->setGroup("General");
  QValueList<int> sizes = config->readIntListEntry(  "SplitterSizes" );
  if (!sizes.isEmpty())
     _splitter->setSizes(sizes);

  // That one does the trick ...
  _splitter->setResizeMode( _tab, QSplitter::KeepSize );

  // set up the right hand side (the docking area)
  QVBox *base = new QVBox( _splitter );
  _title = new ModuleTitle( base );
  _title->hide();
  _dock = new DockContainer( base );
  base->setStretchFactor( _dock, 10 );
  _dock->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

  connect(_dock, SIGNAL(newModule(const QString&, const QString&, const QString&)),
                  this, SLOT(newModule(const QString&, const QString&, const QString&)));
  connect(_dock, SIGNAL(changedModule(ConfigModule*)),
          SLOT(changedModule(ConfigModule*)));

  // insert the about widget
  // First preload about pixmaps for speedup
  AboutWidget::initPixmaps();

  AboutWidget *aw = new AboutWidget(this);
  connect( aw, SIGNAL( moduleSelected( ConfigModule * ) ),
           SLOT( activateModule( ConfigModule * ) ) );
  _dock->setBaseWidget(aw);

  // set the main view
  setCentralWidget( _splitter );

  // initialize the GUI actions
  setupActions();

  // activate defaults
  if (KCGlobal::viewMode() == Tree)   {
    activateTreeView();
    tree_view->setChecked(true);
  }
  else {
    activateIconView();
    icon_view->setChecked(true);
  }
  if (KCGlobal::isInfoCenter())
  {
      AboutWidget *aw = new AboutWidget( this, 0, _indextab->firstTreeViewItem());
      connect( aw, SIGNAL( moduleSelected( ConfigModule * ) ),
               SLOT( activateModule( ConfigModule * ) ) );
      _dock->setBaseWidget( aw );
  }
}

TopLevel::~TopLevel()
{
  KConfig *config = KGlobal::config();
  config->setGroup("Index");
  if (KCGlobal::viewMode() == Tree)
    config->writeEntry("ViewMode", "Tree");
  else
    config->writeEntry("ViewMode", "Icon");

  switch (KCGlobal::iconSize())
    {
    case Small:
      config->writeEntry("IconSize", "Small");
      break;
    case Medium:
      config->writeEntry("IconSize", "Medium");
      break;
    case Large:
      config->writeEntry("IconSize", "Large");
      break;
    default:
      config->writeEntry("IconSize", "Medium");
      break;
    }

  config->setGroup("General");
  config->writeEntry("SplitterSizes", _splitter->sizes());

  config->sync();

  delete _modules;

  // Not used anymore, free the pixmaps to the X server
  AboutWidget::freePixmaps();
}

bool TopLevel::queryClose()
{
  return _dock->dockModule(0);
}

void TopLevel::setupActions()
{
  KStdAction::quit(this, SLOT(close()), actionCollection());
  KStdAction::keyBindings( this, SLOT( slotConfigureKeys() ), actionCollection() );
  icon_view = new KRadioAction
    (i18n("&Icon View"), 0, this, SLOT(activateIconView()),
     actionCollection(), "activate_iconview");
  icon_view->setExclusiveGroup( "viewmode" );

  tree_view = new KRadioAction
    (i18n("&Tree View"), 0, this, SLOT(activateTreeView()),
     actionCollection(), "activate_treeview");
  tree_view->setExclusiveGroup( "viewmode" );

  icon_small = new KRadioAction
    (i18n("&Small"), 0, this, SLOT(activateSmallIcons()),
     actionCollection(), "activate_smallicons");
  icon_small->setExclusiveGroup( "iconsize" );

  icon_medium = new KRadioAction
    (i18n("&Medium"), 0, this, SLOT(activateMediumIcons()),
     actionCollection(), "activate_mediumicons");
  icon_medium->setExclusiveGroup( "iconsize" );

  icon_large = new KRadioAction
    (i18n("&Large"), 0, this, SLOT(activateLargeIcons()),
     actionCollection(), "activate_largeicons");
  icon_large->setExclusiveGroup( "iconsize" );

  about_module = new KAction(i18n("About Current Module"), 0, this, SLOT(aboutModule()), actionCollection(), "help_about_module");
  about_module->setEnabled(false);

  // I need to add this so that each module can get a bug reported,
  // and not just KControl
  if (KCGlobal::isInfoCenter())
    createGUI("kinfocenterui.rc");
  else
    createGUI("kcontrolui.rc");

  report_bug = actionCollection()->action("help_report_bug");
  report_bug->setText(i18n("&Report Bug..."));
  report_bug->disconnect();
  connect(report_bug, SIGNAL(activated()), SLOT(reportBug()));
}

void TopLevel::slotConfigureKeys()
{
  KKeyDialog::configure( actionCollection(), this );
}

void TopLevel::activateIconView()
{
  KCGlobal::setViewMode(Icon);
  _indextab->activateView(Icon);

  icon_small->setEnabled(true);
  icon_medium->setEnabled(true);
  icon_large->setEnabled(true);

  switch(KCGlobal::iconSize())
    {
    case Small:
      icon_small->setChecked(true);
      break;
    case Large:
      icon_large->setChecked(true);
      break;
    default:
      icon_medium->setChecked(true);
      break;
    }
}

void TopLevel::activateTreeView()
{
  KCGlobal::setViewMode(Tree);
  _indextab->activateView(Tree);

  icon_small->setEnabled(false);
  icon_medium->setEnabled(false);
  icon_large->setEnabled(false);
}

void TopLevel::activateSmallIcons()
{
  KCGlobal::setIconSize(Small);
  _indextab->reload();
}

void TopLevel::activateMediumIcons()
{
  KCGlobal::setIconSize(Medium);
  _indextab->reload();
}

void TopLevel::activateLargeIcons()
{
  KCGlobal::setIconSize(Large);
  _indextab->reload();
}

void TopLevel::newModule(const QString &name, const QString& docPath, const QString &quickhelp)
{
    setCaption(name, false);

  _helptab->setText( docPath, quickhelp );

  if (!report_bug) return;

  if(name.isEmpty())
    report_bug->setText(i18n("&Report Bug..."));
  else
    report_bug->setText(i18n("Report Bug on Module %1...").arg( handleAmpersand( name)));
}

void TopLevel::changedModule(ConfigModule *changed)
{
    if (!changed)
        return;
    setCaption(changed->moduleName(), changed->isChanged() );
}

void TopLevel::categorySelected(QListViewItem *category)
{
  if (_active)
  {
    if (_active->isChanged())
      {
        int res = KMessageBox::warningYesNoCancel(this, _active ?
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
          _active->module()->applyClicked();
        else if (res == KMessageBox::Cancel)
          return;
      }
  }
  _dock->removeModule();
  about_module->setText( i18n( "About Current Module" ) );
  about_module->setIconSet( QIconSet() );
  about_module->setEnabled( false );

  // insert the about widget
  QListViewItem *firstItem = category->firstChild();
  QString caption = static_cast<ModuleTreeItem*>(category)->caption();
  if( _dock->baseWidget()->isA( "AboutWidget" ) )
  {
    static_cast<AboutWidget *>( _dock->baseWidget() )->setCategory( firstItem, caption);
  }
  else
  {
    AboutWidget *aw = new AboutWidget( this, 0, firstItem, caption );
    connect( aw, SIGNAL( moduleSelected( ConfigModule * ) ),
             SLOT( activateModule( ConfigModule * ) ) );
    _dock->setBaseWidget( aw );
  }
}


void TopLevel::activateModule(ConfigModule *mod)
{
  // tell the index to display the module
  _indextab->makeVisible(mod);

  // tell the index to mark this module as loaded
  _indextab->makeSelected(mod);

  _title->showTitleFor( mod );

  // dock it
  if (!_dock->dockModule(mod))
  {
     _title->hide();
     _indextab->makeVisible(_active);
     _indextab->makeSelected(_active);
     return;
  }

  _active=mod;

  if (mod->aboutData())
  {
     about_module->setText(i18n("Help menu->about <modulename>", "About %1").arg(
                             handleAmpersand( mod->moduleName())));
     about_module->setIcon(mod->icon());
     about_module->setEnabled(true);
  }
  else
  {
     about_module->setText(i18n("About Current Module"));
     about_module->setIconSet(QIconSet());
     about_module->setEnabled(false);
  }
}

#if 0
void TopLevel::activateModule(const QString& name)
{
  kdDebug(1208) << "activate: " << name << endl;
  for (ConfigModule *mod = _modules->first(); mod != 0; mod = _modules->next())
  {
     if (mod->fileName() == name)
     {
        activateModule(mod);
        return;
     }
  }
}
#endif

void TopLevel::deleteDummyAbout()
{
  delete dummyAbout;
  dummyAbout = 0;
}


void TopLevel::slotHelpRequest()
{
    _tab->showPage( _helptab );
}

void TopLevel::reportBug()
{
    // this assumes the user only opens one bug report at a time
    static char buffer[128];

    dummyAbout = 0;
    bool deleteit = false;

    if (!_active) // report against kcontrol
        dummyAbout = const_cast<KAboutData*>(KGlobal::instance()->aboutData());
    else
    {
        if (_active->aboutData())
            dummyAbout = const_cast<KAboutData*>(_active->aboutData());
        else
        {
            snprintf(buffer, sizeof(buffer), "kcm%s", _active->library().latin1());
            dummyAbout = new KAboutData(buffer, _active->moduleName().utf8(), "2.0");
            deleteit = true;
        }
    }
    KBugReport *br = new KBugReport(this, false, dummyAbout);
    if (deleteit)
        connect(br, SIGNAL(finished()), SLOT(deleteDummyAbout()));
    else
        dummyAbout = 0;
    br->show();
}

void TopLevel::aboutModule()
{
    KAboutApplication dlg(_active->aboutData());
    dlg.exec();
}

QString TopLevel::handleAmpersand( QString modulename ) const
{
   if( modulename.contains( '&' )) // double it
   {
      for( int i = modulename.length();
           i >= 0;
           --i )
         if( modulename[ i ] == '&' )
             modulename.insert( i, "&" );
   }
   return modulename;
}
