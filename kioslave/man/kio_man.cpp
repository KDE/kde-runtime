/*  This file is part of the KDE libraries
    Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>

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

#include "kio_man.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextDocument>
#include <QMap>
#include <QRegExp>
#include <QTextCodec>

#include <kdebug.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <KProcess>
#include <klocale.h>

#include "kio_man.moc"
#include "man2html.h"
#include <assert.h>
#include <kfilterbase.h>
#include <kfilterdev.h>

using namespace KIO;

MANProtocol *MANProtocol::_self = 0;

#define SGML2ROFF_DIRS "/usr/lib/sgml"

/*
 * Drop trailing ".section[.gz]" from name
 */
static
void stripExtension( QString *name )
{
    int pos = name->length();

    if ( name->indexOf(".gz", -3) != -1 )
        pos -= 3;
    else if ( name->indexOf(".z", -2, Qt::CaseInsensitive) != -1 )
        pos -= 2;
    else if ( name->indexOf(".bz2", -4) != -1 )
        pos -= 4;
    else if ( name->indexOf(".bz", -3) != -1 )
        pos -= 3;
    else if ( name->indexOf(".lzma", -5) != -1 )
        pos -= 5;
    else if ( name->indexOf(".xz", -3) != -1 )
        pos -= 3;

    if ( pos > 0 )
        pos = name->lastIndexOf('.', pos-1);

    if ( pos > 0 )
        name->truncate( pos );
}

static
bool parseUrl(const QString& _url, QString &title, QString &section)
{
    section.clear();

    QString url = _url;
    url = url.trimmed();
    if (url.isEmpty() || url.at(0) == '/') {
        if (url.isEmpty() || KStandardDirs::exists(url)) {
            // man:/usr/share/man/man1/ls.1.gz is a valid file
            title = url;
            return true;
        } else
        {
            // If the directory does not exist, then it is perhaps a normal man page
            kDebug(7107) << url << " does not exist";
        }
    }

    while (!url.isEmpty() && url.at(0) == '/')
        url.remove(0,1);

    title = url;

    int pos = url.indexOf('(');
    if (pos < 0)
        return true; // man:ls -> title=ls

    title = title.left(pos);

    section = url.mid(pos+1);

    pos = section.indexOf(')');
    if (pos >= 0) {
	if (pos < section.length() - 2 && title.isEmpty()) {
		title = section.mid(pos + 2);
	}
        section = section.left(pos);
    }

    // man:ls(2) -> title="ls", section="2"

    return true;
}


MANProtocol::MANProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : QObject(), SlaveBase("man", pool_socket, app_socket)
{
    assert(!_self);
    _self = this;
    const QString common_dir = KGlobal::dirs()->findResourceDir( "html", "en/common/kde-default.css" );
    const QString strPath=QString( "file:%1/en/common" ).arg( common_dir );
    m_cssPath=strPath.toLocal8Bit(); // ### TODO encode for CSS
    section_names << "0" << "0p" << "1" << "1p" << "2" << "3" << "3n" << "3p" << "4" << "5" << "6" << "7"
                  << "8" << "9" << "l" << "n";

    QString cssPath(KStandardDirs::locate( "data", "kio_docfilter/kio_docfilter.css" ));
    KUrl cssUrl(KUrl::fromPath(cssPath));
    m_manCSSFile = cssUrl.url().toUtf8();
}

MANProtocol *MANProtocol::self() { return _self; }

MANProtocol::~MANProtocol()
{
    _self = 0;
}

void MANProtocol::parseWhatIs( QMap<QString, QString> &i, QTextStream &t, const QString &mark )
{
    QRegExp re( mark );
    QString l;
    while ( !t.atEnd() )
    {
	l = t.readLine();
	int pos = re.indexIn( l );
	if (pos != -1)
	{
	    QString names = l.left(pos);
	    QString descr = l.mid(pos + re.matchedLength());
	    while ((pos = names.indexOf(",")) != -1)
	    {
		i[names.left(pos++)] = descr;
		while (names[pos] == ' ')
		    pos++;
		names = names.mid(pos);
	    }
	    i[names] = descr;
	}
    }
}

bool MANProtocol::addWhatIs(QMap<QString, QString> &i, const QString &name, const QString &mark)
{
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QTextStream t(&f);
    parseWhatIs( i, t, mark );
    return true;
}

QMap<QString, QString> MANProtocol::buildIndexMap(const QString &section)
{
    QMap<QString, QString> i;
    QStringList man_dirs = manDirectories();
    // Supplementary places for whatis databases
    man_dirs += m_mandbpath;
    if (!man_dirs.contains("/var/cache/man"))
        man_dirs << "/var/cache/man";
    if (!man_dirs.contains("/var/catman"))
        man_dirs << "/var/catman";

    QStringList names;
    names << "whatis.db" << "whatis";
    QString mark = "\\s+\\(" + section + "[a-z]*\\)\\s+-\\s+";

    for ( QStringList::ConstIterator it_dir = man_dirs.constBegin();
          it_dir != man_dirs.constEnd();
          ++it_dir )
    {
        if ( QFile::exists( *it_dir ) ) {
    	    QStringList::ConstIterator it_name;
            for ( it_name = names.constBegin();
	          it_name != names.constEnd();
	          it_name++ )
            {
	        if (addWhatIs(i, (*it_dir) + '/' + (*it_name), mark))
		    break;
	    }
            if ( it_name == names.constEnd() ) {
                KProcess proc;
                proc << "whatis" << "-M" << (*it_dir) << "-w" << "*";
                proc.setOutputChannelMode( KProcess::OnlyStdoutChannel );
                proc.execute();
                QTextStream t( proc.readAllStandardOutput(), QIODevice::ReadOnly );
                parseWhatIs( i, t, mark );
            }
        }
    }
    return i;
}

