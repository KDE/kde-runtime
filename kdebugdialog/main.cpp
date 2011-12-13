/* This file is part of the KDE libraries
   Copyright (C) 2000, 2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kdebugdialog.h"
#include "klistdebugdialog.h"
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <QTextStream>
#include <klocale.h>
#include <kdebug.h>
#include <kuniqueapplication.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QFile>

static KAbstractDebugDialog::AreaMap readAreas()
{
    KAbstractDebugDialog::AreaMap areas;
    // Group 0 is not used anymore. kDebug() uses the area named after the appname.
    //areas.insert( "      0" /*cf rightJustified below*/, "0 (generic)" );

    const QString confAreasFile = KStandardDirs::locate("config", "kdebug.areas");
    QFile file( confAreasFile );
    if (!file.open(QIODevice::ReadOnly)) {
        kWarning() << "Couldn't open" << confAreasFile;
    } else {
        QString data;

        QTextStream ts(&file);
        ts.setCodec( "ISO-8859-1" );
        while (!ts.atEnd()) {
            data = ts.readLine().simplified();

            int pos = data.indexOf("#");
            if ( pos != -1 ) {
                data.truncate( pos );
                data = data.simplified();
            }

            if (data.isEmpty())
                continue;

            const int space = data.indexOf(' ');
            if (space == -1)
                kError() << "No space:" << data << endl;

            bool longOK;
            unsigned long number = data.left(space).toULong(&longOK);
            if (!longOK)
                kError() << "The first part wasn't a number : " << data << endl;

            const QString description = data.mid(space).simplified();

            // In the key, right-align the area number to 6 digits for proper sorting
            const QString key = QString::number(number).rightJustified(6);
            areas.insert( key, QString("%1 %2").arg(number).arg(description) );
        }
    }

    bool ok;
#ifndef NDEBUG
    // Builtin unittest for our expectations of QString::toInt
    QString("4a").toInt(&ok);
    Q_ASSERT(!ok);
#endif

    KConfig config("kdebugrc", KConfig::NoGlobals);
    Q_FOREACH(const QString& groupName, config.groupList()) {
        groupName.toInt(&ok);
        if (ok)
            continue; // we are not interested in old-style number-only groups
        areas.insert(groupName, groupName); // ID == description
    }

    return areas;
}

int main(int argc, char ** argv)
{
    KAboutData data( "kdebugdialog", 0, ki18n( "KDebugDialog"),
            "1.0", ki18n("A dialog box for setting preferences for debug output"),
            KAboutData::License_GPL, ki18n("Copyright 1999-2009, David Faure <email>faure@kde.org</email>"));
    data.addAuthor(ki18n("David Faure"), ki18n("Maintainer"), "faure@kde.org");
    data.setProgramIconName("tools-report-bug");
    KCmdLineArgs::init( argc, argv, &data );

    KCmdLineOptions options;
    options.add("fullmode", ki18n("Show the fully-fledged dialog instead of the default list dialog"));
    options.add("on <area>", ki18n(/*I18N_NOOP TODO*/ "Turn area on"));
    options.add("off <area>", ki18n(/*I18N_NOOP TODO*/ "Turn area off"));
    KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();
    KUniqueApplication app;
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KAbstractDebugDialog * dialog;
    if (args->isSet("fullmode")) {
        dialog = new KDebugDialog(readAreas());
    } else {
        KListDebugDialog * listdialog = new KListDebugDialog(readAreas());
        if (args->isSet("on"))
        {
            listdialog->activateArea( args->getOption("on").toUtf8(), true );
            /*listdialog->save();
              listdialog->config()->sync();
              return 0;*/
        } else if ( args->isSet("off") )
        {
            listdialog->activateArea( args->getOption("off").toUtf8(), false );
            /*listdialog->save();
              listdialog->config()->sync();
              return 0;*/
        }
        dialog = listdialog;
    }

    /* Show dialog */
    int nRet = dialog->exec();
    if( nRet == QDialog::Accepted )
    {
        dialog->save();
        dialog->config()->sync();
    }
    else
        dialog->config()->markAsClean();

    return 0;
}
