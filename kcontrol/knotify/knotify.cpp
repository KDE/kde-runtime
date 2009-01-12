/*
    Copyright (C) 2000,2002 Carsten Pfeiffer <pfeiffer@kde.org>
    Copyright (C) 2005,2006 Olivier Goffart <ogoffart at kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QDBusInterface>


#include <kapplication.h>
#include <kaboutdata.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <knotifyconfigwidget.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kstandarddirs.h>
#include <kurlcompletion.h>
#include <kurlrequester.h>


#include "knotify.h"
#include "ui_playersettings.h"

static const int COL_FILENAME = 1;

K_PLUGIN_FACTORY( NotifyFactory, registerPlugin<KCMKNotify>(); )
K_EXPORT_PLUGIN( NotifyFactory("kcmnotify") )

KCMKNotify::KCMKNotify(QWidget *parent, const QVariantList & )
    : KCModule(NotifyFactory::componentData(), parent/*, name*/),
      m_playerSettings( 0L )
{
    setButtons( Help | Default | Apply );

    setQuickHelp( i18n("<h1>System Notifications</h1>"
                "KDE allows for a great deal of control over how you "
                "will be notified when certain events occur. There are "
                "several choices as to how you are notified:"
                "<ul><li>As the application was originally designed.</li>"
                "<li>With a beep or other noise.</li>"
                "<li>Via a popup dialog box with additional information.</li>"
                "<li>By recording the event in a logfile without "
                "any additional visual or audible alert.</li>"
                "</ul>"));

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->setSpacing( KDialog::spacingHint() );
    QTabWidget *tab = new QTabWidget(this);
    layout->addWidget(tab);

    QWidget * app_tab = new QWidget(tab);
    QVBoxLayout *app_layout = new QVBoxLayout( app_tab );

    QLabel *label = new QLabel( i18n( "Event source:" ), app_tab );
    m_appCombo = new KComboBox( false, app_tab );
    m_appCombo->setObjectName( "app combo" );
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setSpacing( KDialog::spacingHint() );
    app_layout->addItem( hbox );
    hbox->addWidget( label );
    hbox->addWidget( m_appCombo, 10 );

    m_notifyWidget = new KNotifyConfigWidget( app_tab );
    app_layout->addWidget( m_notifyWidget );

    connect( m_notifyWidget, SIGNAL(changed(bool)), this,  SIGNAL(changed(bool)));

    m_playerSettings = new PlayerSettingsDialog(tab);
    connect(m_playerSettings, SIGNAL(changed(bool)) , this, SIGNAL(changed(bool)));

/*  general->layout()->setMargin( KDialog::marginHint() );
    hardware->layout()->setMargin( KDialog::marginHint() );*/
    tab->addTab(app_tab, i18n("&Applications"));
    tab->addTab(m_playerSettings, i18n("&Player Settings"));

    connect( m_appCombo, SIGNAL( activated( int ) ),
             SLOT( slotAppActivated( int )) );

    KAboutData* ab = new KAboutData(
        "kcmknotify", 0, ki18n("KNotify"), "4.0",
        ki18n("System Notification Control Panel Module"),
        KAboutData::License_GPL, ki18n("(c) 2002-2006 KDE Team"));

    ab->addAuthor( ki18n("Olivier Goffart"), KLocalizedString(), "ogoffart@kde.org" );
    ab->addAuthor( ki18n("Carsten Pfeiffer"), KLocalizedString(), "pfeiffer@kde.org" );
    ab->addCredit( ki18n("Charles Samuels"), ki18n("Original implementation"),
        "charles@altair.dhs.org" );
    setAboutData( ab );
}

KCMKNotify::~KCMKNotify()
{
    KConfig _config("knotifyrc", KConfig::NoGlobals);
    KConfigGroup config(&_config, "Misc" );
    config.writeEntry( "LastConfiguredApp", m_appCombo->currentText() );
    config.sync();
}

void KCMKNotify::slotAppActivated( int index )
{
    QString text;
    if(index>=0 && index < m_appNames.size())
        text=m_appNames[index];
    m_notifyWidget->save();
    m_notifyWidget->setApplication( text );
}

void KCMKNotify::slotPlayerSettings()
{
}