//---------------------------------------------------------------------

QStringList MANProtocol::manDirectories()
{
    checkManPaths();
    //
    // Build a list of man directories including translations
    //
    QStringList man_dirs;

    for ( QStringList::ConstIterator it_dir = m_manpath.constBegin();
          it_dir != m_manpath.constEnd();
          it_dir++ )
    {
        // Translated pages in "<mandir>/<lang>" if the directory
        // exists
        QStringList languages = KGlobal::locale()->languageList();

        for (QStringList::ConstIterator it_lang = languages.constBegin();
             it_lang != languages.constEnd();
             it_lang++ )
        {
            if ( !(*it_lang).isEmpty() && (*it_lang) != QString("C") ) {
                QString dir = (*it_dir) + '/' + (*it_lang);

                struct stat sbuf;

                if ( ::stat( QFile::encodeName( dir ), &sbuf ) == 0
                    && S_ISDIR( sbuf.st_mode ) )
                {
                    const QString p = QDir(dir).canonicalPath();
                    if (!man_dirs.contains(p)) man_dirs += p;
                }
            }
        }

        // Untranslated pages in "<mandir>"
        const QString p = QDir(*it_dir).canonicalPath();
        if (!man_dirs.contains(p)) man_dirs += p;
    }
    return man_dirs;
}

QStringList MANProtocol::findPages(const QString &_section,
                                   const QString &title,
                                   bool full_path)
{
    QString section = _section;

    QStringList list;

    // kDebug() << "findPages '" << section << "' '" << title << "'\n";
    if ( (!title.isEmpty()) && (title.at(0) == '/') ) {
       list.append(title);
       return list;
    }

    const QString star( "*" );

    //
    // Find man sections in this directory
    //
    QStringList sect_list;
    if ( section.isEmpty() )
        section = star;

    if ( section != star )
    {
        //
        // Section given as argument
        //
        sect_list += section;
        while ( (!section.isEmpty()) && (section.at(section.length() - 1).isLetter()) ) {
            section.truncate(section.length() - 1);
            sect_list += section;
        }
    } else {
        sect_list += section;
    }

    QStringList man_dirs = manDirectories();

    //
    // Find man pages in the sections listed above
    //
    for ( int i=0;i<sect_list.count(); i++)
    {
            QString it_s=sect_list.at(i);
	    QString it_real = it_s.toLower();
        //
        // Find pages
        //
        //kDebug(7107)<<"Before inner loop";
        for ( QStringList::const_iterator it_dir = man_dirs.constBegin();
              it_dir != man_dirs.constEnd();
              it_dir++ )
        {
            QString man_dir = (*it_dir);
            //
            // Sections = all sub directories "man*" and "sman*"
            //
            DIR *dp = ::opendir( QFile::encodeName( man_dir ) );

            if ( !dp )
                continue;

            struct dirent *ep;

            const QString man = QString("man");
            const QString sman = QString("sman");

            while ( (ep = ::readdir( dp )) != 0L ) {

                const QString file = QFile::decodeName( ep->d_name );
                QString sect;

                if ( file.startsWith( man ) )
                    sect = file.mid(3);
                else if (file.startsWith(sman))
                    sect = file.mid(4);

		if (sect.toLower()==it_real) it_real = sect;

                // Only add sect if not already contained, avoid duplicates
                if (!sect_list.contains(sect) && _section.isEmpty())  {
                    //kDebug() << "another section " << sect;
                    sect_list += sect;
                }
            }

            //kDebug(7107) <<" after while loop";
            ::closedir( dp );
#if 0
            kDebug(7107)<<"================-";
            kDebug(7107)<<"star=="<<star;
            kDebug(7107)<<"it_s=="<<it_s;
            kDebug(7107)<<"================+";
#endif
            if ( it_s != star ) { // in that case we only look around for sections
                //kDebug(7107)<<"Within if ( it_s != star )";
                const QString dir = man_dir + QString("/man") + (it_real) + '/';
                const QString sdir = man_dir + QString("/sman") + (it_real) + '/';

                //kDebug(7107)<<"calling findManPagesInSection";
                findManPagesInSection(dir, title, full_path, list);
                findManPagesInSection(sdir, title, full_path, list);
            }
            kDebug(7107)<<"After if";
        }
    }

    //kDebug(7107) << "finished " << list << " " << sect_list;

    return list;
}

