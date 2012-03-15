/*
 *   Copyright 2011 by Marco Martin <mart@kde.org>
 *   Copyright 2012 by Sebastian Kügler <sebas@kde.org>
 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "plasmaextracomponentsplugin.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeItem>


#include <KSharedConfig>
#include <KConfigGroup>
#include <KDebug>


void PlasmaExtraComponentsPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.extras"));

    QString componentsPlatform = getenv("KDE_PLASMA_COMPONENTS_PLATFORM");
    if (componentsPlatform.isEmpty()) {
        KConfigGroup cg(KSharedConfig::openConfig("kdeclarativerc"), "Components-platform");
        componentsPlatform = cg.readEntry("name", "desktop");
    }
    // Register types...
}


#include "plasmaextracomponentsplugin.moc"

