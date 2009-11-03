/*
    This file is part of KDE.

    Copyright (c) 2009 Eckhart Wörner <ewoerner@kde.org>

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

#include "eventparser.h"

#include <QtCore/QRegExp>


using namespace Attica;

Event Event::Parser::parseXml(QXmlStreamReader& xml)
{
    Event event;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == "id") {
                event.setId(xml.readElementText());
            } else if (xml.name() == "name") {
                event.setName(xml.readElementText());
            } else if (xml.name() == "description") {
                event.setDescription(xml.readElementText());
            } else if (xml.name() == "user") {
                event.setUser(xml.readElementText());
            } else if (xml.name() == "startdate") {
                QString date = xml.readElementText().remove(QRegExp("\\+.*$"));
                event.setStartDate(QDate::fromString(date, Qt::ISODate));
            } else if (xml.name() == "enddate") {
                QString date = xml.readElementText().remove(QRegExp("\\+.*$"));
                event.setEndDate(QDate::fromString(date, Qt::ISODate));
            } else if (xml.name() == "latitude") {
                event.setLatitude(xml.readElementText().toFloat());
            } else if (xml.name() == "longitude") {
                event.setLongitude(xml.readElementText().toFloat());
            } else if (xml.name() == "homepage") {
                event.setHomepage(xml.readElementText());
            } else if (xml.name() == "country") {
                event.setCountry(xml.readElementText());
            } else if (xml.name() == "city") {
                event.setCity(xml.readElementText());
            } else {
                event.addExtendedAttribute(xml.name().toString(), xml.readElementText());
            }
        } else if (xml.isEndElement() && xml.name() == "event") {
            break;
        }
    }

    return event;
}


QStringList Event::Parser::xmlElement() const {
    return QStringList("event");
}
