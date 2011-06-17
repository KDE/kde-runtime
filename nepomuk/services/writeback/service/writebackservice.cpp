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

    #include <kmimetypetrader.h>
    #include <KDebug>
    #include <KUrl>
    #include <KService>

    #include <QString>

    #include <taglib/fileref.h>
    #include <taglib/tag.h>
    #include<taglib/tstring.h>

    #include<Nepomuk/Resource>
    #include <Nepomuk/Vocabulary/NIE>
    #include <Nepomuk/Vocabulary/NFO>
    #include <Nepomuk/Variant>
    #include <Nepomuk/Types/Class>
    #include <Nepomuk/Types/Property>

    using namespace Nepomuk::Vocabulary;

    Nepomuk::WriteBackService::WriteBackService( QObject* parent, const QList< QVariant >& )
        : Service(parent)
    {
        ResourceWatcher* resourcewatcher;
        kDebug()<<"this part is executed";

        resourcewatcher  = new ResourceWatcher(this);
        resourcewatcher->addProperty(Types::Property(NFO::FileDataObject()));
        resourcewatcher->start();

        connect( resourcewatcher, SIGNAL( propertyAdded(Nepomuk::Resource, Types::Property,QVariant) ),
                 this, SLOT ( test(Nepomuk::Resource) ) );
        connect( resourcewatcher, SIGNAL( propertyRemoved(Nepomuk::Resource, Types::Property, QVariant) ),
                 this, SLOT ( test(Nepomuk::Resource) ) );
    }

    Nepomuk::WriteBackService::~WriteBackService()
    {
    }

    void Nepomuk::WriteBackService::test(const Nepomuk::Resource & resource)
    {
        kDebug()<<"lol";

        const QStringList mimetypes = resource.property(NIE::mimeType()).toStringList();
        if(!mimetypes.isEmpty())
        {
            QString  mimetype = mimetypes.first();
            WritebackPlugin* plugin= KMimeTypeTrader::createInstanceFromQuery<WritebackPlugin>(mimetype,"Nepomuk/WritebackPlugin",this);

            if (plugin)
            {
                plugin->writeback(resource.resourceUri());
            }
            else
            {
                kError(5001) << "read write part";
            }
        }
    }

    #include <kpluginfactory.h>
    #include <kpluginloader.h>

    NEPOMUK_EXPORT_SERVICE( Nepomuk::WriteBackService, "nepomukwritebackservice" )

    #include "writebackservice.moc"
