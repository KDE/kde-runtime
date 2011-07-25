/*
    Copyright (C) 2011  Smit Shah <Who828@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <libkexiv2/kexiv2.h>

#include <QFile>
#include <QUrl>

#include <Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NEXIF>
#include <Nepomuk/Variant>

#include "Exiv2WritebackPlugin.h"

using namespace Nepomuk::Vocabulary;

Nepomuk::Exiv2WritebackPlugin::Exiv2WritebackPlugin(QObject* parent,const QList<QVariant>&): WritebackPlugin(parent)
{

}

Nepomuk::Exiv2WritebackPlugin::~Exiv2WritebackPlugin()
{

}

void Nepomuk::Exiv2WritebackPlugin::doWriteback(const QUrl& url)
{
    Nepomuk::Resource resource(url.toLocalFile());
    if(resource.isValid())
    {
        KExiv2Iface::KExiv2 image;
        image.load(url.toLocalFile());

        if((resource.property(NEXIF::dateTime())).isValid())
        {
            QDateTime datetime = (resource.property(NEXIF::dateTime())).toDateTime();
            if(datetime != QDateTime::fromString(image.getExifTagString("Exif.Image.DateTime"), QLatin1String("yyyy:MM:dd hh:mm:ss")))
            {
                image.setExifTagString("Exif.Image.DateTime",datetime.toString(QLatin1String("yyyy:MM:dd hh:mm:ss")));
            }
        }

        if((resource.property(NEXIF::dateTimeDigitized())).isValid())
        {
            QDateTime datetime = (resource.property(NEXIF::dateTimeDigitized())).toDateTime();
            if(datetime != QDateTime::fromString(image.getExifTagString("Exif.Photo.DateTimeDigitized"),QLatin1String("yyyy:MM:dd hh:mm:ss")))
            {
                image.setExifTagString("Exif.Photo.DateTimeDigitized",datetime.toString(QLatin1String("yyyy:MM:dd hh:mm:ss")));
            }
        }
        if((resource.property(NEXIF::dateTimeOriginal())).isValid())
        {
            QDateTime datetime = (resource.property(NEXIF::dateTimeOriginal())).toDateTime();
            if(datetime !=QDateTime::fromString (image.getExifTagString("Exif.Image.DateTimeOriginal"),QLatin1String("yyyy:MM:dd hh:mm:ss")))
            {
                image.setExifTagString("Exif.Image.DateTimeOriginal",datetime.toString(QLatin1String("yyyy:MM:dd hh:mm:ss")));
            }
        }

        if((resource.property(NEXIF::imageDescription())).isValid())
        {
            QString imagedes = (resource.property(NEXIF::imageDescription())).toString();
            if(imagedes != image.getExifTagString("Exif.Image.ImageDescription"))
            {
                image.setExifTagString("Exif.Image.ImageDescription",imagedes);
            }
        }

        if((resource.property(NEXIF::orientation())).isValid())
        {
            QString imageorin = (resource.property(NEXIF::orientation())).toString();
            if(imageorin != image.getExifTagString("Exif.Image.Orientation"))
            {
                image.setExifTagString("Exif.Image.Orientation",imageorin);
            }
        }
        image.applyChanges();
        emitFinished();
    }
}

NEPOMUK_EXPORT_WRITEBACK_PLUGIN(Nepomuk::Exiv2WritebackPlugin,"nepomuk_writeback_exiv2")
#include "Exiv2WritebackPlugin.moc"
