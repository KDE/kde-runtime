/*
   Copyright (C) 2010 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <KApplication>
#include <KAboutData>
#include <KMessageBox>
#include <KUrl>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>
#include <KCmdLineArgs>
#include <KCmdLineOptions>

#include <QtGui/QLineEdit>
#include <QtGui/QInputDialog>

#include <Nepomuk/Query/Query>

#include "userqueries.h"


int main( int argc, char** argv )
{
    KAboutData aboutData( "kio_nepomuksearch_userquery_helper",
                          "kio_nepomuksearch",
                          ki18n("nepomuksearch helper"),
                          "0.1",
                          ki18n("Helper application which saves user queries"),
                          KAboutData::License_GPL,
                          ki18n("(c) 2010, Sebastian Trüg"),
                          KLocalizedString(),
                          "http://nepomuk.kde.org" );
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer"), "trueg@kde.org");
    aboutData.setProgramIconName( "nepomuk" );

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
	options.add("+url", ki18n("The query URL to save as a user query"));
    KCmdLineArgs::addCmdLineOptions( options );

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app;

    KUrl url = args->url( 0 );
    if ( url.protocol() == QLatin1String( "nepomuksearch" ) ) {
        Nepomuk::UserQueryUrlList userQueries;
        QString title;
        if ( userQueries.containsQueryUrl( url ) )
            title = QInputDialog::getText( 0,
                                           i18n( "Renaming User Query" ),
                                           i18n( "Please enter a new title for the query" ),
                                           QLineEdit::Normal,
                                           Nepomuk::Query::Query::titleFromQueryUrl( *userQueries.findQueryUrl( url ) ) );
        else
            title = QInputDialog::getText( 0,
                                           i18n( "Saving User Query" ),
                                           i18n( "Please enter a title for the query you are about to save" ),
                                           QLineEdit::Normal,
                                           Nepomuk::Query::Query::titleFromQueryUrl( url ) );
        if ( !title.isEmpty() ) {
            userQueries.addUserQuery( url, title );
        }
    }
    else {
        KMessageBox::error( 0, i18n( "Not a nepomuksearch URL: %1", url.prettyUrl() ) );
    }
}
