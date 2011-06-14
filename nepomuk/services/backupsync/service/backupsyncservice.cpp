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


#include "backupsyncservice.h"
#include "diffgenerator.h"
#include "syncmanager.h"
#include "backupmanager.h"
#include "dbusoperators.h"

#include "syncfile.h"
#include "changelog.h"

#include <KDebug>

#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::BackupSyncService, "nepomukbackupsync" )

Nepomuk::BackupSyncService::BackupSyncService( QObject* parent, const QList< QVariant >& )
	: Service(parent)
{
    kDebug();

    m_diffGenerator = new DiffGenerator( this );

    m_syncManager = new SyncManager( this );
    m_backupManager = new BackupManager( this );
}

Nepomuk::BackupSyncService::~BackupSyncService()
{
}

void Nepomuk::BackupSyncService::test()
{
    //QUrl url("/home/vishesh/syncnew");

    /*
    LogFile lf;
    lf.load(QUrl("file:///home/vishesh/ident.trig"));
    SyncFile::create( lf, url );
    */
    /*

    SyncFile sf( url );

    LogFile lf = sf.logFile();
    foreach( const Record & r, lf.allRecords() ) {
        kDebug() << r.st;
    }

    kDebug();
    kDebug();
    kDebug() << "Identification File";
    foreach( const Soprano::Statement & st, sf.identificationFile().allStatements() ) {
        kDebug() << st;
    }
    */

    //Sync::SyncFile sf( url );
    //m_identifier->process( sf );
//    m_identifier->process( sf );
//     m_identifier->process( sf );
//     m_identifier->process( sf );
//     m_identifier->process( sf );
//     m_identifier->process( sf );

}

#include "backupsyncservice.moc"
