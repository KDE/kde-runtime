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

#include "strigiservice.h"
#include "strigicontroller.h"
#include "../../common/strigiconfigfile.h"

Nepomuk::StrigiService::StrigiService( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{

    m_strigiController = new StrigiController( this );
    updateStrigiConfig();
    m_strigiController->start();
}

Nepomuk::StrigiService::~StrigiService()
{
}


void Nepomuk::StrigiService::updateStrigiConfig()
{
    StrigiConfigFile strigiConfig ( StrigiConfigFile::defaultStrigiConfigFilePath() );
    strigiConfig.load();
    strigiConfig.defaultRepository().setType( "sopranobackend" );
    strigiConfig.save();
}

#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::StrigiService, "nepomukstrigiservice" )

#include "strigiservice.moc"

