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

#include "storage.h"
#include "nepomukcore.h"
#include "repository.h"

#include <QtDBus/QDBusConnection>
#include <QtCore/QFile>

#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

#include <Soprano/Backend>

#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::Storage, "nepomukstorage" )


Nepomuk::Storage::Storage( QObject* parent, const QList<QVariant>& )
    : Service( parent, true /* delayed initialization */ )
{
    QDBusConnection::sessionBus().registerService( "org.kde.NepomukStorage" );

    m_core = new Core( this );
    connect( m_core, SIGNAL( initializationDone(bool) ),
             this, SLOT( slotNepomukCoreInitialized(bool) ) );
    m_core->init();
}


Nepomuk::Storage::~Storage()
{
}


void Nepomuk::Storage::slotNepomukCoreInitialized( bool success )
{
    if ( success ) {
        kDebug() << "Successfully initialized nepomuk core";

        // the core is initialized. Export it to the clients.
        // the D-Bus interface
        m_core->registerAsDBusObject();

        // the faster local socket interface
        QString socketPath = KGlobal::dirs()->locateLocal( "data", "nepomuk/socket" );
        QFile::remove( socketPath ); // in case we crashed
        m_core->start( socketPath );
    }
    else {
        kDebug() << "Failed to initialize nepomuk core";
    }

    setServiceInitialized( success );
}


QString Nepomuk::Storage::usedSopranoBackend() const
{
    if ( Repository* rep = static_cast<Repository*>( m_core->model( QLatin1String( "main" ) ) ) )
        return rep->usedSopranoBackend();
    else
        return QString();
}

#include "storage.moc"
