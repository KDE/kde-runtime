/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#include "drkonqi.h"
#include "drkonqidialog.h"
#include "backtracewidget.h"

#include <KApplication>
#include <KCmdLineArgs>
#include <KAboutData>
#include <KLocalizedString>
#include <kdefakes.h>

#include <cstdlib>
#include <unistd.h>
#include <signal.h>

static const char version[] = "2.1.0";
static const char description[] = I18N_NOOP("The KDE Crash Handler gives the user feedback "
                                            "if a program has crashed.");

int main(int argc, char* argv[])
{
#ifndef Q_OS_WIN //krazy:exclude=cpp
// Drop privs.
    setgid(getgid());
    if (setuid(getuid()) < 0 && geteuid() != getuid()) {
        exit(255);
    }
#endif

    // Make sure that DrKonqi doesn't start DrKonqi when it crashes :-]
    setenv("KDE_DEBUG", "true", 1);

    KAboutData aboutData("drkonqi", 0, ki18n("The KDE Crash Handler"),
                         version, ki18n(description),
                         KAboutData::License_GPL,
                         ki18n("(C) 2000-2009, The DrKonqi Authors"));
    aboutData.addAuthor(ki18nc("@info:credit","Hans Petter Bieker"), KLocalizedString(),
                         "bieker@kde.org");
    aboutData.addAuthor(ki18nc("@info:credit","Dario Andres Rodriguez"), KLocalizedString(),
                         "andresbajotierra@gmail.com");
    aboutData.addAuthor(ki18nc("@info:credit","George Kiagiadakis"), KLocalizedString(),
                         "gkiagia@users.sourceforge.net");
    aboutData.addAuthor(ki18nc("@info:credit","A. L. Spehr"), KLocalizedString(),
                         "spehr@kde.org");
    aboutData.setProgramIconName("tools-report-bug");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("signal <number>", ki18nc("@info:shell","The signal number that was caught"));
    options.add("appname <name>", ki18nc("@info:shell","Name of the program"));
    options.add("apppath <path>", ki18nc("@info:shell","Path to the executable"));
    options.add("appversion <version>", ki18nc("@info:shell","The version of the program"));
    options.add("bugaddress <address>", ki18nc("@info:shell","The bug address to use"));
    options.add("programname <name>", ki18nc("@info:shell","Translated name of the program"));
    options.add("pid <pid>", ki18nc("@info:shell","The PID of the program"));
    options.add("startupid <id>", ki18nc("@info:shell","Startup ID of the program"));
    options.add("kdeinit", ki18nc("@info:shell","The program was started by kdeinit"));
    options.add("safer", ki18nc("@info:shell","Disable arbitrary disk access"));
    options.add("restarted", ki18nc("@info:shell","The program has already been restarted"));
    options.add("keeprunning", ki18nc("@info:shell","DrKonqi should try to keep the program alice and will try to receive the backtrace instantly"));
    KCmdLineArgs::addCmdLineOptions(options);

    KComponentData inst(KCmdLineArgs::aboutData());
    QApplication *qa =
        KCmdLineArgs::parsedArgs()->isSet("safer") ?
        new QApplication(KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv()) :
        new KApplication;
    qa->setApplicationName(inst.componentName());

    if (!DrKonqi::init()) {
        delete qa;
        return 1;
    }

    qa->setQuitOnLastWindowClosed(false);
    KGlobal::setAllowQuit(true);

    DrKonqiDialog *w = new DrKonqiDialog();
    if(KCmdLineArgs::parsedArgs()->isSet("keeprunning"))
    {
        w->backtraceWidget()->generateBacktrace();
    }
    w->show();
    int ret = qa->exec();

    DrKonqi::cleanup();
    delete qa;
    return ret;
}