void KCMKNotify::defaults()
{
//    m_notifyWidget->resetDefaults( true ); // ask user
    m_playerSettings->defaults();
}
void KCMKNotify::load()
{
    //setEnabled( false );
    // setCursor( KCursor::waitCursor() );

    m_appCombo->clear();
    m_appNames.clear();
//    m_notifyWidget->clear();

    QStringList fullpaths =
        KGlobal::dirs()->findAllResources("data", "*/*.notifyrc", KStandardDirs::NoDuplicates );

    foreach (const QString &fullPath, fullpaths )
    {
        int slash = fullPath.lastIndexOf( '/' ) - 1;
        int slash2 = fullPath.lastIndexOf( '/', slash );
        QString appname= slash2 < 0 ? QString() :  fullPath.mid( slash2+1 , slash-slash2  );
        if ( !appname.isEmpty() )
        {
            KConfig config(fullPath, KConfig::NoGlobals, "data" );
            KConfigGroup globalConfig( &config, QString::fromLatin1("Global") );
            QString icon = globalConfig.readEntry(QString::fromLatin1("IconName"), QString::fromLatin1("misc"));
            QString description = globalConfig.readEntry( QString::fromLatin1("Comment"), appname );
            m_appCombo->addItem( SmallIcon( icon ), description );
            m_appNames.append( appname );
        }
    }
    /*
    KConfig config( "knotifyrc", true, false );
    config.setGroup( "Misc" );
    QString appDesc = config.readEntry( "LastConfiguredApp", "KDE System Notifications" );

    if this code gets enabled again, make sure to apply r494965

    if ( !appDesc.isEmpty() )
        m_appCombo->setCurrentItem( appDesc );

     // sets the applicationEvents for KNotifyWidget
    slotAppActivated( m_appCombo->currentText() );

    // unsetCursor(); // unsetting doesn't work. sigh.
    setEnabled( true );
    emit changed( false );
    */

    m_playerSettings->load();

    if ( m_appNames.count() > 0 )
        m_notifyWidget->setApplication( m_appNames.at( 0 ) );

    emit changed(false);

}

void KCMKNotify::save()
{
    if ( m_playerSettings )
        m_playerSettings->save();

    m_notifyWidget->save(); // will dcop knotify about its new config

    emit changed( true );
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

PlayerSettingsDialog::PlayerSettingsDialog( QWidget *parent )
    : QWidget(parent), m_change(false)
{

    m_ui = new Ui::PlayerSettingsUI();
    m_ui->setupUi( this );

    load();
    
    connect( m_ui->cbExternal, SIGNAL( toggled( bool ) ), this, SLOT( externalToggled( bool ) ) );
    connect( m_ui->cbArts, SIGNAL(clicked(bool)), this, SLOT(slotChanged()));
    connect( m_ui->cbExternal, SIGNAL(clicked(bool)), this, SLOT(slotChanged()));
    connect( m_ui->cbNone, SIGNAL(clicked(bool)), this, SLOT(slotChanged()));
    connect( m_ui->volumeSlider, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );
    connect( m_ui->reqExternal, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotChanged() ) );
    m_ui->reqExternal->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
}

void PlayerSettingsDialog::load()
{
    KConfig _config( "knotifyrc", KConfig::NoGlobals  );
    KConfigGroup config(&_config, "Sounds" );
    bool useExternal = config.readEntry( "Use external player", false );
    m_ui->cbExternal->setChecked( useExternal );
    m_ui->reqExternal->setPath( config.readPathEntry( "External player", QString() ) );
    m_ui->volumeSlider->setValue( config.readEntry( "Volume", 100 ) );

    if ( !m_ui->cbExternal->isChecked() )
    {
        m_ui->cbNone->setChecked( config.readEntry( "No sound", false ) );
    }
    emit changed( false );
    m_change=false;
}

void PlayerSettingsDialog::save()
{
    if(!m_change)
        return;
    
    // see kdebase/runtime/knotify/notifybysound.h
    KConfig _config("knotifyrc", KConfig::NoGlobals);
    KConfigGroup config(&_config, "Sounds" );

    config.writePathEntry( "External player", m_ui->reqExternal->url().path() );
    config.writeEntry( "Use external player", m_ui->cbExternal->isChecked() );
    config.writeEntry( "Volume", m_ui->volumeSlider->value() );
    config.writeEntry( "No sound",  m_ui->cbNone->isChecked() );

    config.sync();
    
    QDBusInterface itr("org.kde.knotify", "/Notify", "org.kde.KNotify", QDBusConnection::sessionBus(), this);
    itr.call("reconfigure");
    m_change=false;
}


void PlayerSettingsDialog::slotChanged()
{
    m_change=true;
    emit changed(true);
}

void PlayerSettingsDialog::defaults()
{
    m_ui->cbArts->setChecked(true);
    m_change=true;
    emit changed(true);
}

void PlayerSettingsDialog::externalToggled( bool on )
{
    if ( on )
        m_ui->reqExternal->setFocus();
    else
        m_ui->reqExternal->clearFocus();
}

PlayerSettingsDialog::~ PlayerSettingsDialog( )
{
	delete m_ui;
}

#include "knotify.moc"
