/*******************************************************************
* drkonqiadaptor.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#include "drkonqiadaptor.h"
#include "krashconf.h"
#include "drkonqi.h"

#include <QtCore/QString>
#include <KDebug>

DrKonqiAdaptor::DrKonqiAdaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent)
{
    m_drkonqiInstance = static_cast<DrKonqi*>(parent);
    setAutoRelaySignals(true);
}

DrKonqiAdaptor::~DrKonqiAdaptor()
{
}

QString DrKonqiAdaptor::appName()
{
    // handle method call org.kde.Krash.appName
    return m_drkonqiInstance->krashConfig()->appName();
}

int DrKonqiAdaptor::pid()
{
    // handle method call org.kde.Krash.pid
    return m_drkonqiInstance->krashConfig()->pid();
}

QString DrKonqiAdaptor::programName()
{
    // handle method call org.kde.Krash.programName
    return m_drkonqiInstance->krashConfig()->programName();
}

void DrKonqiAdaptor::registerDebuggingApplication(const QString &launchName)
{
    // handle method call org.kde.Krash.registerDebuggingApplication
    kDebug() << "la" << launchName;
    m_drkonqiInstance->registerDebuggingApplication(launchName);
}

bool DrKonqiAdaptor::safeMode()
{
    // handle method call org.kde.Krash.safeMode
    return m_drkonqiInstance->krashConfig()->safeMode();
}

QString DrKonqiAdaptor::signalName()
{
    // handle method call org.kde.Krash.signalName
    return m_drkonqiInstance->krashConfig()->signalName();
}

int DrKonqiAdaptor::signalNumber()
{
    // handle method call org.kde.Krash.signalNumber
    return m_drkonqiInstance->krashConfig()->signalNumber();
}

bool DrKonqiAdaptor::startedByKdeinit()
{
    // handle method call org.kde.Krash.startedByKdeinit
    return m_drkonqiInstance->krashConfig()->startedByKdeinit();
}

#include "drkonqiadaptor.moc"