void MANProtocol::findManPagesInSection(const QString &dir, const QString &title, bool full_path, QStringList &list)
{
    kDebug(7107) << "findManPagesInSection " << dir << " " << title;
    bool title_given = !title.isEmpty();

    DIR *dp = ::opendir( QFile::encodeName( dir ) );

    if ( !dp )
        return;

    struct dirent *ep;

    while ( (ep = ::readdir( dp )) != 0L ) {
        if ( ep->d_name[0] != '.' ) {

            QString name = QFile::decodeName( ep->d_name );

            // check title if we're looking for a specific page
            if ( title_given ) {
                if ( !name.startsWith( title ) ) {
                    continue;
                }
                else {
                    // beginning matches, do a more thorough check...
                    QString tmp_name = name;
                    stripExtension( &tmp_name );
                    if ( tmp_name != title )
                        continue;
                }
            }

            if ( full_path )
                name.prepend( dir );

            list += name ;
        }
    }
    ::closedir( dp );
}

void MANProtocol::output(const char *insert)
{
    if (insert)
    {
        m_outputBuffer.write(insert,strlen(insert));
    }
    if (!insert || m_outputBuffer.pos() >= 2048)
    {
        m_outputBuffer.close();
        data(m_outputBuffer.buffer());
        m_outputBuffer.setData(QByteArray());
        m_outputBuffer.open(QIODevice::WriteOnly);
    }
}

#ifndef SIMPLE_MAN2HTML
// called by man2html
extern char *read_man_page(const char *filename)
{
    return MANProtocol::self()->readManPage(filename);
}

// called by man2html
extern void output_real(const char *insert)
{
    MANProtocol::self()->output(insert);
}
#endif

void MANProtocol::get(const KUrl& url )
{
    kDebug(7107) << "GET " << url.url();

    QString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        showMainIndex();
        return;
    }

    // tell the mimetype
    mimeType("text/html");

    // see if an index was requested
    if (url.query().isEmpty() && (title.isEmpty() || title == "/" || title == "."))
    {
        if (section == "index" || section.isEmpty())
            showMainIndex();
        else
            showIndex(section);
        return;
    }

    const QStringList foundPages=findPages(section, title);
    bool pageFound=true;

    if (foundPages.isEmpty())
    {
       outputError(i18n("No man page matching to %1 found.<br /><br />"
           "Check that you have not mistyped the name of the page that you want.<br />"
           "Check that you have typed the name using the correct upper and lower case characters.<br />"
           "If everything looks correct, then you may need to improve the search path "
           "for man pages; either using the environment variable MANPATH or using a matching file "
           "in the /etc directory.", Qt::escape(title)));
       pageFound=false;
    }
    else if (foundPages.count()>1)
    {
       pageFound=false;
       //check for the case that there is foo.1 and foo.1.gz found:
       // ### TODO make it more generic (other extensions)
       if ((foundPages.count()==2) &&
           ((QString(foundPages[0]+".gz") == foundPages[1]) ||
            (foundPages[0] == QString(foundPages[1]+".gz"))))
          pageFound=true;
       else
          outputMatchingPages(foundPages);
    }
    //yes, we found exactly one man page

    if (pageFound)
    {
       setResourcePath(m_cssPath);
       setCssFile(m_manCSSFile);
       m_outputBuffer.open(QIODevice::WriteOnly);
       const QByteArray filename=QFile::encodeName(foundPages[0]);
       char *buf = readManPage(filename);

       if (!buf)
       {
          outputError(i18n("Open of %1 failed.", title));
          finished();
          return;
       }
       // will call output_real
       scan_man_page(buf);
       delete [] buf;

       output(0); // flush

       m_outputBuffer.close();
       data(m_outputBuffer.buffer());
       m_outputBuffer.setData(QByteArray());
       // tell we are done
       data(QByteArray());
    }
    finished();
}

//---------------------------------------------------------------------

char *MANProtocol::readManPage(const char *_filename)
{
    QByteArray filename = _filename;
    QByteArray array, dirName;

    /* Determine type of man page file by checking its path. Determination by
     * MIME type with KMimeType doesn't work reliablely. E.g., Solaris 7:
     * /usr/man/sman7fs/pcfs.7fs -> text/x-csrc : WRONG
     * If the path name constains the string sman, assume that it's SGML and
     * convert it to roff format (used on Solaris). */
    //QString file_mimetype = KMimeType::findByPath(QString(filename), 0, false)->name();
    if (QString(filename).contains("sman", Qt::CaseInsensitive)) //file_mimetype == "text/html" || )
    {
        KProcess proc;
        // Determine path to sgml2roff, if not already done.
        getProgramPath();
        proc << mySgml2RoffPath << filename;
        proc.setOutputChannelMode( KProcess::OnlyStdoutChannel );
        proc.execute();
        array = proc.readAllStandardOutput();
    }
    else
    {
      if (QDir::isRelativePath(filename))
      {
          kDebug(7107) << "relative " << filename;
          filename = QDir::cleanPath(lastdir + '/' + filename).toUtf8();
          kDebug(7107) << "resolved to " << filename;
      }

      lastdir = filename.left(filename.lastIndexOf('/'));

      // get the last directory name (which might be a language name, to be able to guess the encoding)
      QDir dir(lastdir);
      dir.cdUp();
      dirName = QFile::encodeName(dir.dirName());

      if ( !QFile::exists(QFile::decodeName(filename)) )  // if given file does not exist, find with suffix
      {
          kDebug(7107) << "not existing " << filename;
          QDir mandir(lastdir);
          mandir.setNameFilters(QStringList() << (filename.mid(filename.lastIndexOf('/') + 1) + ".*"));
          filename = lastdir + '/' + QFile::encodeName(mandir.entryList().first());
          kDebug(7107) << "resolved to " << filename;
      }

      QIODevice *fd = KFilterDev::deviceForFile(filename);

      if ( !fd || !fd->open(QIODevice::ReadOnly))
      {
         delete fd;
         return 0;
      }
      array = fd->readAll();
      kDebug(7107) << "read " << array.size();
      fd->close();
      delete fd;
    }

    if (array.isEmpty())
      return 0;

    return manPageToUtf8(array, dirName);
}

