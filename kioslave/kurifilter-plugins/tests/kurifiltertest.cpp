/*
 *  Copyright (C) 2002, 2003 David Faure   <faure@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kurifiltertest.h"
#include "qtest_kde.h"

#include <kurifilter.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <ksharedconfig.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kdefakes.h>
#include <kconfiggroup.h>

#include <QtCore/QDir>
#include <QtCore/QRegExp>
#include <QtNetwork/QHostInfo>

#include <iostream>

QTEST_KDEMAIN( KUriFilterTest, NoGUI )

static const char * const s_uritypes[] = { "NetProtocol", "LOCAL_FILE", "LOCAL_DIR", "EXECUTABLE", "HELP", "SHELL", "BLOCKED", "ERROR", "UNKNOWN" };
#define NO_FILTERING -2

static void filter( const char* u, const char * expectedResult = 0, int expectedUriType = -1, const QStringList& list = QStringList(), const char * abs_path = 0, bool checkForExecutables = true )
{
    QString a = QString::fromUtf8( u );
    KUriFilterData * filterData = new KUriFilterData;
    filterData->setData( a );
    filterData->setCheckForExecutables( checkForExecutables );

    if( abs_path )
    {
        filterData->setAbsolutePath( QLatin1String( abs_path ) );
        kDebug() << "Filtering: " << a << " with abs_path=" << abs_path;
    }
    else
        kDebug() << "Filtering: " << a;

    if (KUriFilter::self()->filterUri(*filterData, list))
    {
        if ( expectedUriType == NO_FILTERING ) {
            kError() << u << "Did not expect filtering. Got" << filterData->uri();
            QVERIFY( expectedUriType != NO_FILTERING ); // fail the test
        }

        // Copied from minicli...
        QString cmd;
        KUrl uri = filterData->uri();

        if ( uri.isLocalFile() && !uri.hasRef() && uri.query().isEmpty() &&
             (filterData->uriType() != KUriFilterData::NetProtocol))
            cmd = uri.toLocalFile();
        else
            cmd = uri.url();

        switch( filterData->uriType() )
        {
            case KUriFilterData::LocalFile:
            case KUriFilterData::LocalDir:
                kDebug() << "*** Result: Local Resource =>  '"
                          << filterData->uri().toLocalFile() << "'" << endl;
                break;
            case KUriFilterData::Help:
                kDebug() << "*** Result: Local Resource =>  '"
                          << filterData->uri().url() << "'" << endl;
                break;
            case KUriFilterData::NetProtocol:
                kDebug() << "*** Result: Network Resource => '"
                          << filterData->uri().url() << "'" << endl;
                break;
            case KUriFilterData::Shell:
            case KUriFilterData::Executable:
                if( filterData->hasArgsAndOptions() )
                    cmd += filterData->argsAndOptions();
                kDebug() << "*** Result: Executable/Shell => '" << cmd << "'";
                break;
            case KUriFilterData::Error:
                kDebug() << "*** Result: Encountered error => '" << cmd << "'";
                kDebug() << "Reason:" << filterData->errorMsg();
                break;
            default:
                kDebug() << "*** Result: Unknown or invalid resource.";
        }

        if ( expectedUriType != -1 && expectedUriType != filterData->uriType() )
        {
            kError() << u << "Got URI type" << s_uritypes[filterData->uriType()]
                      << "expected" << s_uritypes[expectedUriType];
            QCOMPARE( s_uritypes[filterData->uriType()],
                      s_uritypes[expectedUriType] );
        }

        if ( expectedResult )
        {
            // Hack for other locales than english, normalize google hosts to google.com
            cmd = cmd.replace( QRegExp( "www\\.google\\.[^/]*/" ), "www.google.com/" );
            QString expected = QString::fromUtf8( expectedResult );
            if (cmd != expected) {
                kError() << u;
                QCOMPARE( cmd, expected );
            }
        }
    }
    else
    {
        if ( expectedUriType == NO_FILTERING )
            kDebug() << "*** No filtering required.";
        else
        {
            kDebug() << "*** Could not be filtered.";
            if( expectedUriType != filterData->uriType() )
            {
                QCOMPARE( s_uritypes[filterData->uriType()],
                          s_uritypes[expectedUriType] );
            }
        }
    }

    delete filterData;
    kDebug() << "-----";
}

