
#ifndef KWALLET_CUSTOM_QTEST
#define KWALLET_CUSTOM_QTEST
#include <QtTest/QtTest>
#include <stdlib.h>
#include <assert.h>
#include <kaboutdata.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kurl.h>

#define QTEST_KDEMAIN_CORE_WITH_DBUS_DAEMON(TestObject) \
int main(int argc, char *argv[]) \
{ \
    setenv("LC_ALL", "C", 1); \
    assert( !QDir::homePath().isEmpty() ); \
    setenv("KDEHOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test" )), 1); \
    setenv("XDG_DATA_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/local") ), 1); \
    setenv("XDG_CONFIG_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/config") ), 1); \
    setenv("KDE_SKIP_KDERC", "1", 1); \
    unsetenv("KDE_COLOR_DEBUG"); \
    QFile::remove(QDir::homePath() + QString::fromLatin1("/.kde-unit-test/share/config/qttestrc"));  \
    QProcess dbus; \
    dbus.start("dbus-launch"); \
    dbus.waitForFinished(10000);    \
    QByteArray session = dbus.readLine(); \
    if (session.isEmpty()) { \
        qFatal("Couldn't execute new dbus session"); \
    } \
    int pos = session.indexOf('='); \
    setenv(session.left(pos), session.right(session.count() - pos - 1).trimmed(), 1); \
    session = dbus.readLine(); \
    pos = session.indexOf('='); \
    QByteArray pid = session.right(session.count() - pos - 1).trimmed(); \
    KAboutData aboutData( QByteArray("qttest"), QByteArray(), ki18n("KDE Test Program"), QByteArray("version") );  \
    KComponentData cData(&aboutData); \
    QCoreApplication app( argc, argv ); \
    app.setApplicationName( QLatin1String("qttest") ); \
    qRegisterMetaType<KUrl>(); /*as done by kapplication*/ \
    qRegisterMetaType<KUrl::List>(); \
    TestObject tc; \
    KGlobal::ref(); /* don't quit qeventloop after closing a mainwindow */ \
    int result = QTest::qExec( &tc, argc, argv ); \
    dbus.start("kill", QStringList() << "-9" << pid); \
    dbus.waitForFinished(); \
    return result; \
}
#endif //KWALLET_CUSTOM_QTEST