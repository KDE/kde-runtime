/*************************************************************************************
 *  Copyright (C) 2014 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "kwalletexecuter.h"

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <QTest>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtNetwork/QLocalSocket>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusConnectionInterface>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#ifndef Q_OS_WIN
extern char **environ;
#endif

KWalletExecuter::KWalletExecuter(QObject* parent): QObject(parent)
{
    // Make sure wallet is enabled, otherwise test will fail
    KConfig cfg("kwalletrc");
    KConfigGroup cg( &cfg, "Wallet");
    cg.writeEntry("First Use", false);
    cg.writeEntry("Enabled", true);
}

void KWalletExecuter::pamOpen()
{
    QDBusMessage msg =
    QDBusMessage::createMethodCall("org.kde.kwalletd", "/modules/kwalletd", "org.kde.KWallet", "open");
    QVariantList args;
    qlonglong wid = 0;
    args << QLatin1String("kdewallet") << wid << QLatin1String("buh");
    msg.setArguments(args);
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);

    QVERIFY2(reply.type() != QDBusMessage::ErrorMessage, reply.errorMessage().toLocal8Bit());
    m_handler = reply.arguments().first().toInt();
    if (m_handler <= 0) {
        qFatal("Couldn't open the wallet via dbus");//We don't want the test to continue
    }
}

void KWalletExecuter::pamWrite(const QString &value) const
{
    QDBusMessage msg =
    QDBusMessage::createMethodCall("org.kde.kwalletd", "/modules/kwalletd", "org.kde.KWallet", "writePassword");
    QVariantList args;
    args << m_handler
        << QLatin1String("Passwords")
        << QLatin1String("foo")
        << value
        << QLatin1String("buh");
    msg.setArguments(args);
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);

    QVERIFY2(reply.type() != QDBusMessage::ErrorMessage, reply.errorMessage().toLocal8Bit());
    int ret = reply.arguments().first().toInt();
    QCOMPARE(ret, 0);
}

void KWalletExecuter::pamRead(const QString &value) const
{
    QDBusMessage msg =
    QDBusMessage::createMethodCall("org.kde.kwalletd", "/modules/kwalletd", "org.kde.KWallet", "readPassword");
    QVariantList args;
    args << m_handler
        << QLatin1String("Passwords")
        << QLatin1String("foo")
        << QLatin1String("buh");
    msg.setArguments(args);
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);

    QVERIFY2(reply.type() != QDBusMessage::ErrorMessage, reply.errorMessage().toLocal8Bit());
    const QString password = reply.arguments().first().toString();
    QCOMPARE(password, value);
}

void KWalletExecuter::execute_kwallet(int toWalletPipe[2], int envSocket)
{
    int x = 3;
    //Close fd that are not of interest of kwallet
    for (; x < 64; ++x) {
        if (x != toWalletPipe[0] && x != envSocket) {
            close (x);
        }
    }

    close (toWalletPipe[1]);

    char pipeInt[2];
    sprintf(pipeInt, "%d", toWalletPipe[0]);
    char sockIn[2];
    sprintf(sockIn, "%d", envSocket);

    qDebug() << "Executing kwalletd";
    QByteArray wallet = KStandardDirs::locate("exe", "kwalletd").toLocal8Bit();
    char *args[] = {wallet.data(), (char*)"--pam-login", pipeInt, sockIn, NULL};
    execve(args[0], args, environ);
    qFatal("Couldn't execute kwalletd");
}

void KWalletExecuter::execute()
{
//Preparing sockets, we will share them with kwalletd
    int toWalletPipe[2] = { -1, -1};
    if (pipe(toWalletPipe) < 0) {
        qFatal("Couldn't craete pipes");
    }

    int envSocket;
    if ((envSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        qFatal("Couldn't create socket");
    }

    QByteArray sock = KStandardDirs::locateLocal("socket", QLatin1String("test.socket")).toLocal8Bit();
    struct sockaddr_un local;
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, sock.constData());
    unlink(local.sun_path);//Just in case it exists from a previous login

    int len;
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(envSocket, (struct sockaddr *)&local, len) == -1) {
        qFatal("Couldn't bind the socket");
    }

    if (listen(envSocket, 5) == -1) {
        qFatal("Couldn't listen into the socket");
    }

    qputenv("PAM_KWALLET_LOGIN", "1");
    pid_t pid;
    switch (pid = fork ()) {
    case -1:
        qFatal("Couldn't fork to execv kwalletd");

    //Child fork, will contain kwalletd
    case 0:
        execute_kwallet(toWalletPipe, envSocket);
        /* Should never be reached */
        break;

    //Parent
    default:
        break;
    };

    close(toWalletPipe[0]);//Read end of the pipe, we will only use the write

    QByteArray hash =
    QByteArray::fromHex("1f1e7736894243657ef4a5274211b9a62703494c286e1699418a36e7ecf31d37319644db63d9fb26eb57cd9b7fea3e88bc18312480ba54f4");
    write(toWalletPipe[1], hash.constData(), 56);

    QLocalSocket *socket = new QLocalSocket(this);
    socket->connectToServer(sock);
    //No need to send any environment vars, the env is already ok
    socket->close();

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kwalletd")) {
        qDebug() << "AAAAAAAA";
        QEventLoop loop;
        QDBusServiceWatcher *watcher = new QDBusServiceWatcher("org.kde.kwalletd",
                                                            QDBusConnection::sessionBus(),
                                                            QDBusServiceWatcher::WatchForRegistration);
        connect(watcher, SIGNAL(serviceRegistered(QString)), &loop, SLOT(quit()));
        loop.exec();
    }
}

void KWalletExecuter::cleanupTestCase()
{
    QDBusMessage msg =
    QDBusMessage::createMethodCall("org.kde.kwalletd", "/MainApplication", "org.kde.KApplication", "quit");
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);

    Q_ASSERT(reply.type() != QDBusMessage::ErrorMessage);
}