static void testLocalFile( const QString& filename )
{
    QFile tmpFile( filename ); // Yeah, I know, security risk blah blah. This is a test prog!

    if ( tmpFile.open( QIODevice::ReadWrite ) )
    {
        QByteArray fname = QFile::encodeName( tmpFile.fileName() );
        filter(fname, fname, KUriFilterData::LocalFile);
        tmpFile.close();
        tmpFile.remove();
    }
    else
        kDebug() << "Couldn't create " << tmpFile.fileName() << ", skipping test";
}

static char s_delimiter = ':'; // the alternative is ' '

KUriFilterTest::KUriFilterTest()
{
    minicliFilters << "kshorturifilter" << "kurisearchfilter" << "localdomainurifilter";
    qtdir = getenv("QTDIR");
    home = getenv("HOME");
    kdehome = getenv("KDEHOME");
}

void KUriFilterTest::init()
{
    kDebug() ;
    setenv( "KDE_FORK_SLAVES", "yes", true ); // simpler, for the final cleanup

    // Allow testing of the search engine using both delimiters...
    const char* envDelimiter = ::getenv( "KURIFILTERTEST_DELIMITER" );
    if ( envDelimiter )
        s_delimiter = envDelimiter[0];

    // Many tests check the "default search engine" feature.
    // There is no default search engine by default (since it was annoying when making typos),
    // so the user has to set it up, which we do here.
    {
      KConfigGroup cfg( KSharedConfig::openConfig( "kuriikwsfilterrc", KConfig::SimpleConfig ), "General" );
      cfg.writeEntry( "DefaultWebShortcut", "google" );
      cfg.writeEntry( "Verbose", true );
      cfg.writeEntry( "KeywordDelimiter", QString(s_delimiter) );
      cfg.sync();
    }

    // Enable verbosity for debugging
    {
      KSharedConfig::openConfig("kshorturifilterrc", KConfig::SimpleConfig )->group(QString()).writeEntry( "Verbose", true );
    }

    KStandardDirs::makeDir( kdehome+"/urifilter" );
}

void KUriFilterTest::noFiltering()
{
    // URI that should require no filtering
    filter( "http://www.kde.org", "http://www.kde.org", KUriFilterData::NetProtocol );
    filter( "http://www.kde.org/developer//index.html", "http://www.kde.org/developer//index.html", KUriFilterData::NetProtocol );
    filter( "file:///", "file:///", KUriFilterData::NetProtocol );
    filter( "file:///etc", "file:///etc", KUriFilterData::NetProtocol );
    filter( "file:///etc/passwd", "file:///etc/passwd", KUriFilterData::NetProtocol );
}

void KUriFilterTest::localFiles()
{
    filter( "/", "/", KUriFilterData::LocalDir );
    filter( "/", "/", KUriFilterData::LocalDir, QStringList( "kshorturifilter" ) );
    if (QFile::exists(QDir::homePath() + QLatin1String("/.bashrc")))
        filter( "~/.bashrc", QDir::homePath().toLocal8Bit()+"/.bashrc", KUriFilterData::LocalFile, QStringList( "kshorturifilter" ) );
    filter( "~", QDir::homePath().toLocal8Bit(), KUriFilterData::LocalDir, QStringList( "kshorturifilter" ), "/tmp" );
    filter( "~bin", 0, KUriFilterData::LocalDir, QStringList( "kshorturifilter" ) );
    filter( "~does_not_exist", 0, KUriFilterData::Error, QStringList( "kshorturifilter" ) );

    // Absolute Path tests for kshorturifilter
    const QStringList kshorturifilter( QString("kshorturifilter") );
    filter( "./", kdehome+"/share", KUriFilterData::LocalDir, kshorturifilter, kdehome+"/share/" ); // cleanPath removes the trailing slash
    filter( "../", kdehome, KUriFilterData::LocalDir, kshorturifilter, kdehome+"/share" );
    filter( "config", kdehome+"/share/config", KUriFilterData::LocalDir, kshorturifilter, kdehome+"/share" );
    // Invalid URLs
    filter( "http://a[b]", "http://a[b]", KUriFilterData::Unknown, kshorturifilter, "/" );
}