//---------------------------------------------------------------------

void MANProtocol::outputError(const QString& errmsg)
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);
    os.setCodec( "UTF-8" );

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << i18n("Man output") << "</title>\n" << endl;
    if ( !m_manCSSFile.isEmpty() )
        os << "<link href=\"" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" << endl;
    os << "<body>" << i18n("<h1>KDE Man Viewer Error</h1>") << errmsg << "</body>" << endl;
    os << "</html>" << endl;

    data(array);
}

void MANProtocol::outputMatchingPages(const QStringList &matchingPages)
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);
    os.setCodec( "UTF-8" );

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"<<endl;
    os << "<title>" << i18n("Man output") <<"</title>" << endl;
    if ( !m_manCSSFile.isEmpty() )
        os << "<link href=\"" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" <<endl;
    os << "<body><h1>" << i18n("There is more than one matching man page.");
    os << "</h1>\n<ul>\n";

    int acckey=1;
    for (QStringList::ConstIterator it = matchingPages.begin(); it != matchingPages.end(); ++it)
    {
      os<<"<li><a href='man:"<<(*it)<<"' accesskey='"<< acckey <<"'>"<< *it <<"</a><br>\n<br>\n";
      acckey++;
    }
    os << "</ul>\n";
    os << "<hr>\n";
    os << "<p>" << i18n("Note: if you read a man page in your language,"
       " be aware it can contain some mistakes or be obsolete."
       " In case of doubt, you should have a look at the English version.") << "</p>";

    os << "</body>\n</html>"<<endl;

    data(array);
    finished();
}

void MANProtocol::stat( const KUrl& url)
{
    kDebug(7107) << "ENTERING STAT " << url.url();

    QString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        error(KIO::ERR_MALFORMED_URL, url.url());
        return;
    }

    kDebug(7107) << "URL " << url.url() << " parsed to title='" << title << "' section=" << section;

    UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_NAME, title);
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);

#if 0 // not useful, is it?
    QString newUrl = "man:"+title;
    if (!section.isEmpty())
        newUrl += QString("(%1)").arg(section);
    entry.insert(KIO::UDSEntry::UDS_URL, newUrl);
#endif

    entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("text/html"));

    statEntry(entry);

    finished();
}


extern "C"
{

    int KDE_EXPORT kdemain( int argc, char **argv ) {

        KComponentData componentData("kio_man");

        kDebug(7107) <<  "STARTING";

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_man protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        MANProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        kDebug(7107) << "Done";

        return 0;
    }

}

void MANProtocol::mimetype(const KUrl & /*url*/)
{
    mimeType("text/html");
    finished();
}

static QString sectionName(const QString& section)
{
    if      (section ==  "0") return i18n("Header files");
    else if (section == "0p") return i18n("Header files (POSIX)");
    else if (section ==  "1") return i18n("User Commands");
    else if (section == "1p") return i18n("User Commands (POSIX)");
    else if (section ==  "2") return i18n("System Calls");
    else if (section ==  "3") return i18n("Subroutines");
    else if (section == "3p") return i18n("Perl Modules");
    else if (section == "3n") return i18n("Network Functions");
    else if (section ==  "4") return i18n("Devices");
    else if (section ==  "5") return i18n("File Formats");
    else if (section ==  "6") return i18n("Games");
    else if (section ==  "7") return i18n("Miscellaneous");
    else if (section ==  "8") return i18n("System Administration");
    else if (section ==  "9") return i18n("Kernel");
    else if (section ==  "l") return i18n("Local Documentation");
    else if (section ==  "n") return i18n("New");

    return QString();
}

QStringList MANProtocol::buildSectionList(const QStringList& dirs) const
{
    QStringList l;

    for (QStringList::ConstIterator it = section_names.begin();
	    it != section_names.end(); ++it)
    {
	    for (QStringList::ConstIterator dir = dirs.begin();
		    dir != dirs.end(); ++dir)
	    {
		QDir d((*dir)+"/man"+(*it));
		if (d.exists())
		{
		    l << *it;
		    break;
		}
	    }
    }
    return l;
}

