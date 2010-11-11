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

#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H

#include <KDialog>
#include "ui_controlwidget.h"

namespace Nepomuk {

    class BackupSyncService;
    class BackupManager;
    class SyncManager;

    class ControlWidget : public KDialog, public Ui::ControlWidget
    {
        Q_OBJECT
    public:
        ControlWidget( BackupSyncService* service, QWidget* parent = 0);

    public Q_SLOTS:
        void sync();
        void createSyncFile();

    private:
        BackupSyncService * m_service;
        BackupManager * m_backupManager;
        SyncManager * m_syncManager;
    };

}

#endif // CONTROLWIDGET_H