void KUriFilterTest::refOrQuery()
{
    // URL with reference
    filter( "http://www.kde.org/index.html#q8", "http://www.kde.org/index.html#q8", KUriFilterData::NetProtocol );
    // local file with reference
    filter( "file:/etc/passwd#q8", "file:///etc/passwd#q8", KUriFilterData::LocalFile );
    filter( "file:///etc/passwd#q8", "file:///etc/passwd#q8", KUriFilterData::LocalFile );
    filter( "/etc/passwd#q8", "file:///etc/passwd#q8", KUriFilterData::LocalFile );
    // local file with query (can be used by javascript)
    filter( "file:/etc/passwd?foo=bar", "file:///etc/passwd?foo=bar", KUriFilterData::LocalFile );
    testLocalFile( "/tmp/kurifiltertest?foo" ); // local file with ? in the name (#58990)
    testLocalFile( "/tmp/kurlfiltertest#foo" ); // local file with '#' in the name
    testLocalFile( "/tmp/kurlfiltertest#foo?bar" ); // local file with both
    testLocalFile( "/tmp/kurlfiltertest?foo#bar" ); // local file with both, the other way round
}

void KUriFilterTest::shortUris()
{
    // hostnames are lowercased by KUrl
    filter( "http://www.myDomain.commyPort/ViewObjectRes//Default:name=hello",
            "http://www.mydomain.commyport/ViewObjectRes//Default:name=hello", KUriFilterData::NetProtocol);
    filter( "ftp://ftp.kde.org", "ftp://ftp.kde.org", KUriFilterData::NetProtocol );
    filter( "ftp://username@ftp.kde.org:500", "ftp://username@ftp.kde.org:500", KUriFilterData::NetProtocol );

    // ShortURI/LocalDomain filter tests. NOTE: any of these tests can fail
    // if you have specified your own patterns in kshorturifilterrc. For
    // examples, see $KDEDIR/share/config/kshorturifilterrc .
    filter( "linuxtoday.com", "http://linuxtoday.com", KUriFilterData::NetProtocol );
    filter( "LINUXTODAY.COM", "http://linuxtoday.com", KUriFilterData::NetProtocol );
    filter( "kde.org", "http://kde.org", KUriFilterData::NetProtocol );
    filter( "ftp.kde.org", "ftp://ftp.kde.org", KUriFilterData::NetProtocol );
    filter( "ftp.kde.org:21", "ftp://ftp.kde.org:21", KUriFilterData::NetProtocol );
    filter( "cr.yp.to", "http://cr.yp.to", KUriFilterData::NetProtocol );
    filter( "www.kde.org:21", "http://www.kde.org:21", KUriFilterData::NetProtocol );
    filter( "foobar.local:8000", "http://foobar.local:8000", KUriFilterData::NetProtocol );
    filter( "foo@bar.com", "mailto:foo@bar.com", KUriFilterData::NetProtocol );
    filter( "firstname.lastname@x.foo.bar", "mailto:firstname.lastname@x.foo.bar", KUriFilterData::NetProtocol );
    filter( "www.123.foo", "http://www.123.foo", KUriFilterData::NetProtocol );
    filter( "user@www.123.foo:3128", "http://user@www.123.foo:3128", KUriFilterData::NetProtocol );
    filter( "ftp://user@user@www.123.foo:3128", "ftp://user%40user@www.123.foo:3128", KUriFilterData::NetProtocol );
    filter( "user@user@www.123.foo:3128", "http://user%40user@www.123.foo:3128", KUriFilterData::NetProtocol );

    // IPv4 address formats...
    filter( "user@192.168.1.0:3128", "http://user@192.168.1.0:3128", KUriFilterData::NetProtocol );
    filter( "127.0.0.1", "http://127.0.0.1", KUriFilterData::NetProtocol );
    filter( "127.0.0.1:3128", "http://127.0.0.1:3128", KUriFilterData::NetProtocol );
    filter( "127.1", "http://127.1", KUriFilterData::NetProtocol );
    filter( "127.0.1", "http://127.0.1", KUriFilterData::NetProtocol );

    // IPv6 address formats (taken from RFC 2732)...
    filter("[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html", "http://[fedc:ba98:7654:3210:fedc:ba98:7654:3210]:80/index.html", KUriFilterData::NetProtocol );
    filter("[1080:0:0:0:8:800:200C:417A]/index.html", "http://[1080:0:0:0:8:800:200c:417a]/index.html", KUriFilterData::NetProtocol );
    filter("[3ffe:2a00:100:7031::1]", "http://[3ffe:2a00:100:7031::1]", KUriFilterData::NetProtocol );
    filter("[1080::8:800:200C:417A]/foo", "http://[1080::8:800:200c:417a]/foo", KUriFilterData::NetProtocol );
    filter("[::192.9.5.5]/ipng", "http://[::192.9.5.5]/ipng", KUriFilterData::NetProtocol );
    filter("[::FFFF:129.144.52.38]:80/index.html", "http://[::ffff:129.144.52.38]:80/index.html", KUriFilterData::NetProtocol );
    filter("[2010:836B:4179::836B:4179]", "http://[2010:836b:4179::836b:4179]", KUriFilterData::NetProtocol );

    // Local domain filter - If you uncomment these test, make sure you
    // you adjust it based on the localhost entry in your /etc/hosts file.
    // filter( "localhost:3128", "http://localhost.localdomain:3128", KUriFilterData::NetProtocol );
    // filter( "localhost", "http://localhost.localdomain", KUriFilterData::NetProtocol );
    // filter( "localhost/~blah", "http://localhost.localdomain/~blah", KUriFilterData::NetProtocol );

    filter( "user@host.domain", "mailto:user@host.domain", KUriFilterData::NetProtocol ); // new in KDE-3.2

    // Windows style SMB (UNC) URL. Should be converted into the valid smb format...
    filter( "\\\\mainserver\\share\\file", "smb://mainserver/share/file" , KUriFilterData::NetProtocol );

    // KDE3: was not be filtered at all. All valid protocols of this form were be ignored.
    // KDE4: parsed as "network protocol", seems fine to me (DF)
    filter( "ftp:" , "ftp:", KUriFilterData::NetProtocol );
    filter( "http:" , "http:", KUriFilterData::NetProtocol );

    // The default search engine is set to 'Google'
    //this may fail if your DNS knows domains KDE or FTP
    filter( "gg:", "", KUriFilterData::NetProtocol ); // see bug 56218
    filter( "KDE", "https://www.google.com/search?q=KDE&ie=UTF-8", KUriFilterData::NetProtocol );
    filter( "HTTP", "https://www.google.com/search?q=HTTP&ie=UTF-8", KUriFilterData::NetProtocol );
}

