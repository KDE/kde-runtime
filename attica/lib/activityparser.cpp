/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "activityparser.h"

#include <QtCore/QDateTime>
#include <QRegExp>


using namespace Attica;

Activity Activity::Parser::parseXml(QXmlStreamReader& xml)
{
    Activity activity;
    Person person;
    
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == "id") {
                activity.setId(xml.readElementText());
            } else if (xml.name() == "personid") {
                person.setId(xml.readElementText());
            } else if (xml.name() == "avatarpic") {
                person.setAvatarUrl(xml.readElementText());
            } else if (xml.name() == "firstname") {
                person.setFirstName(xml.readElementText());
            } else if (xml.name() == "lastname") {
                person.setLastName(xml.readElementText());
            } else if (xml.name() == "timestamp") {
                QString timestampString = xml.readElementText();
                timestampString.remove(QRegExp("\\+.*$"));
                QDateTime timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
                activity.setTimestamp(timestamp);
            } else if (xml.name() == "message") {
                activity.setMessage(xml.readElementText());
            } else if (xml.name() == "link") {
                activity.setLink(xml.readElementText());
            }
        } else if (xml.isEndElement() && xml.name() == "activity") {
            break;
        }
    }

    activity.setAssociatedPerson(person);
    return activity;
}


QStringList Activity::Parser::xmlElement() const {
    return QStringList("activity");
}
