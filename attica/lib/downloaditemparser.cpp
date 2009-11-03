/*
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

#include "downloaditemparser.h"

#include <QtXml/QXmlStreamReader>

using namespace Attica;


QStringList DownloadItem::Parser::xmlElement() const
{
    return QStringList("content");
}


DownloadItem DownloadItem::Parser::parseXml(QXmlStreamReader& xml)
{
    DownloadItem item;

    while ( !xml.atEnd() ) {
        xml.readNext();
        if ( xml.isStartElement() ) {
            if ( xml.name() == "downloadlink" ) {
                item.setUrl( xml.readElementText() );
            }
        }
    }
    return item;
}
