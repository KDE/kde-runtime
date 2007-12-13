/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GSTHELPER_H__
#define __GSTHELPER_H__

namespace Phonon
{
namespace Gstreamer
{
class GstHelper
{
public:
    static QStringList extractProperties(GstElement *elem, const QString &value);
    static bool setProperty(GstElement *elem, const QString &propertyName, const QString &propertyValue);
    static QString property(GstElement *elem, const QString &propertyName);
    static QString name(GstObject *elem);
    static GstElement* createPluggablePlaybin();
};

} // ns Gstreamer
} // ns Phonon

#endif // __GSTHELPER_H__