void MANProtocol::showMainIndex()
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);
    os.setCodec( "UTF-8" );

    // print header
    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << i18n("UNIX Manual Index") << "</title>" << endl;
    if (!m_manCSSFile.isEmpty())
        os << "<link href=\"" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" << endl;
    os << "<body><h1>" << i18n("UNIX Manual Index") << "</h1>" << endl;

    // ### TODO: why still the environment variable
    const QString sectList = getenv("MANSECT");
    QStringList sections;
    if (sectList.isEmpty())
    	sections = buildSectionList(manDirectories());
    else
	sections = sectList.split( ':');

    os << "<table>" << endl;

    QSet<QChar> accessKeys;
    char alternateAccessKey = 'a';
    QStringList::ConstIterator it;
    for (it = sections.constBegin(); it != sections.constEnd(); ++it)
    {
        // create a unique access key
        QChar accessKey = (*it).at((*it).length() - 1);  // rightmost char

        while ( accessKeys.contains(accessKey) )
            accessKey = alternateAccessKey++;

        accessKeys.insert(accessKey);

        os << "<tr><td><a href=\"man:(" << *it << ")\" accesskey=\"" << accessKey
	<< "\">" << i18n("Section %1", *it)
	<< "</a></td><td>&nbsp;</td><td> " << sectionName(*it) << "</td></tr>" << endl;
    }

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    data(array);
    finished();
}

void MANProtocol::constructPath(QStringList& constr_path, QStringList constr_catmanpath)
{
    QMap<QString, QString> manpath_map;
    QMap<QString, QString> mandb_map;

    // Add paths from /etc/man.conf
    //
    // Explicit manpaths may be given by lines starting with "MANPATH" or
    // "MANDATORY_MANPATH" (depending on system ?).
    // Mappings from $PATH to manpath are given by lines starting with
    // "MANPATH_MAP"

    QRegExp manpath_regex( "^MANPATH\\s" );
    QRegExp mandatory_regex( "^MANDATORY_MANPATH\\s" );
    QRegExp manpath_map_regex( "^MANPATH_MAP\\s" );
    QRegExp mandb_map_regex( "^MANDB_MAP\\s" );
    //QRegExp section_regex( "^SECTION\\s" );
    QRegExp space_regex( "\\s+" ); // for parsing manpath map

    QFile mc("/etc/man.conf");             // Caldera
    if (!mc.exists())
        mc.setFileName("/etc/manpath.config"); // SuSE, Debian
    if (!mc.exists())
        mc.setFileName("/etc/man.config");  // Mandrake

    if (mc.open(QIODevice::ReadOnly))
    {
        QTextStream is(&mc);
        is.setCodec( QTextCodec::codecForLocale () );

        while (!is.atEnd())
        {
            const QString line = is.readLine();
            if ( manpath_regex.indexIn(line) == 0 )
            {
                const QString path = line.mid(8).trimmed();
                constr_path += path;
            }
            else if ( mandatory_regex.indexIn(line) == 0 )
            {
                const QString path = line.mid(18).trimmed();
                constr_path += path;
            }
            else if ( manpath_map_regex.indexIn(line) == 0 )
            {
                        // The entry is "MANPATH_MAP  <path>  <manpath>"
                const QStringList mapping =
                        line.split( space_regex);

                if ( mapping.count() == 3 )
                {
                    const QString dir = QDir::cleanPath( mapping[1] );
                    const QString mandir = QDir::cleanPath( mapping[2] );

                    manpath_map[ dir ] = mandir;
                }
            }
            else if ( mandb_map_regex.indexIn(line) == 0 )
            {
                        // The entry is "MANDB_MAP  <manpath>  <catmanpath>"
                const QStringList mapping =
                        line.split( space_regex);

                if ( mapping.count() == 3 )
                {
                    const QString mandir = QDir::cleanPath( mapping[1] );
                    const QString catmandir = QDir::cleanPath( mapping[2] );

                    mandb_map[ mandir ] = catmandir;
                }
            }
    /* sections are not used
            else if ( section_regex.find(line, 0) == 0 )
            {
            if ( !conf_section.isEmpty() )
            conf_section += ':';
            conf_section += line.mid(8).trimmed();
        }
    */
        }
        mc.close();
    }

    // Default paths
    static const char * const manpaths[] = {
        "/usr/X11/man",
        "/usr/X11R6/man",
        "/usr/man",
        "/usr/local/man",
        "/usr/exp/man",
        "/usr/openwin/man",
        "/usr/dt/man",
        "/opt/freetool/man",
        "/opt/local/man",
        "/usr/tex/man",
        "/usr/www/man",
        "/usr/lang/man",
        "/usr/gnu/man",
        "/usr/share/man",
        "/usr/motif/man",
        "/usr/titools/man",
        "/usr/sunpc/man",
        "/usr/ncd/man",
        "/usr/newsprint/man",
        NULL };


    int i = 0;
    while (manpaths[i]) {
        if ( constr_path.indexOf( QString( manpaths[i] ) ) == -1 )
            constr_path += QString( manpaths[i] );
        i++;
    }

    // Directories in $PATH
    // - if a manpath mapping exists, use that mapping
    // - if a directory "<path>/man" or "<path>/../man" exists, add it
    //   to the man path (the actual existence check is done further down)

    if ( ::getenv("PATH") ) {
        const QStringList path =
              QString::fromLocal8Bit( ::getenv("PATH") ).split( ':', QString::SkipEmptyParts );

        for ( QStringList::const_iterator it = path.constBegin();
              it != path.constEnd();
              ++it )
        {
            const QString dir = QDir::cleanPath( *it );
            QString mandir = manpath_map[ dir ];

            if ( !mandir.isEmpty() ) {
                                // a path mapping exists
                if ( constr_path.indexOf( mandir ) == -1 )
                    constr_path += mandir;
            }
            else {
                                // no manpath mapping, use "<path>/man" and "<path>/../man"

                mandir = dir + QString( "/man" );
                if ( constr_path.indexOf( mandir ) == -1 )
                    constr_path += mandir;

                int pos = dir.lastIndexOf( '/' );
                if ( pos > 0 ) {
                    mandir = dir.left( pos ) + QString("/man");
                    if ( constr_path.indexOf( mandir ) == -1 )
                        constr_path += mandir;
                }
            }
            QString catmandir = mandb_map[ mandir ];
            if ( !mandir.isEmpty() )
            {
                if ( constr_catmanpath.indexOf( catmandir ) == -1 )
                    constr_catmanpath += catmandir;
            }
            else
            {
            // What is the default mapping?
                catmandir = mandir;
                catmandir.replace("/usr/share/","/var/cache/");
                if ( constr_catmanpath.indexOf( catmandir ) == -1 )
                    constr_catmanpath += catmandir;
            }
        }
    }
}

