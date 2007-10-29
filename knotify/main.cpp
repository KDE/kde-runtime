/*
   Copyright (C) 2005-2006 by Olivier Goffart <ogoffart at kde.org>


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


#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessage.h>
#include <kpassivepopupmessagehandler.h>

#include "knotify.h"


int main(int argc, char **argv)
{
    KAboutData aboutdata("knotify", "knotify4", ki18n("KNotify"),
                         "4.0", ki18n("KDE Notification Daemon"),
                         KAboutData::License_GPL, ki18n("(C) 1997-2006, KDE Developers"));
    aboutdata.addAuthor(ki18n("Olivier Goffart"),ki18n("Current Maintainer"),"ogoffart@kde.org");
    aboutdata.addAuthor(ki18n("Carsten Pfeiffer"),ki18n("Previous Maintainer"),"pfeiffer@kde.org");
    aboutdata.addAuthor(ki18n("Christian Esken"),KLocalizedString(),"esken@kde.org");
    aboutdata.addAuthor(ki18n("Stefan Westerfeld"),ki18n("Sound support"),"stefan@space.twc.de");
    aboutdata.addAuthor(ki18n("Charles Samuels"),ki18n("Previous Maintainer"),"charles@kde.org");
    aboutdata.addAuthor(ki18n("Allan Sandfeld Jensen"),ki18n("Porting to KDE 4"),"kde@carewolf.com");

    KCmdLineArgs::init( argc, argv, &aboutdata );
    KUniqueApplication::addCmdLineOptions();

    // initialize application
    if ( !KUniqueApplication::start() ) {
        kDebug() << "Running knotify found";
        return 0;
    }

    KUniqueApplication app;
    app.disableSessionManagement();
    
    /*
     * the default KMessageBoxMessageHandler will do messagesbox that notify
     * so we have a deadlock if one debug message is shown as messagebox.
     * that's why we're forced to change the default handler
     */
    KMessage::setMessageHandler( new KPassivePopupMessageHandler(0) );

    // start notify service
    KNotify notify;

    return app.exec();
}

