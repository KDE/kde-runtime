/*
 * main.cpp
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "dialoghelper.h"

#include <kaboutdata.h>
#include <KApplication>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <KPasswordDialog>

#include <QWeakPointer>

int main(int argc, char **argv)
{
    KAboutData aboutData("dialog_helper", "kio_archive", ki18n("kio_archive"),
                         0,
                         ki18n("Dialog helper for kio_archive"),
                         KAboutData::License_GPL,
                         ki18n("(c) 2012 basysKom GmbH (info@basyskom.com)"));

    aboutData.addAuthor(ki18n("Lamarque V. Souza"), KLocalizedString(), "Lamarque.Souza@basyskom.com");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("prompt <prompt text>", ki18n("Dialog text to show"));
    options.add("error-message <text>", ki18n("Error message in case password is incorrect"));
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KApplication app;

    const KCmdLineArgs * args = KCmdLineArgs::parsedArgs();

    QStringList a = args->allArguments();

    /*if (args->count() < 1) {
        args->usage();
        return -1;
    }*/

    QString prompt = args->getOption("prompt");
    QString errorMessage;
    if (args->isSet("error-message")) {
        errorMessage = args->getOption("error-message");
    }

    DialogHelper dialogHelper(prompt, errorMessage);

    return app.exec();
}
// vim: sw=4 et sts=4

