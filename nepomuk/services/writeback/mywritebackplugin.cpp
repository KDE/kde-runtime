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

#include<taglib/fileref.h>
#include<taglib/tag.h>

#include<mywritebackplugin.h>

Nepomuk::MyWritebackPlugin::MyWritebackPlugin(QObject* parent, const QVariantList&)
{


}

Nepomuk::MyWritebackPlugin::~MyWritebackPlugin()
{

}


void Nepomuk::MyWritebackPlugin::doWriteback(const QString& url)
{
    QByteArray fileName = QFile::encodeName( url );
    const char * encodedName = fileName.constData();
    // creating a reference of the file
    TagLib::FileRef f(encodedName);
    // just an example
    f.tag()->setAlbum("XYZ");
    f.tag()->setTitle("abc");
    f.tag()->setArtist("Who");
    f.tag()->setComment("this is the best song,ever !");

    emitFinsihed();
}



