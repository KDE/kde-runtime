/*
  Copyright (C) 2007-2011 Sebastian Trueg <trueg@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "util.h"
#include "datamanagement.h"
#include "nepomuktools.h"

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>
#include <QtCore/QScopedPointer>
#include <QtCore/QDebug>

#include <KJob>
#include <KDebug>
#include <KGlobal>
#include <KComponentData>


KJob* Nepomuk::clearIndexedData( const QUrl& url )
{
    return clearIndexedData(QList<QUrl>() << url);
}

KJob* Nepomuk::clearIndexedData( const QList<QUrl>& urls )
{
    if ( urls.isEmpty() )
        return 0;

    kDebug() << urls;

    //
    // New way of storing Strigi Data
    // The Datamanagement API will automatically find the resource corresponding to that url
    //
    KComponentData component = KGlobal::mainComponent();
    if( component.componentName() != QLatin1String("nepomukindexer") ) {
        component = KComponentData( QByteArray("nepomukindexer"),
                                    QByteArray(), KComponentData::SkipMainComponentRegistration );
    }
    return Nepomuk::removeDataByApplication( urls, RemoveSubResoures, component );
}
