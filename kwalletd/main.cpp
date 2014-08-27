/**
  * This file is part of the KDE project
  * Copyright (C) 2008 Michael Leupold <lemma@confuego.org>
  * Copyright (C) 2014 Alejandro Fiestas Olivares <afiestas@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocale.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "kwalletd.h"
#include "backend/kwalletbackend.h" //For the hash size

#define BSIZE 1000
static int pipefd = 0;
static int socketfd = 0;
static bool isWalletEnabled()
{
	KConfig cfg("kwalletrc");
	KConfigGroup walletGroup(&cfg, "Wallet");
	return walletGroup.readEntry("Enabled", true);
}

//Waits until the PAM_MODULE sends the hash
static char *waitForHash()
{
    printf("kwalletd: Waiting for hash on %d-\n", pipefd);
    int totalRead = 0;
    int readBytes = 0;
    int attemps = 0;
    char *buf = (char*)malloc(sizeof(char) * PBKDF2_SHA512_KEYSIZE);
    memset(buf, '\0', PBKDF2_SHA512_KEYSIZE);
    while(totalRead != PBKDF2_SHA512_KEYSIZE) {
        readBytes = read(pipefd, buf + totalRead, PBKDF2_SHA512_KEYSIZE - totalRead);
        if (readBytes == -1 || attemps > 5) {
            free(buf);
            return NULL;
        }
        totalRead += readBytes;
        ++attemps;
    }

    close(pipefd);
    return buf;
}

//Waits until startkde sends the environment variables
static int waitForEnvironment()
{
    printf("kwalletd: waitingForEnvironment on: %d\n", socketfd);

    int s2;
    struct sockaddr_un remote;
    socklen_t t = sizeof(remote);
    if ((s2 = accept(socketfd, (struct sockaddr *)&remote, &t)) == -1) {
        fprintf(stdout, "kwalletd: Couldn't accept incoming connection\n");
        return -1;
    }
    printf("kwalletd: client connected\n");

    char str[BSIZE];
    memset(str, '\0', sizeof(char) * BSIZE);

    int chop = 0;
    FILE *s3 = fdopen(s2, "r");
    while(!feof(s3)) {
        if (fgets(str, BSIZE, s3)) {
            chop = strlen(str) - 1;
            str[chop] = '\0';
            putenv(strdup(str));
        }
    }
    printf("kwalletd: client disconnected\n");
    close(socketfd);
    return 1;
}

char* checkPamModule(int argc, char **argv)
{
    printf("Checking for pam module\n");
    char *hash = NULL;
    int x = 1;
    for (; x < argc; ++x) {
        if (strcmp(argv[x], "--pam-login") != 0) {
            continue;
        }
        printf("Got pam-login\n");
        argv[x] = NULL;
        x++;
        //We need at least 2 extra arguments after --pam-login
        if (x + 1 > argc) {
            printf("Invalid arguments (less than needed)\n");
            return NULL;
        }

        //first socket for the hash, comes from a pipe
        pipefd = atoi(argv[x]);
        argv[x] = NULL;
        x++;
        //second socket for environemtn, comes from a localsock
        socketfd = atoi(argv[x]);
        argv[x] = NULL;
        break;
    }

    if (!pipefd || !socketfd) {
        printf("Lacking a socket, pipe: %d, env:%d\n", pipefd, socketfd);
        return NULL;
    }

    hash = waitForHash();

    if (hash == NULL || waitForEnvironment() == -1) {
        printf("Hash or environment not received\n");
        return NULL;
    }

    return hash;
}

extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
    char *hash = NULL;
    if (getenv("PAM_KWALLET_LOGIN")) {
        hash = checkPamModule(argc, argv);
    }

    KAboutData aboutdata("kwalletd", 0, ki18n("KDE Wallet Service"),
                         "0.2", ki18n("KDE Wallet Service"),
                         KAboutData::License_LGPL, ki18n("(C) 2002-2008 George Staikos, Michael Leupold, Thiago Maceira, Valentin Rusu"));
    aboutdata.addAuthor(ki18n("Michael Leupold"),ki18n("Maintainer"),"lemma@confuego.org");
    aboutdata.addAuthor(ki18n("George Staikos"),ki18n("Former maintainer"),"staikos@kde.org");
    aboutdata.addAuthor(ki18n("Thiago Maceira"),ki18n("D-Bus Interface"),"thiago@kde.org");
    aboutdata.addAuthor(ki18n("Valentin Rusu"),ki18n("GPG backend support"),"kde@rusu.info");

    aboutdata.setProgramIconName("kwalletmanager");

    KCmdLineArgs::init( argc, argv, &aboutdata );
    KUniqueApplication::addCmdLineOptions();
    KUniqueApplication app;

    // This app is started automatically, no need for session management
    app.disableSessionManagement();
    app.setQuitOnLastWindowClosed( false );

    // check if kwallet is disabled
    if (!isWalletEnabled()) {
      kDebug() << "kwalletd is disabled!";
      return (0);
    }

    if (!KUniqueApplication::start())
    {
      kDebug() << "kwalletd is already running!";
      return (0);
    }

    kDebug() << "kwalletd started";
    KWalletD walletd;
    if (hash) {
        kDebug() << "LOGIN INSIDE!";
        QByteArray passHash(hash, PBKDF2_SHA512_KEYSIZE);
        int wallet = walletd.pamOpen(KWallet::Wallet::LocalWallet(), passHash, 0);
        kDebug() << "Wallet handler: " << wallet;
        free(hash);
    } else {
        kDebug() << "Not pam login";
    }
    return app.exec();
}
