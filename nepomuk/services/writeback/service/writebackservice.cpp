/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Smit Shah <who828@gmail.com>

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
#include "writebackplugin.h"

#include <kservicetypetrader.h>
#include <KDebug>
#include <KUrl>
#include <kservice.h>
#include <KService>

#include <QString>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include<taglib/tstring.h>

#include<Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Variant>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Types/Property>

using namespace Nepomuk::Vocabulary;

Nepomuk::WriteBackService::WriteBackService( QObject* parent, const QList< QVariant >& )
    : Service(parent)
{
    kDebug();

}

Nepomuk::WriteBackService::~WriteBackService()
{
}

void Nepomuk::WriteBackService::test(const QString& url)
{
   // KUrl url_(url);
    //Nepomuk::Resource resource(url_);

  //  const QStringList mimetypes = resource.property(NIE::mimeType()).toStringList();
    //if(!mimetypes.isEmpty())
    {
       // QStringList subQueries;//( "'*' in [X-Nepomuk-MimeTypes]" );
        //for( int i = 0; i < mimetypes.count(); ++i ) {
          //  subQueries << QString( "'%1' in [X-Nepomuk-MimeTypes]" ).arg( mimetypes[i]);
        }
       KService::List services;
        KServiceTypeTrader* trader = KServiceTypeTrader::self();
//        QString mimetype = mimetypes.first();
//        QString y =" in MimeTypes";
//        mimetype += y;
        services = trader->query("Nepomuk/WritebackPlugin");
       // WritebackPlugin* plugin= KServiceTypeTrader::createInstanceFromQuery<WritebackPlugin>("Nepomuk/WritebackPlugin",constraint, this);
        foreach(const KSharedPtr<KService>& service, services) {
            WritebackPlugin* plugin = service-> createInstance<WritebackPlugin>();

   if (!plugin)
        {
            kError(5001) << "read write part" << service->createInstance<WritebackPlugin>();

            //plugin->writeback(KUrl(url));
        }

        }

        // Nepomuk::MyWritebackPlugin d;
        // d.doWriteback(KUrl(url));


    }
//}


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::WriteBackService, "nepomukwritebackservice" )

#include "writebackservice.moc"