void MANProtocol::checkManPaths()
{
    static bool inited = false;

    if (inited)
        return;

    inited = true;

    const QString manpath_env = QString::fromLocal8Bit( ::getenv("MANPATH") );
    //QString mansect_env = QString::fromLocal8Bit( ::getenv("MANSECT") );

    // Decide if $MANPATH is enough on its own or if it should be merged
    // with the constructed path.
    // A $MANPATH starting or ending with ":", or containing "::",
    // should be merged with the constructed path.

    bool construct_path = false;

    if ( manpath_env.isEmpty()
        || manpath_env[0] == ':'
        || manpath_env[manpath_env.length()-1] == ':'
        || manpath_env.contains( "::" ) )
    {
        construct_path = true; // need to read config file
    }

    // Constucted man path -- consists of paths from
    //   /etc/man.conf
    //   default dirs
    //   $PATH
    QStringList constr_path;
    QStringList constr_catmanpath; // catmanpath

    QString conf_section;

    if ( construct_path )
    {
        constructPath(constr_path, constr_catmanpath);
    }

    m_mandbpath=constr_catmanpath;

    // Merge $MANPATH with the constructed path to form the
    // actual manpath.
    //
    // The merging syntax with ":" and "::" in $MANPATH will be
    // satisfied if any empty string in path_list_env (there
    // should be 1 or 0) is replaced by the constructed path.

    const QStringList path_list_env = manpath_env.split( ':', QString::KeepEmptyParts);

    for ( QStringList::const_iterator it = path_list_env.constBegin();
          it != path_list_env.constEnd();
          ++it )
    {
        struct stat sbuf;

        QString dir = (*it);

        if ( !dir.isEmpty() ) {
            // Add dir to the man path if it exists
            if ( m_manpath.indexOf( dir ) == -1 ) {
                if ( ::stat( QFile::encodeName( dir ), &sbuf ) == 0
                    && S_ISDIR( sbuf.st_mode ) )
                {
                    m_manpath += dir;
                }
            }
        }
        else {
            // Insert constructed path ($MANPATH was empty, or
            // there was a ":" at an end or "::")

            for ( QStringList::const_iterator it2 = constr_path.constBegin();
                  it2 != constr_path.constEnd();
                  it2++ )
            {
                dir = (*it2);

                if ( !dir.isEmpty() ) {
                    if ( m_manpath.indexOf( dir ) == -1 ) {
                        if ( ::stat( QFile::encodeName( dir ), &sbuf ) == 0
                            && S_ISDIR( sbuf.st_mode ) )
                        {
                            m_manpath += dir;
                        }
                    }
                }
            }
        }
    }

/* sections are not used
    // Sections
    QStringList m_mansect = mansect_env.split( ':', QString::KeepEmptyParts);

    const char* const default_sect[] =
        { "1", "2", "3", "4", "5", "6", "7", "8", "9", "n", 0L };

    for ( int i = 0; default_sect[i] != 0L; i++ )
        if ( m_mansect.indexOf( QString( default_sect[i] ) ) == -1 )
            m_mansect += QString( default_sect[i] );
*/

}


// Setup my own structure, with char pointers.
// from now on only pointers are copied, no strings
//
// containing the whole path string,
// the beginning of the man page name
// and the length of the name
struct man_index_t {
    char *manpath;  // the full path including man file
    const char *manpage_begin;  // pointer to the begin of the man file name in the path
    int manpage_len; // len of the man file name
};
typedef man_index_t *man_index_ptr;