void KUriFilterTest::executables()
{
    // Executable tests - No IKWS in minicli
    filter( "cp", "cp", KUriFilterData::Executable, minicliFilters );
    filter( "ktraderclient", "ktraderclient", KUriFilterData::Executable, minicliFilters );
    filter( "KDE", "KDE", NO_FILTERING, minicliFilters );
    filter( "I/dont/exist", "I/dont/exist", NO_FILTERING, minicliFilters );      //krazy:exclude=spelling
    filter( "/I/dont/exist", 0, KUriFilterData::Error, minicliFilters );         //krazy:exclude=spelling
    filter( "/I/dont/exist#a", 0, KUriFilterData::Error, minicliFilters );       //krazy:exclude=spelling
    filter( "ktraderclient --help", "ktraderclient --help", KUriFilterData::Executable, minicliFilters ); // the args are in argsAndOptions()
    filter( "/usr/bin/gs", "/usr/bin/gs", KUriFilterData::Executable, minicliFilters );
    filter( "/usr/bin/gs -q -option arg1", "/usr/bin/gs -q -option arg1", KUriFilterData::Executable, minicliFilters ); // the args are in argsAndOptions()

    // Typing 'cp' or any other valid unix command in konq's location bar should result in
    // a search using the default search engine
    // 'ls' is a bit of a special case though, due to the toplevel domain called 'ls'
    filter( "cp", "https://www.google.com/search?q=cp&ie=UTF-8", KUriFilterData::NetProtocol,
            QStringList(), 0, false /* don't check for executables, see konq_misc.cc */ );
}

