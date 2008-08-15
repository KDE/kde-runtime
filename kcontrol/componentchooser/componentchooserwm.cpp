/***************************************************************************
                          componentchooserwm.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License verstion 2 as    *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/

#include "componentchooserwm.h"
#include "componentchooserwm.moc"

#include <kdesktopfile.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>

CfgWm::CfgWm(QWidget *parent):WmConfig_UI(parent),CfgPlugin()
{
    connect(wmCombo,SIGNAL(activated(int)), this, SLOT(configChanged()));
    connect(kwinRB,SIGNAL(toggled(bool)),this,SLOT(configChanged()));
    connect(differentRB,SIGNAL(toggled(bool)),this,SLOT(configChanged()));

    KGlobal::dirs()->addResourceType( "windowmanagers", "data", "ksmserver/windowmanagers" );
}

CfgWm::~CfgWm()
{
}

void CfgWm::configChanged()
{
    emit changed(true);
}

void CfgWm::defaults()
{
    wmCombo->setCurrentIndex( 0 );
}


void CfgWm::load(KConfig *)
{
    KConfig cfg("ksmserverrc", KConfig::NoGlobals);
    KConfigGroup c( &cfg, "General");
    loadWMs(c.readEntry("windowManager", "kwin"));
    emit changed(false);
}

void CfgWm::save(KConfig *)
{
    KConfig cfg("ksmserverrc", KConfig::NoGlobals);
    KConfigGroup c( &cfg, "General");
    c.writeEntry("windowManager", currentWM());
    emit changed(false);
    if( oldwm != currentWM())
    { // TODO switch it already in the session instead and tell ksmserver
        KMessageBox::information( this,
            i18n( "The new window manager will be used when KDE is started the next time." ),
            i18n( "Window manager change" ), "windowmanagerchange" );
    }
}

void CfgWm::loadWMs( const QString& current )
{
    wms[ "KWin" ] = "kwin";
    oldwm = "kwin";
    kwinRB->setChecked( true );
    wmCombo->setEnabled( false );
    QStringList list = KGlobal::dirs()->findAllResources( "windowmanagers", QString(), KStandardDirs::NoDuplicates );
    QRegExp reg( ".*/([^/\\.]*)\\.[^/\\.]*" );
    foreach( const QString& wmfile, list )
    {
        KDesktopFile file( wmfile );
        if( file.noDisplay())
            continue;
        if( !file.tryExec())
            continue;
        QString testexec = file.desktopGroup().readEntry( "X-KDE-WindowManagerTestExec" );
        if( !testexec.isEmpty())
        {
            KProcess proc;
            proc.setShellCommand( testexec );
            if( proc.execute() != 0 )
                continue;
        }
        QString name = file.readName();
        if( name.isEmpty())
            continue;
        if( !reg.exactMatch( wmfile ))
            continue;
        QString wm = reg.cap( 1 );
        if( wms.values().contains( wm ))
            continue;
        wms[ name ] = wm;
        wmCombo->addItem( name );
        if( wms[ name ] == current ) // make it selected
        {
            wmCombo->setCurrentIndex( wmCombo->count() - 1 );
            oldwm = wm;
            differentRB->setChecked( true );
            wmCombo->setEnabled( true );
        }
    }
}

QString CfgWm::currentWM() const
{
    return kwinRB->isChecked() ? "kwin" : wms[ wmCombo->currentText() ];
}