int compare_man_index(const void *s1, const void *s2)
{
    struct man_index_t *m1 = *(struct man_index_t **)s1;
    struct man_index_t *m2 = *(struct man_index_t **)s2;
    int i;
    // Compare the names of the pages
    // with the shorter length.
    // Man page names are not '\0' terminated, so
    // this is a bit tricky
    if ( m1->manpage_len > m2->manpage_len)
    {
	i = qstrnicmp( m1->manpage_begin,
		      m2->manpage_begin,
		      m2->manpage_len);
	if (!i)
	    return 1;
	return i;
    }

    if ( m1->manpage_len < m2->manpage_len)
    {
	i = qstrnicmp( m1->manpage_begin,
		      m2->manpage_begin,
		      m1->manpage_len);
	if (!i)
	    return -1;
	return i;
    }

    return qstrnicmp( m1->manpage_begin,
		     m2->manpage_begin,
		     m1->manpage_len);
}

void MANProtocol::showIndex(const QString& section)
{
    QByteArray array_h;
    QTextStream os_h(&array_h, QIODevice::WriteOnly);
    os_h.setCodec( "UTF-8" );

    // print header
    os_h << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os_h << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os_h << "<title>" << i18n("UNIX Manual Index") << "</title>" << endl;
    if ( !m_manCSSFile.isEmpty() )
        os_h << "<link href=\"" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os_h << "</head>" << endl << "<body>" << endl;

    QByteArray array_d;
    QTextStream os(&array_d, QIODevice::WriteOnly);
    os.setCodec( "UTF-8" );
    os << "<div class=\"secidxmain\">" << endl;
    os << "<h1>" << i18n( "Index for Section %1: %2", section, sectionName(section)) << "</h1>" << endl;

    // compose list of search paths -------------------------------------------------------------

    checkManPaths();
    infoMessage(i18n("Generating Index"));

    // search for the man pages
    QStringList pages = findPages( section, QString() );

    if ( pages.count() == 0 )  // not a single page found
    {
      // print footer
      os << "</div></body></html>" << endl;

      infoMessage(QString());
      data(array_h + array_d);
      finished();
      return;
    }

    QMap<QString, QString> indexmap = buildIndexMap(section);

    // print out the list
    os << "<table>" << endl;

    int listlen = pages.count();
    man_index_ptr *indexlist = new man_index_ptr[listlen];
    listlen = 0;

    QStringList::const_iterator page;
    for (page = pages.constBegin(); page != pages.constEnd(); ++page)
    {
	// I look for the beginning of the man page name
	// i.e. "bla/pagename.3.gz" by looking for the last "/"
	// Then look for the end of the name by searching backwards
	// for the last ".", not counting zip extensions.
	// If the len of the name is >0,
	// store it in the list structure, to be sorted later

        char *manpage_end;
        struct man_index_t *manindex = new man_index_t;
	manindex->manpath = strdup((*page).toUtf8());

	manindex->manpage_begin = strrchr(manindex->manpath, '/');
	if (manindex->manpage_begin)
	{
	    manindex->manpage_begin++;
	    assert(manindex->manpage_begin >= manindex->manpath);
	}
	else
	{
	    manindex->manpage_begin = manindex->manpath;
	    assert(manindex->manpage_begin >= manindex->manpath);
	}

	// Skip extension ".section[.gz]"

	char *begin = (char*)(manindex->manpage_begin);
	int len = strlen( begin );
	char *end = begin+(len-1);

	if ( len >= 3 && strcmp( end-2, ".gz" ) == 0 )
	    end -= 3;
	else if ( len >= 2 && strcmp( end-1, ".Z" ) == 0 )
	    end -= 2;
	else if ( len >= 2 && strcmp( end-1, ".z" ) == 0 )
	    end -= 2;
	else if ( len >= 4 && strcmp( end-3, ".bz2" ) == 0 )
	    end -= 4;
	else if ( len >= 5 && strcmp( end-4, ".lzma" ) == 0 )
	    end -= 5;
	else if ( len >= 3 && strcmp( end-2, ".xz" ) == 0 )
	    end -= 3;

	while ( end >= begin && *end != '.' )
	    end--;

	if ( end < begin )
	    manpage_end = 0;
	else
	    manpage_end = end;

	if (NULL == manpage_end)
	{
	    // no '.' ending ???
	    // set the pointer past the end of the filename
	    manindex->manpage_len = (*page).length();
	    manindex->manpage_len -= (manindex->manpage_begin - manindex->manpath);
	    assert(manindex->manpage_len >= 0);
	}
	else
	{
	    manindex->manpage_len = (manpage_end - manindex->manpage_begin);
	    assert(manindex->manpage_len >= 0);
	}

	if (0 < manindex->manpage_len)
	{
	    indexlist[listlen] = manindex;
	    listlen++;
	}
	else delete manindex;
    }

    //
    // Now do the sorting on the page names
    // and the printout afterwards
    // While printing avoid duplicate man page names
    //

    struct man_index_t dummy_index = {0l,0l,0};
    struct man_index_t *last_index = &dummy_index;

    // sort and print
    qsort(indexlist, listlen, sizeof(struct man_index_t *), compare_man_index);

    QChar firstchar, tmp;
    QString indexLine="<div class=\"secidxshort\">\n";
    if (indexlist[0]->manpage_len>0)
    {
	firstchar=QChar((indexlist[0]->manpage_begin)[0]).toLower();

	const QString appendixstr = QString(
	    " [<a href=\"#%1\" accesskey=\"%2\">%3</a>]\n"
	).arg(firstchar).arg(firstchar).arg(firstchar);
	indexLine.append(appendixstr);
    }
    os << "<tr><td class=\"secidxnextletter\"" << " colspan=\"3\">\n  <a name=\""
       << firstchar << "\">" << firstchar <<"</a>\n</td></tr>" << endl;

    for (int i=0; i<listlen; i++)
    {
	struct man_index_t *manindex = indexlist[i];

	// qstrncmp():
	// "last_man" has already a \0 string ending, but
	// "manindex->manpage_begin" has not,
	// so do compare at most "manindex->manpage_len" of the strings.
	if (last_index->manpage_len == manindex->manpage_len &&
	    !qstrncmp(last_index->manpage_begin,
		      manindex->manpage_begin,
		      manindex->manpage_len)
	    )
	{
	    continue;
	}

	tmp=QChar((manindex->manpage_begin)[0]).toLower();
	if (firstchar != tmp)
	{
	    firstchar = tmp;
	    os << "<tr><td class=\"secidxnextletter\"" << " colspan=\"3\">\n  <a name=\""
	       << firstchar << "\">" << firstchar << "</a>\n</td></tr>" << endl;

    	    const QString appendixstr = QString(
    		" [<a href=\"#%1\" accesskey=\"%2\">%3</a>]\n"
	    ).arg(firstchar).arg(firstchar).arg(firstchar);
	    indexLine.append(appendixstr);
	}
	os << "<tr><td><a href=\"man:"
	   << manindex->manpath << "\">\n";

	((char *)manindex->manpage_begin)[manindex->manpage_len] = '\0';
	os << manindex->manpage_begin
	   << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(manindex->manpage_begin) ? indexmap[manindex->manpage_begin] : "" )
	   << "</td></tr>"  << endl;
	last_index = manindex;
    }
    indexLine.append("</div>");

    for (int i=0; i<listlen; i++) {
	::free(indexlist[i]->manpath);   // allocated by strdup
	delete indexlist[i];
    }

    delete [] indexlist;

    os << "</table></div>" << endl;

    os << indexLine << endl;

    // print footer
    os << "</body></html>" << endl;

    // set the links "toolbar" also at the top
    os_h << indexLine << endl;

    infoMessage(QString());
    data(array_h + array_d);
    finished();
}

