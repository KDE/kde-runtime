/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "systray.h"
#include "indexscheduler.h"

#include <KMenu>
#include <KToggleAction>
#include <KLocale>
#include <KCMultiDialog>
#include <KIcon>



Nepomuk::SystemTray::SystemTray( IndexScheduler* scheduler, QWidget* parent )
    : KSystemTrayIcon( "nepomuk", parent ),
      m_indexScheduler( scheduler )
{
    KMenu* menu = new KMenu;
    menu->addTitle( i18n( "Strigi File Indexing" ) );

    m_suspendResumeAction = new KToggleAction( i18n( "Suspend Strigi Indexing" ), menu );
    m_suspendResumeAction->setCheckedState( KGuiItem( i18n( "Resume Strigi Indexing" ) ) );
    m_suspendResumeAction->setToolTip( i18n( "Suspend or resume the Strigi file indexer manually" ) );
    connect( m_suspendResumeAction, SIGNAL( toggled( bool ) ),
             m_indexScheduler, SLOT( setSuspended( bool ) ) );

    KAction* configAction = new KAction( menu );
    configAction->setText( i18n( "Configure Strigi" ) );
    configAction->setIcon( KIcon( "configure" ) );
    connect( configAction, SIGNAL( triggered() ),
             this, SLOT( slotConfigure() ) );

    menu->addAction( m_suspendResumeAction );
    menu->addAction( configAction );

    setContextMenu( menu );

    connect( m_indexScheduler, SIGNAL( indexingStarted() ),
             this, SLOT( slotUpdateStrigiStatus() ) );
    connect( m_indexScheduler, SIGNAL( indexingStopped() ),
             this, SLOT( slotUpdateStrigiStatus() ) );
    connect( m_indexScheduler, SIGNAL( indexingFolder(QString) ),
             this, SLOT( slotUpdateStrigiStatus() ) );
}


Nepomuk::SystemTray::~SystemTray()
{
}


void Nepomuk::SystemTray::slotUpdateStrigiStatus()
{
    bool indexing = m_indexScheduler->isIndexing();
    bool suspended = m_indexScheduler->isSuspended();
    QString folder = m_indexScheduler->currentFolder();

    if ( suspended )
        setToolTip( i18n( "File indexer is suspended" ) );
    else if ( indexing )
        setToolTip( i18n( "Strigi is currently indexing files in folder %1", folder ) );
    else
        setToolTip( i18n( "File indexer is idle" ) );

    m_suspendResumeAction->setChecked( suspended );
}


void Nepomuk::SystemTray::slotConfigure()
{
    KCMultiDialog dlg;
    dlg.addModule( "kcm_nepomuk" );
    dlg.exec();
}

#include "systray.moc"
