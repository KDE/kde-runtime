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

#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H

#include <KDialog>
#include "ui_controlwidget.h"

#include "../../service/dbusoperators.h"
#include "syncmanagerinterface.h"
#include "backupmanagerinterface.h"


namespace Nepomuk {

    class IdentifierWidget;
    class MergerWidget;

    class ControlWidget : public KDialog, public Ui::ControlWidget
    {
        Q_OBJECT
    public:
        ControlWidget( QWidget* parent = 0);

    public Q_SLOTS:
        void backup();
        void restore();
        void sync();
        void createSyncFile();

    private:
        typedef org::kde::nepomuk::services::nepomukbackupsync::SyncManager SyncManager; 
        typedef org::kde::nepomuk::services::nepomukbackupsync::BackupManager BackupManager;
        
        SyncManager * m_syncManager;
        BackupManager * m_backupManager;

        IdentifierWidget * m_identifierWidget;
        MergerWidget * m_mergerWidget;
    };
}

#endif // CONTROLWIDGET_H
