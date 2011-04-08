/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

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

#include "datamanagementservice.h"
#include "datamanagementmodel.h"
#include "datamanagementadaptor.h"

#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusConnection>


Q_DECLARE_METATYPE(QList<QUrl>)


class Nepomuk::DataManagementService::Private
{
public:
    DataManagementModel* m_dmModel;
};


Nepomuk::DataManagementService::DataManagementService(QObject *parent, const QVariantList& )
    : Service(parent),
      d(new Private())
{
    d->m_dmModel = new DataManagementModel(mainModel(), this);

    Nepomuk::DataManagementAdaptor* adaptor = new Nepomuk::DataManagementAdaptor(d->m_dmModel);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/datamanagementmodel"), adaptor, QDBusConnection::ExportScriptableContents);

    // register the fancier name for this important service
    QDBusConnection::sessionBus().registerService(QLatin1String("org.kde.nepomuk.DataManagement"));
}


Nepomuk::DataManagementService::~DataManagementService()
{
    delete d;
}


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::DataManagementService, "nepomukdatamanagementservice" )
