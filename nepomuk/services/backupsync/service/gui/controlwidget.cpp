/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

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
#include "../backupsyncservice.h"
#include "../backupmanager.h"
#include "../syncmanager.h"


Nepomuk::ControlWidget::ControlWidget(Nepomuk::BackupSyncService* service, QWidget* parent)
    : KDialog( parent ),
      m_service( service )
{
    m_backupManager = service->backupManager();
    m_syncManager = service->syncManager();

    setupUi( mainWidget() );

    connect( m_backupButton, SIGNAL(clicked(bool)),
             m_backupManager, SLOT(backup()) );
    connect( m_restoreBackupButton, SIGNAL(clicked(bool)),
             m_backupManager, SLOT(restore()) );
    connect( m_syncButton, SIGNAL(clicked(bool)),
             this, SLOT(sync()) );
    connect( m_createSyncFileButton, SIGNAL(clicked(bool)),
             this, SLOT(createSyncFile()) );

}


void Nepomuk::ControlWidget::createSyncFile()
{
}


void Nepomuk::ControlWidget::sync()
{
}