void MANProtocol::listDir(const KUrl &url)
{
    kDebug( 7107 ) << url;

    QString title;
    QString section;

    if ( !parseUrl(url.path(), title, section) ) {
        error( KIO::ERR_MALFORMED_URL, url.url() );
        return;
    }

    // stat() and listDir() declared that everything is an html file.
    // However we can list man: and man:(1) as a directory (e.g. in dolphin).
    // But we cannot list man:ls as a directory, this makes no sense (#154173)

    if (!title.isEmpty() && title != "/") {
	error(KIO::ERR_IS_FILE, url.url());
        return;
    }

    UDSEntryList uds_entry_list;

    if (section.isEmpty()) {
        for (QStringList::ConstIterator it = section_names.constBegin(); it != section_names.constEnd(); ++it) {
            UDSEntry     uds_entry;

            QString name = "man:/(" + *it + ')';
            uds_entry.insert( KIO::UDSEntry::UDS_NAME, sectionName( *it ) );
            uds_entry.insert( KIO::UDSEntry::UDS_URL, name );
            uds_entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );

            uds_entry_list.append( uds_entry );
        }
    }

    QStringList list = findPages( section, QString(), false );

    QStringList::Iterator it = list.begin();
    QStringList::Iterator end = list.end();

    for ( ; it != end; ++it ) {
        stripExtension( &(*it) );

        UDSEntry     uds_entry;
        uds_entry.insert( KIO::UDSEntry::UDS_NAME, *it );
        uds_entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        uds_entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("text/html"));
        uds_entry_list.append( uds_entry );
    }

    listEntries( uds_entry_list );
    finished();
}

void MANProtocol::getProgramPath()
{
  if (!mySgml2RoffPath.isEmpty())
    return;

  mySgml2RoffPath = KGlobal::dirs()->findExe("sgml2roff");
  if (!mySgml2RoffPath.isEmpty())
    return;

  /* sgml2roff isn't found in PATH. Check some possible locations where it may be found. */
  mySgml2RoffPath = KGlobal::dirs()->findExe("sgml2roff", QString(SGML2ROFF_DIRS));
  if (!mySgml2RoffPath.isEmpty())
    return;

  /* Cannot find sgml2roff program: */
  outputError(i18n("Could not find the sgml2roff program on your system. Please install it, if necessary, and extend the search path by adjusting the environment variable PATH before starting KDE."));
  finished();
  exit();
}
