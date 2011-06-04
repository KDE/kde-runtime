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


#include<kpluginfactory.h>

#include<QStringList>
#include <QFile>
#include <QUrl>

#include<taglib/fileref.h>
#include<taglib/tag.h>
#include<taglib/tstring.h>

#include<Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NCO>

#include <Nepomuk/Variant>

#include<mywritebackplugin.h>
using namespace Nepomuk::Vocabulary;
Nepomuk::MyWritebackPlugin::MyWritebackPlugin(QObject* parent,const QList<QVariant>&): WritebackPlugin(parent)

{

}

Nepomuk::MyWritebackPlugin::~MyWritebackPlugin()
{

}


void Nepomuk::MyWritebackPlugin::doWriteback(const QUrl& url)
{
    TagLib::FileRef f(QFile::encodeName( url.toLocalFile()).data());
    f.tag()->setTitle("Good God");
    f.save();
    /*
    Nepomuk::Resource resource(url);
    if(resource.exists())
    {
        // creatin  Nepomuk::Resource resource(KUrl(url));
        TagLib::FileRef f(QFile::encodeName( url.toLocalFile()).data());
        // just an example

        if((resource.property(NIE::title())).isString())
        {
            QString title = (resource.property(NIE::title())).toString();
            if(title != TStringToQString(f.tag()->title()))
            {
                f.tag()->setTitle(Q4StringToTString(title));
                f.save();
            }
        }
        if((resource.property(NMM::musicAlbum())).isValid())
        {
            if((((resource.property(NMM::musicAlbum())).toResource()).property(NIE::title())).isString())
            {
                QString album =(((resource.property(NMM::musicAlbum())).toResource()).property(NIE::title())).toString();
                if(album != TStringToQString(f.tag()->album()))
                {
                    f.tag()->setAlbum(Q4StringToTString(album));
                    f.save();
                }
            }
        }
        if((resource.property(NIE::comment())).isString())
        {
            QString comment = (resource.property(NIE::comment())).toString();
            if(comment != TStringToQString(f.tag()->comment()))
            {
                f.tag()->setComment(Q4StringToTString(comment));
                f.save();
            }
        }
        if((resource.property(NMM::genre()).isString()))
        {
            QString genre = (resource.property(NMM::genre())).toString();
            if(genre != TStringToQString(f.tag()->genre()))
            {
                f.tag()->setGenre(Q4StringToTString(genre));
                f.save();
            }
        }
        if((resource.property(NMM::trackNumber()).isInt()))
        {
            uint track = (resource.property(NMM::trackNumber())).toInt();
            if(track != f.tag()->track())
            {
                f.tag()->setTrack(track);
                f.save();
            }
        }
        if((resource.property(NMM::performer())).isValid())
        {
            if(((((resource.property(NMM::performer())).toResource()).property(NCO::fullname())).isString()))
            {
                QString artist =(((resource.property(NMM::performer())).toResource()).property(NCO::fullname())).toString();
                if(artist != TStringToQString(f.tag()->artist()))
                {
                    f.tag()->setArtist(Q4StringToTString(artist));
                    f.save();
                }
            }
        }*/
     emitFinished();
    }
//}
NEPOMUK_EXPORT_WRITEBACK_PLUGIN(Nepomuk::MyWritebackPlugin,"nepomuk_writeback_taglib")
//#include "mywritebackplugin.moc"