void KUriFilterTest::environmentVariables()
{
    // ENVIRONMENT variable
    setenv( "SOMEVAR", "/somevar", 0 );
    setenv( "ETC", "/etc", 0 );

    filter( "$SOMEVAR/kdelibs/kio", 0, KUriFilterData::Error ); // note: this dir doesn't exist...
    filter( "$ETC/passwd", "/etc/passwd", KUriFilterData::LocalFile );
    QString qtdocPath = qtdir+"/doc/html/functions.html";
    if (QFile::exists(qtdocPath)) {
        QString expectedUrl = KUrl(qtdocPath).url()+"#s";
        filter( "$QTDIR/doc/html/functions.html#s", expectedUrl.toUtf8(), KUriFilterData::LocalFile );
    }
    filter( "http://www.kde.org/$USER", "http://www.kde.org/$USER", KUriFilterData::NetProtocol ); // no expansion

    filter( "$KDEHOME/share", kdehome+"/share", KUriFilterData::LocalDir );
    KStandardDirs::makeDir( kdehome+"/urifilter/a+plus" );
    filter( "$KDEHOME/urifilter/a+plus", kdehome+"/urifilter/a+plus", KUriFilterData::LocalDir );

    // BR 27788
    KStandardDirs::makeDir( kdehome+"/share/Dir With Space" );
    filter( "$KDEHOME/share/Dir With Space", kdehome+"/share/Dir With Space", KUriFilterData::LocalDir );

    // support for name filters (BR 93825)
    filter( "$KDEHOME/*.txt", kdehome+"/*.txt", KUriFilterData::LocalDir );
    filter( "$KDEHOME/[a-b]*.txt", kdehome+"/[a-b]*.txt", KUriFilterData::LocalDir );
    filter( "$KDEHOME/a?c.txt", kdehome+"/a?c.txt", KUriFilterData::LocalDir );
    filter( "$KDEHOME/?c.txt", kdehome+"/?c.txt", KUriFilterData::LocalDir );
    // but let's check that a directory with * in the name still works
    KStandardDirs::makeDir( kdehome+"/share/Dir*With*Stars" );
    filter( "$KDEHOME/share/Dir*With*Stars", kdehome+"/share/Dir*With*Stars", KUriFilterData::LocalDir );
    KStandardDirs::makeDir( kdehome+"/share/Dir?QuestionMark" );
    filter( "$KDEHOME/share/Dir?QuestionMark", kdehome+"/share/Dir?QuestionMark", KUriFilterData::LocalDir );
    KStandardDirs::makeDir( kdehome+"/share/Dir[Bracket" );
    filter( "$KDEHOME/share/Dir[Bracket", kdehome+"/share/Dir[Bracket", KUriFilterData::LocalDir );

    filter( "$HOME/$KDEDIR/kdebase/kcontrol/ebrowsing", 0, KUriFilterData::Error );
    filter( "$1/$2/$3", "https://www.google.com/search?q=%241%2F%242%2F%243&ie=UTF-8", KUriFilterData::NetProtocol );  // can be used as bogus or valid test. Currently triggers default search, i.e. google
    filter( "$$$$", "https://www.google.com/search?q=%24%24%24%24&ie=UTF-8", KUriFilterData::NetProtocol ); // worst case scenarios.

    if (!qtdir.isEmpty()) {
        filter( "$QTDIR", qtdir, KUriFilterData::LocalDir, QStringList( "kshorturifilter" ) ); //use specific filter.
    }
    filter( "$HOME", home, KUriFilterData::LocalDir, QStringList( "kshorturifilter" ) ); //use specific filter.
}

void KUriFilterTest::internetKeywords()
{
    QString sc;
    filter( sc.sprintf("gg%cfoo bar",s_delimiter).toUtf8(), "https://www.google.com/search?q=foo+bar&ie=UTF-8", KUriFilterData::NetProtocol );
    filter( sc.sprintf("bug%c55798", s_delimiter).toUtf8(), "https://bugs.kde.org/show_bug.cgi?id=55798", KUriFilterData::NetProtocol );

    filter( sc.sprintf("gg%cC++", s_delimiter).toUtf8(), "https://www.google.com/search?q=C%2B%2B&ie=UTF-8", KUriFilterData::NetProtocol );
    filter( sc.sprintf("gg%cC#", s_delimiter).toUtf8(), "https://www.google.com/search?q=C%23&ie=UTF-8", KUriFilterData::NetProtocol );
    filter( sc.sprintf("ya%cfoo bar was here", s_delimiter).toUtf8(), 0, -1 ); // this triggers default search, i.e. google
    filter( sc.sprintf("gg%cwww.kde.org", s_delimiter).toUtf8(), "https://www.google.com/search?q=www.kde.org&ie=UTF-8", KUriFilterData::NetProtocol );
    filter( QString::fromUtf8("gg%1é").arg(s_delimiter).toUtf8() /*eaccent in utf8*/, "https://www.google.com/search?q=%C3%A9&ie=UTF-8", KUriFilterData::NetProtocol );
    filter( QString::fromUtf8("gg%1прйвет").arg(s_delimiter).toUtf8() /* greetings in russian utf-8*/, "https://www.google.com/search?q=%D0%BF%D1%80%D0%B9%D0%B2%D0%B5%D1%82&ie=UTF-8", KUriFilterData::NetProtocol );
}

void KUriFilterTest::localdomain()
{
    const QString host = QHostInfo::localHostName();
    if (host.isEmpty()) {
        const QString expected = QLatin1String("http://") + host;
        filter(host.toUtf8(), expected.toUtf8(), KUriFilterData::NetProtocol, QStringList() << "localdomainurifilter", 0, false);
    }
}

#include "kurifiltertest.moc"
