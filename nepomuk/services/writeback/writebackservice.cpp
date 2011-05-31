/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "writebackservice.h"
#include "mywritebackplugin.h"
#include <KDebug>

#include <QString>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include<taglib/tstring.h>

Nepomuk::WriteBackService::WriteBackService( QObject* parent, const QList< QVariant >& )
	: Service(parent)
{
    kDebug();

}

Nepomuk::WriteBackService::~WriteBackService()
{
}

void Nepomuk::WriteBackService::test(const QString url)
{
    Nepomuk::MyWritebackPlugin d= new MyWritebackPlugin();
d.doWriteback(url);
}
    //    QByteArray fileName = QFile::encodeName( url );
//    const char * encodedName = fileName.constData();
//    TagLib::FileRef f (encodedName);
//    f.tag()->setTitle(QStringToTString(title));
//    f.tag()->setArtist("life");
//    f.tag()->setAlbum("goon");
//    f.save();

//}

d.
#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::WriteBackService, "nepomukwritebackservice" )

#include "writebackservice.moc"
