/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-11 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "indexer.h"
#include "../util.h"
#include "../../../servicestub/priority.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocale>
#include <KComponentData>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include <KDebug>
#include <KUrl>

int main(int argc, char *argv[])
{
    lowerIOPriority();
    lowerSchedulingPriority();
    lowerPriority();

    KAboutData aboutData("nepomukindexer", 0, ki18n("NepomukIndexer"),
                         "1.0",
                         ki18n("NepomukIndexer indexes the contents of a file and saves the results in Nepomuk"),
                         KAboutData::License_LGPL_V2,
                         ki18n("(C) 2011, Vishesh Handa"));
    aboutData.addAuthor(ki18n("Vishesh Handa"), ki18n("Current maintainer"), "handa.vish@gmail.com");
    
    KCmdLineArgs::init(argc, argv, &aboutData);
    
    KCmdLineOptions options;
    options.add("uri <uri>", ki18n("The URI provided will be forced on the resource"));
    options.add("mtime <time>", ki18n("The modification time of the resource in time_t format"));
    options.add("+[url]", ki18n("The URL of the file to be indexed"));
    options.add("clear", ki18n("Remove all indexed data of the URL provided"));
    
    KCmdLineArgs::addCmdLineOptions(options);   
    const KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    // Application
    QCoreApplication app( argc, argv );
    KComponentData data( aboutData, KComponentData::RegisterAsMainComponent );
    
    const KUrl uri = args->getOption("uri");
    const uint mtime = args->getOption("mtime").toUInt();

    if( args->count() == 0 ) {
        Nepomuk::Indexer indexer;
        indexer.indexStdin( uri, mtime );
        return 0;
    }
    else if( args->isSet("clear") ) {
        Nepomuk::clearLegacyIndexedDataForUrls( args->url(0) );
        Nepomuk::clearIndexedData( args->url(0) );
        kDebug() << "Removed indexed data for" << args->url(0);
        return 0;
    }
    else {
        Nepomuk::Indexer indexer;
        indexer.indexFile( args->url(0), uri, mtime );
        kDebug() << "Indexed data for" << args->url(0);
        return 0;
    }
}
