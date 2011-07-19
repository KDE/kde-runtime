/*
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
#include <KServiceTypeTrader>

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
    resourcewatcher->addType(NFO::FileDataObject());
    resourcewatcher->start();

    connect( resourcewatcher, SIGNAL( propertyAdded(Nepomuk::Resource, Nepomuk::Types::Property, QVariant) ),
            this, SLOT ( test(Nepomuk::Resource) ) );
    connect( resourcewatcher, SIGNAL( propertyRemoved(Nepomuk::Resource, Nepomuk::Types::Property, QVariant) ),
            this, SLOT ( test(Nepomuk::Resource) ) );
}

Nepomuk::WriteBackService::~WriteBackService()
{
}

void Nepomuk::WriteBackService::test(const Nepomuk::Resource & resource)
{
    kDebug()<<"Test method executed";
    if(resource.hasType(NFO::FileDataObject()))
    {
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
    else
    {
        QStringList subQueries( "'*' in [X-Nepomuk-ResourceTypes]" );
        for( int i = 0; i < (resource.types()).count(); ++i ) {
            subQueries << QString( "'%1' in [X-Nepomuk-ResourceTypes]" ).arg( ((resource.types()).at(i)).toString() );
        }

        KService::List services
                = KServiceTypeTrader::self()->query( "Nepomuk/WritebackPlugin", subQueries.join( " or " ) );

        foreach(const KSharedPtr<KService>& service, services) {
            // Return the cached instance if we have one, create a new one else
            WritebackPlugin* plugin = service->createInstance<WritebackPlugin>();
            if ( plugin )
                plugin->writeback(resource.resourceUri());

            connect(plugin,SIGNAL(finished()),this,SLOT(finishWriteback()));
        }
    }
}

void Nepomuk::WriteBackService::finishWriteback()
{
    kDebug()<<"Writeback finished";
    return;
}


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::WriteBackService, "nepomukwritebackservice" )

#include "writebackservice.moc"
