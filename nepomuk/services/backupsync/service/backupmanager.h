/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

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


#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <KConfig>

namespace Nepomuk {

    class Identifier;
    class Merger;

    class BackupManager : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.nepomuk.services.nepomukbackupsync.BackupManager")

    public:
        BackupManager(Nepomuk::Identifier* ident, QObject* parent = 0);
        virtual ~BackupManager();

    public slots:
        void backup( const QString & url = QString() );
        int restore( const QString & url = QString() );

    signals:
        void backupDone();
        
    private:
        Identifier * m_identifier;
        QString m_backupLocation;

        QTime m_backupTime;
        int m_daysBetweenBackups;
        int m_maxBackups;

        KConfig m_config;

        QTimer m_timer;
        void resetTimer();
        void removeOldBackups();
        
    private slots:
        void slotConfigDirty();
        void automatedBackup();
    };

}
#endif // BACKUPMANAGER_H
