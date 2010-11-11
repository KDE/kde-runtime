/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef BACKUPWIZARD_H
#define BACKUPWIZARD_H

#include <QtGui/QWizard>

namespace Nepomuk {

    class BackupWizard : public QWizard
    {
        Q_OBJECT

    public:
        BackupWizard(QWidget* parent = 0, Qt::WindowFlags flags = 0);

        enum Pages {
            Id_IntroPage = 0,
            Id_BackupSettingsPage,
            Id_BackupPage,
            Id_RestorePage,
            Id_RestoreSelectionPage,
            Id_BackupDone,
            Id_RestoreFinalPage,
            Id_ErrorPage
        };
        
        void startBackup();
        void startRestore();
        void showError(const QString& error);
    };

}
#endif // BACKUPWIZARD_H
