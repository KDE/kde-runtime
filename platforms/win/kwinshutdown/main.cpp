/* This file is part of the KDE Project

   Copyright (C) 2011 Ralf Habacker <ralf.habacker@freenet.de>
   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License version 2
   as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include <kdebug.h>
#include <KAboutData>
#include <Kapplication>
#include <KMessageBox>
#include <KCmdLineArgs>
#include <KGlobal>
#include <KProcess>
#include <QtDebug>

int main(int argc, char **argv)
{
    KAboutData about("kwinshutdown", "kde-runtime", ki18n("kwinshutdown"), "1.0",
                     ki18n("A helper tool to shutdown a running installation"),
                     KAboutData::License_GPL,
                     ki18n("(C) 2011 Ralf Habacker"));
    KCmdLineArgs::init( argc, argv, &about);

    KCmdLineOptions options;
    KCmdLineArgs::addCmdLineOptions( options ); // Add my own options.

    KComponentData a(&about);

    // Get application specific arguments
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app(true);

    int res = KMessageBox::warningContinueCancel(0,
        "Should I really shutdown all applications and processes of your recent installation ?"
        "\n\nPlease make sure you have saved all documents.",
        "Shutdown KDE");
    if (res == KMessageBox::Cancel)
        return 2;

    KProcess kinit;

    kinit << "kdeinit4" << "--shutdown";
    kinit.start();
    bool finish = kinit.waitForFinished(10000);
    QByteArray result = kinit.readAll();
    qDebug() << result;

    return finish ? 0 : 1;
}

// vim: ts=4 sw=4 et
