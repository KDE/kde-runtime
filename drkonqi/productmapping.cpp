/*******************************************************************
* productmapping.cpp
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

#include "productmapping.h"

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>

ProductMapping::ProductMapping(const QString & appName, QObject * parent)
    : QObject(parent)
{
    m_bugzillaProduct = appName;
    m_bugzillaComponent = QLatin1String("general");
    map(appName);
}

void ProductMapping::map(const QString & appName)
{
    mapUsingInternalFile(appName);
}

void ProductMapping::mapUsingInternalFile(const QString & appName)
{
    KConfig mappingsFile(QString::fromLatin1("mappings"), KConfig::NoGlobals, "appdata");
    const KConfigGroup mappings = mappingsFile.group("Mappings");
    if (mappings.hasKey(appName)) {
        QString mappingString = mappings.readEntry(appName);
        if (!mappingString.isEmpty()) {
            QStringList list = mappingString.split('|', QString::SkipEmptyParts);
            if (list.count()==2) {
                m_bugzillaProduct = list.at(0);
                m_bugzillaComponent = list.at(1);
            } else {
                kWarning() << "Error while reading mapping entry. Sections found " << list.count();
            }
        } else {
            kWarning() << "Error while reading mapping entry. Entry exists but it is empty "
                            "(or there was an error when reading)";
        }
    }
}

QString ProductMapping::bugzillaProduct() const
{
    return m_bugzillaProduct;
}

QString ProductMapping::bugzillaComponent() const
{
    return m_bugzillaComponent;
}
