/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "controlwidget.h"
#include "../common/identifierwidget.h"
#include "../common/mergerwidget.h"

#include <KFileDialog>
#include <KMessageBox>

#include <QtGui/QHBoxLayout>

static const QString ServiceName_s = QLatin1String("org.kde.nepomuk.services.nepomukbackupsync");

Nepomuk::ControlWidget::ControlWidget( QWidget* parent )
    : KDialog( parent )
{
    m_syncManager = new SyncManager( ServiceName_s, "/syncmanager",
                                     QDBusConnection::sessionBus(), this);
    m_backupManager = new BackupManager( ServiceName_s, "/backupmanager",
                                         QDBusConnection::sessionBus(), this);
    setupUi( mainWidget() );

    connect( m_backupButton, SIGNAL(clicked(bool)),
             this, SLOT(backup()) );
    connect( m_restoreBackupButton, SIGNAL(clicked(bool)),
             this, SLOT(restore()) );

    connect( m_syncButton, SIGNAL(clicked(bool)),
             this, SLOT(sync()) );
    connect( m_createSyncFileButton, SIGNAL(clicked(bool)),
             this, SLOT(createSyncFile()) );

    m_identifierWidget = new IdentifierWidget();
    m_mergerWidget = new MergerWidget();

    QHBoxLayout * layout = new QHBoxLayout();
    layout->addWidget( m_identifierWidget );
    layout->addWidget( m_mergerWidget );

    verticalLayout->addLayout( layout );
}


void Nepomuk::ControlWidget::backup()
{
    if( !m_backupManager->isValid() ) {
        KMessageBox::error( this, "Nepomuk BackupSync Service is not running");
        return;
    }

    m_backupManager->backup( QString() );
}


void Nepomuk::ControlWidget::restore()
{
    if( !m_backupManager->isValid() ) {
        KMessageBox::error( this, "Nepomuk BackupSync Service is not running");
        return;
    }

    m_backupManager->restore( QString() );
}


void Nepomuk::ControlWidget::createSyncFile()
{
    if( !m_syncManager->isValid() ) {
        KMessageBox::error( this, "Nepomuk BackupSync Service is not running");
        return;
    }

    QString url = KFileDialog::getSaveFileName( KUrl(), QLatin1String("*.syncFile"), this );
    if( !url.isEmpty() ) {
        QDateTime min;
        min.setTime_t( 0 );
        m_syncManager->createSyncFile( url, min.toString() );
    }
}


void Nepomuk::ControlWidget::sync()
{
    if( !m_syncManager->isValid() ) {
        KMessageBox::error( this, "Nepomuk BackupSync Service is not running");
        return;
    }

    QString url = KFileDialog::getOpenFileName( KUrl(), QLatin1String("*.syncFile"), this );
    if( !url.isEmpty() ) {
        m_syncManager->sync( url );
    }
}
