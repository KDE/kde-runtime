/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
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
 **/

#include "kio_print.h"
#include <kdeprint/kmprinter.h>
#include <kdeprint/kmmanager.h>
#include <kdeprint/kmjobmanager.h>
#include <kdeprint/kmjob.h>
#include <kdeprint/driver.h>

#include <QFile>
#include <QTextStream>
#include <klocale.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kio/job.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <ktemporaryfile.h>
#include <Qt/qdom.h>
#include <Q3PtrListIterator>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define	PRINT_DEBUG	kDebug(7019) << "kio_print: "

extern "C"
{
	int KDE_EXPORT kdemain(int argc, char **argv);
}

static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime)
{
	entry.clear();
	entry.insert( KIO::UDSEntry::UDS_NAME, name);
	entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
	entry.insert(KIO::UDSEntry::UDS_ACCESS, 0500);
	entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, mime);
	entry.insert(KIO::UDSEntry::UDS_URL, url);
	PRINT_DEBUG << "creating dir entry url=" << url << " mimetype=" << mime << endl;
	entry.insert(KIO::UDSEntry::UDS_SIZE,0);
	//addAtom(entry, KIO::UDSEntry::UDS_GUESSED_MIME_TYPE, 0, "application/octet-stream");
}

static void createFileEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime)
{
	entry.clear();
	entry.insert(KIO::UDSEntry::UDS_NAME, name);
	entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
	entry.insert(KIO::UDSEntry::UDS_URL, url);
	entry.insert(KIO::UDSEntry::UDS_ACCESS, 0400);
	entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, mime);
	entry.insert(KIO::UDSEntry::UDS_SIZE, 0);
	entry.insert(KIO::UDSEntry::UDS_GUESSED_MIME_TYPE, QString::fromLatin1("application/octet-stream"));
}

QString buildMenu(const QStringList& items, const QStringList& links, int active)
{
	if (items.count() == 0 || items.count() != links.count())
		return QString("<td height=20 class=\"menu\">&nbsp;</td>");

	QString	s;
	int	index = 0;
	for (QStringList::ConstIterator it1=items.begin(), it2=links.begin(); it1!=items.end() && it2!=links.end(); ++it1, ++it2, index++)
	{
		if (index == active)
			s.append("<td height=20 class=\"menuactive\">&nbsp; ").append(*it1).append("&nbsp;</td>");
		else
			s.append("<td height=20 class=\"menu\">&nbsp; <a class=\"menu\" href=\"").append(*it2).append("\">").append(*it1).append("</a>&nbsp;</td>");
		if (index < items.count()-1)
			s.append("<td height=20 class=\"menu\">|</td>");
	}
	return s;
}

QString buildOptionRow(DrBase *opt, bool f)
{
	QString	s("<tr class=\"%1\"><td width=\"41%\">%1</td><td width=\"59%\">%1</td></tr>\n");
	s = s.arg(f ? "contentwhite" : "contentyellow").arg(opt->get("text")).arg(opt->prettyText());
	return s;
}

QString buildGroupTable(DrGroup *grp, bool showHeader = true)
{
	QString	s("<tr class=\"top\"><td colspan=\"2\">%1</td></tr>\n");
	if (showHeader)
		s = s.arg(grp->get("text"));
	else
		s.clear();

	bool	f(false);
	for (int i = 0; i < grp->options().size(); ++i)
		s.append(buildOptionRow(grp->options().at(i), f));

	for (int i = 0; i < grp->groups().size(); ++i)
		s.append(buildGroupTable(grp->groups().at(i)));

	return s;
}

int kdemain(int argc, char **argv)
{
        KComponentData componentData("kio_print");

	PRINT_DEBUG << "starting ioslave" << endl;
	if (argc != 4)
	{
		fprintf(stderr, "Usage: kio_print protocol domain-socket1 domain-socket2\n");
		exit(-1);
	}

	/* create fake KApplicatiom object, needed for job stuffs */
	KAboutData about( "kio_print", 0, ki18n("kio_print"), "fake_version",
			ki18n("KDEPrint IO slave"), KAboutData::License_GPL, ki18n("(c) 2003, Michael Goffioul") );
	KCmdLineArgs::init( &about );
	KApplication app;

	KIO_Print	 slave(argv[2], argv[3]);
	slave.dispatchLoop();

	PRINT_DEBUG << "done" << endl;
	return 0;
}

KIO_Print::KIO_Print(const QByteArray& pool, const QByteArray& app)
: KIO::SlaveBase("print", pool, app)
{
}

void KIO_Print::listDir(const KUrl& url)
{
	if ( url.protocol() == "printdb" )
	{
		listDirDB( url );
		return;
	}

	QStringList	path = url.path().split( '/', QString::SkipEmptyParts);

	PRINT_DEBUG << "listing " << url.path() << endl;
	QString	group = path[0].toLower();
	if (path.count() == 0)
		listRoot();
	else if (path.count() == 1 && group != "manager" && group != "jobs")
	{
		PRINT_DEBUG << "listing group " << path[0] << endl;

		int	mask;
		QString	mimeType;
		KIO::UDSEntry	entry;

		if (group == "printers")
		{
			mask = KMPrinter::Printer;
			mimeType = "print/printer";
		}
		else if (group == "classes")
		{
			mask = KMPrinter::Class | KMPrinter::Implicit;
			mimeType = "print/class";
		}
		else if (group == "specials")
		{
			mask = KMPrinter::Special;
			mimeType = "print/printer";
		}
		else
		{
			error(KIO::ERR_DOES_NOT_EXIST, url.url());
			return;
		}

		QListIterator<KMPrinter*>	it((KMManager::self()->printerList()));
		while (it.hasNext())
		{
			KMPrinter *itr = it.next();
			if (!(itr->type() & mask) || !itr->instanceName().isEmpty())
			{
				PRINT_DEBUG << "rejecting " << itr->name() << endl;
				continue;
			}

			//createFileEntry(entry, it.current()->name(), ("print:/"+path[0]+"/"+it.current()->name()), mimeType, "text/html", S_IFDIR);
			createDirEntry(entry, itr->name(), QString("print:/%1/%2" ).arg( group, QString( QUrl::toPercentEncoding(itr->name(), "/")) ), mimeType);
			PRINT_DEBUG << "accepting " << itr->name() << endl;
			listEntry(entry, false);
		}

		listEntry(KIO::UDSEntry(), true);
		finished();
	}
	else
	{
		//error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Unsupported path %1").arg(url.path()));
		// better do nothing
		listEntry(KIO::UDSEntry(), true);
		totalSize(0);
		finished();
	}
}

void KIO_Print::listRoot()
{
	PRINT_DEBUG << "listing root entry" << endl;

	KIO::UDSEntry	entry;

	// Classes entry
	createDirEntry(entry, i18n("Classes"), "print:/classes", "print/folder");
	listEntry(entry, false);

	// Printers entry
	createDirEntry(entry, i18n("Printers"), "print:/printers", "print/folder");
	listEntry(entry, false);

	// Specials entry
	createDirEntry(entry, i18n("Specials"), "print:/specials", "print/folder");
	listEntry(entry, false);

	// Management entry
	//createFileEntry(entry, i18n("Manager"), "print:/manager", "print/manager", QString(), S_IFDIR);
	createDirEntry(entry, i18n("Manager"), "print:/manager", "print/manager");
	listEntry(entry, false);

	// Jobs entry
	createDirEntry(entry, i18n("Jobs"), "print:/jobs", "print/jobs");
	listEntry(entry, false);

	// finish
	totalSize(4);
	listEntry(entry, true);
	finished();
}

void KIO_Print::listDirDB( const KUrl& url )
{
	PRINT_DEBUG << "listDirDB: " << url << endl;

	QStringList pathComps = url.path().split( '/', QString::SkipEmptyParts);
	KUrl remUrl;

	remUrl.setProtocol( "http" );
	remUrl.setHost( url.host() );
	remUrl.setPort( url.port() );
	remUrl.setPath( "/list-data.cgi" );
	switch ( pathComps.size() )
	{
		case 0: /* list manufacturers */
			remUrl.addQueryItem( "type", "makes" );
			break;
		case 1: /* list printers for the given manufacturer */
			remUrl.addQueryItem( "type", "printers" );
			remUrl.addQueryItem( "make", pathComps[ 0 ] );
			break;
		case 2: /* list drivers for given printer */
			remUrl.addQueryItem( "type", "drivers" );
			remUrl.addQueryItem( "printer", pathComps[ 1 ] );
			break;
		default:
			error( KIO::ERR_UNSUPPORTED_ACTION, "Not implemented" );
			return;
	}
	remUrl.addQueryItem( "format", "xml" );

	if ( getDBFile( remUrl ) )
	{
		QDomDocument doc;
		if ( doc.setContent( &m_httpBuffer, false ) )
		{
			QDomNodeList l;
			KIO::UDSEntry entry;
			switch ( pathComps.size() )
			{
				case 0:
					l = doc.documentElement().elementsByTagName( "make" );
					for ( int i=0; i<l.count(); i++ )
					{
						QString make = l.item( i ).toElement().text();
						KUrl makeUrl = url;
						makeUrl.addPath( "/" + make );
						createDirEntry( entry, make, makeUrl.url(), "print/folder" );
						listEntry( entry, false );
						PRINT_DEBUG << "make: " << make << endl;
					}
					break;
				case 1:
					l = doc.documentElement().elementsByTagName( "printer" );
					for ( int i=0; i<l.count(); i++ )
					{
						QString ID, name;
						for ( QDomNode n=l.item( i ).firstChild(); !n.isNull(); n=n.nextSibling() )
						{
							QDomElement e = n.toElement();
							if ( e.tagName() == "id" )
								ID = e.text();
							else if ( e.tagName() == "model" )
								name = e.text();
						}
						if ( !ID.isEmpty() && !name.isEmpty() )
						{
							KUrl printerUrl = url;
							printerUrl.addPath( "/" + ID );
							createDirEntry( entry, name, printerUrl.url(), "print/printermodel" );
							listEntry( entry, false );
							PRINT_DEBUG << "printer: " << ID << endl;
						}
					}
					break;
				case 2:
					l = doc.documentElement().elementsByTagName( "driver" );
					for ( int i=0; i<l.count(); i++ )
					{
						QString driver = l.item( i ).toElement().text();
						KUrl driverUrl = url;
						driverUrl.addPath( "/" + driver );
						createFileEntry( entry, driver, driverUrl.url(), "print/driver" );
						listEntry( entry, false );
						PRINT_DEBUG << "driver: " << driver << endl;
					}
					break;
				default:
					error( KIO::ERR_UNSUPPORTED_ACTION, "Not implemented" );
					return;
			}
			listEntry( KIO::UDSEntry(), true );
			finished();
		}
		else
		{
			if ( m_httpBuffer.buffer().size() == 0 )
				error( KIO::ERR_INTERNAL, i18n( "Empty data received (%1).", url.host() ) );
			else
				error( KIO::ERR_INTERNAL, i18n( "Corrupted/incomplete data or server error (%1).", url.host() ) );
		}
	}
	/*
	 * If error occurred while downloading, error has been called by
	 * getDBFile. No need for a "else" statement.
	 */
}

void KIO_Print::stat(const KUrl& url)
{
	if ( url.protocol() == "printdb" )
	{
		statDB( url );
		return;
	}

	PRINT_DEBUG << "stat: " << url.url() << endl;
	QStringList	path = url.encodedPathAndQuery( KUrl::RemoveTrailingSlash ).split( '/', QString::SkipEmptyParts);
	KIO::UDSEntry	entry;
	QString	mime;
	bool err(false);

	PRINT_DEBUG << "path components: " << path.join(", ") << endl;

	switch (path.count())
	{
		case 0:
			createDirEntry(entry, i18n("Print System"), "print:/", "print/folder");
			break;
		case 1:
			if (path[0].toLower() == "classes")
				createDirEntry(entry, i18n("Classes"), "print:/classes", "print/folder");
			else if (path[0].toLower() == "printers")
				createDirEntry(entry, i18n("Printers"), "print:/printers", "print/folder");
			else if (path[0].toLower() == "specials")
				createDirEntry(entry, i18n("Specials"), "print:/specials", "print/folder");
			else if (path[0].toLower() == "manager")
				createDirEntry(entry, i18n("Manager"), "print:/manager", "print/manager");
			else if (path[0].toLower().startsWith("jobs"))
				createFileEntry(entry, i18n("Jobs"), url.url(), "text/html");
			else
				err = true;
			break;
		case 2:
			if (path[0].toLower() == "printers")
				mime = "print/printer";
			else if (path[0].toLower() == "classes")
				mime = "print/class";
			else if (path[0].toLower() == "specials")
				mime = "print/printer";
			else
				err = true;
			createFileEntry(entry, path[1], "print:/"+path[0]+"/"+path[1], "text/html");
			break;
	}

	if (!err)
	{
		statEntry(entry);
		finished();
	}
	else
		error(KIO::ERR_DOES_NOT_EXIST, url.path());
}

void KIO_Print::statDB( const KUrl& url )
{
	PRINT_DEBUG << "statDB: " << url << endl;
	KIO::UDSEntry entry;
	QStringList pathComps = url.path().split( '/', QString::SkipEmptyParts);
	if ( pathComps.size() == 3 )
		createFileEntry( entry, i18n( "Printer driver" ), url.url(), "print/driver" );
	else
		createDirEntry( entry, i18n( "On-line printer driver database" ), url.url(), "inode/directory" );
	statEntry( entry );
	finished();
}

bool KIO_Print::getDBFile( const KUrl& src )
{
	PRINT_DEBUG << "downloading " << src.url() << endl;

	/* re-initialize the internal buffer */
	if ( m_httpBuffer.isOpen() )
		m_httpBuffer.close();
	m_httpError = 0;
	m_httpBuffer.open( QIODevice::WriteOnly|QIODevice::Truncate ); // be sure to erase the existing data

	/* start the transfer job */
	KIO::TransferJob *job = KIO::get( src, false, false );
	connect( job, SIGNAL( result( KJob* ) ), SLOT( slotResult( KJob* ) ) );
	connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ), SLOT( slotData( KIO::Job*, const QByteArray& ) ) );
	connect( job, SIGNAL( totalSize( KJob*, qulonglong ) ), SLOT( slotTotalSize( KJob*, qulonglong ) ) );
	connect( job, SIGNAL( processedSize( KJob*, qulonglong ) ), SLOT( slotProcessedSize( KJob*, qulonglong ) ) );
  QEventLoop eventLoop;
  connect(this, SIGNAL(leaveModality()),
          &eventLoop, SLOT(quit()));
  eventLoop.exec();

	m_httpBuffer.close();

	/* return the result */
	if ( m_httpError != 0 )
		error( m_httpError, m_httpErrorTxt );
	return ( m_httpError == 0 );
}

void KIO_Print::getDB( const KUrl& url )
{
	PRINT_DEBUG << "downloading PPD file for " << url.url() << endl;

	QStringList pathComps = url.path().split( '/', QString::SkipEmptyParts);
	if ( pathComps.size() != 3 )
		error( KIO::ERR_MALFORMED_URL, url.url() );
	else
	{
		KUrl remUrl;

		remUrl.setProtocol( "http" );
		remUrl.setHost( url.host() );
		remUrl.setPath( "/ppd-o-matic.cgi" );
		remUrl.addQueryItem( "driver", pathComps[ 2 ] );
		remUrl.addQueryItem( "printer", pathComps[ 1 ] );

		if ( getDBFile( remUrl ) )
		{
			mimeType( "text/plain" );
			data( m_httpBuffer.buffer() );
			finished();
		}
		/*
		 * no "else" statement needed, the error has
		 * already been emitted by the getDBFile function
		 */
	}
}

void KIO_Print::slotResult( KJob *j )
{
	/*
	 * store slave results for later user (job gets deleted
	 * after this function). Store only if no other error
	 * occurred previously (when writing to the buffer).
	 */
	if ( m_httpError == 0 )
	{
		m_httpError = j->error();
		m_httpErrorTxt = j->errorText();
	}

  emit leaveModality();
}

void KIO_Print::slotData( KIO::Job *j, const QByteArray& d )
{
	PRINT_DEBUG << "HTTP data received (size=" << d.size() << ")" << endl;
	if ( d.size() > 0 )
	{
		int len = m_httpBuffer.write( d );
		if ( len == -1 || len != ( int )d.size() )
		{
			m_httpError = KIO::ERR_INTERNAL;
			m_httpErrorTxt = "Unable to write to the internal buffer.";
			j->kill();
		}
	}
}

void KIO_Print::slotTotalSize( KJob*, qulonglong sz )
{
	totalSize( sz );
}

void KIO_Print::slotProcessedSize( KJob*, qulonglong sz )
{
	processedSize( sz );
}

void KIO_Print::get(const KUrl& url)
{
	if ( url.protocol() == "printdb" )
	{
		getDB( url );
		return;
	}

	QStringList	elems = url.encodedPathAndQuery().split( '/', QString::SkipEmptyParts);
	QString		group(elems[0].toLower()), printer(QUrl::fromPercentEncoding(elems[1].toAscii())), path, query;
	KMPrinter	*mprinter(0);

	if (group == "manager")
	{
		PRINT_DEBUG << "opening print management part" << endl;

		mimeType("print/manager");
		finished();
		return;
	}

	PRINT_DEBUG << "getting " << url.url() << endl;

	if (group.startsWith("jobs"))
	{
		int	p = group.indexOf('?');
		if (p != -1)
			query = group.mid(p+1);
		if (!query.isEmpty() && query != "jobs" && query != "completed_jobs")
		{
			error(KIO::ERR_MALFORMED_URL, QString());
			return;
		}
		PRINT_DEBUG << "listing jobs for all printers" << endl;
		showJobs(0, query == "completed_jobs");
		return;
	}

	int	p = printer.indexOf('?');
	if (p != -1)
	{
		query = printer.mid(p+1);
		printer = printer.left(p);
	}

	PRINT_DEBUG << "opening " << url.url() << endl;
	PRINT_DEBUG << "extracted printer name = " << printer << endl;

	KMManager::self()->printerList(false);
	mprinter = KMManager::self()->findPrinter(printer);
	if (!mprinter)
		path = locateData(printer.isEmpty() ? group : printer);

	if (elems.count() > 2 || (path.isEmpty() && group != "printers" && group != "classes" && group != "specials")
	    || (mprinter == 0 && path.isEmpty()))
	{
		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		return;
	}

	if (mprinter != 0)
	{
		if (!query.isEmpty() && query != "general")
		{
			if (query == "jobs")
				showJobs(mprinter, false);
			else if (query == "completed_jobs")
				showJobs(mprinter, true);
			else if (query == "driver")
				showDriver(mprinter);
			else
				error(KIO::ERR_MALFORMED_URL, QUrl::fromPercentEncoding(elems[1].toAscii()));
		}
		else if (group == "printers" && mprinter->isPrinter())
			showPrinterInfo(mprinter);
		else if (group == "classes" && mprinter->isClass(true))
			showClassInfo(mprinter);
		else if (group == "specials" && mprinter->isSpecial())
			showSpecialInfo(mprinter);
		else
			error(KIO::ERR_INTERNAL, i18n("Unable to determine object type for %1.", printer));
	}
	else if (!path.isEmpty())
		showData(path);
	else
		error(KIO::ERR_INTERNAL, i18n("Unable to determine source type for %1.", printer));
}

void KIO_Print::showPrinterInfo(KMPrinter *printer)
{
	if (!KMManager::self()->completePrinter(printer))
		error(KIO::ERR_INTERNAL, i18n("Unable to retrieve printer information for %1.", printer->name()));
	else
	{
		mimeType("text/html");

		QString	content;
		QString	filename = QString::fromLatin1("printer.template");
		if (!loadTemplate(filename, content))
		{
			error(KIO::ERR_INTERNAL, i18n("Unable to load template %1", filename));
			return;
		}

		content = content
				 .arg(i18n("Properties of %1", printer->printerName()))
				 .arg(i18n("Properties of %1", printer->printerName()))
				 .arg(buildMenu(i18n("General|Driver|Active jobs|Completed jobs").split('|', QString::SkipEmptyParts),
							 QString("?general|?driver|?jobs|?completed_jobs").split('|'),
							 0))
				 .arg(QString())
				 .arg(printer->pixmap())
				 .arg(printer->name())
				 .arg(i18n("General Properties"))
				 .arg(i18n("Type")).arg(printer->isRemote() ? i18n("Remote") : i18n("Local"))
				 .arg(i18n("State")).arg(printer->stateString())
				 .arg(i18n("Location")).arg(printer->location())
				 .arg(i18n("Description")).arg(printer->description())
				 .arg(i18n("URI")).arg(printer->uri().prettyUrl())
				 .arg(i18n("Interface (Backend)")).arg(printer->device())
				 .arg(i18n("Driver"))
				 .arg(i18n("Manufacturer")).arg(printer->manufacturer())
				 .arg(i18n("Model")).arg(printer->model())
				 .arg(i18n("Driver Information")).arg(printer->driverInfo());

		data(content.toLocal8Bit());
		finished();
	}
}

void KIO_Print::showClassInfo(KMPrinter *printer)
{
	if (!KMManager::self()->completePrinter(printer))
		error(KIO::ERR_INTERNAL, i18n("Unable to retrieve class information for %1.", printer->name()));
	else
	{
		mimeType("text/html");

		QString	content;
		QString	filename = QString::fromLatin1("class.template");
		if (!loadTemplate(filename, content))
		{
			error(KIO::ERR_INTERNAL, i18n("Unable to load template %1", filename));
			return;
		}

		QString		memberContent("<ul>\n");
		QStringList	members(printer->members());
		for (QStringList::ConstIterator it=members.begin(); it!=members.end(); ++it)
		{
			memberContent.append(QString::fromLatin1("<li><a href=\"print:/printers/%1\">%2</a></li>\n").arg(*it).arg(*it));
		}
		memberContent.append("</ul>\n");

		QString		typeContent = (printer->isImplicit() ? i18n("Implicit") : (printer->isRemote() ? i18n("Remote") : i18n("Local")));

		content = content
				 .arg(i18n("Properties of %1", printer->printerName()))
				 .arg(i18n("Properties of %1", printer->printerName()))
				 .arg(buildMenu(i18n("General|Active jobs|Completed jobs").split('|', QString::SkipEmptyParts),
							 QString( "?general|?jobs|?completed_jobs" ).split('|'),
							 0))
				 .arg(QString())
				 .arg(printer->pixmap())
				 .arg(printer->name())
				 .arg(i18n("General Properties"))
				 .arg(i18n("Type")).arg(typeContent)
				 .arg(i18n("State")).arg(printer->stateString())
				 .arg(i18n("Location")).arg(printer->location())
				 .arg(i18n("Description")).arg(printer->description())
				 .arg(i18n("URI")).arg(printer->uri().prettyUrl())
				 .arg(i18n("Members")).arg(memberContent);

		data(content.toLocal8Bit());
		finished();
	}
}

void KIO_Print::showSpecialInfo(KMPrinter *printer)
{
	mimeType("text/html");

	QString	content;
	QString filename = QString::fromLatin1("pseudo.template");
	if (!loadTemplate(filename, content))
	{
		error(KIO::ERR_INTERNAL, i18n("Unable to load template %1", filename));
		return;
	}

	QString	reqContent("<ul>\n");
	QStringList	requirements = printer->option("kde-special-require").split( ",", QString::SkipEmptyParts);
	for (QStringList::ConstIterator it=requirements.begin(); it!=requirements.end(); ++it)
		reqContent += ("<li>" + (*it) + "</li>\n");
	reqContent.append("</ul>\n");

	content = content
			 .arg(i18n("Properties of %1", printer->printerName()))
			 .arg(i18n("Properties of %1", printer->printerName()))
			 .arg(buildMenu(i18n("General").split('|', QString::SkipEmptyParts),
						 QString("?general").split('|'),
						 0))
			 .arg(QString())
			 .arg(printer->pixmap())
			 .arg(printer->name())
			 .arg(i18n("General Properties"))
			 .arg(i18n("Location")).arg(printer->location())
			 .arg(i18n("Description")).arg(printer->description())
			 .arg(i18n("Requirements")).arg(reqContent)
			 .arg(i18n("Command Properties"))
			 .arg(i18n("Command")).arg("<tt>"+printer->option("kde-special-command")+"</tt>")
			 .arg(i18n("Use Output File")).arg(printer->option("kde-special-file") == "1" ? i18n("Yes") : i18n("No"))
			 .arg(i18n("Default Extension")).arg(printer->option("kde-special-extension"));

	data(content.toLocal8Bit());
	finished();
}

bool KIO_Print::loadTemplate(const QString& filename, QString& buffer)
{
	QFile	f(KStandardDirs::locate("data", QString::fromLatin1("kdeprint/template/")+filename));
	if (f.exists() && f.open(QIODevice::ReadOnly))
	{
		QTextStream	t(&f);
		buffer = t.readAll();
		return true;
	}
	else
	{
		buffer.clear();
		return false;
	}
}

void KIO_Print::showData(const QString& pathname)
{
	PRINT_DEBUG << "sending data: " << pathname << endl;
	QFile	f(pathname);
	if (f.exists() && f.open(QIODevice::ReadOnly))
	{
		QByteArray	arr(f.readAll());
		mimeType(KMimeType::findByUrl(KUrl(pathname), 0, true, true)->name());
		data(arr);
		finished();
	}
	else
	{
		PRINT_DEBUG << "file not found" << endl;
		error(KIO::ERR_DOES_NOT_EXIST, pathname);
	}
}

/**
 * Locate a data in this order:
 *	- $KDEDIR/share/apps/kdeprint/template/
 *	- as a desktop icon
 */
QString KIO_Print::locateData(const QString& item)
{
	QString	path = KStandardDirs::locate("data", "kdeprint/template/"+item);
	if (path.isEmpty())
		path = KIconLoader::global()->iconPath(item, K3Icon::Desktop, true);
	return path;
}

void KIO_Print::showJobs(KMPrinter *prt, bool completed)
{
	mimeType("text/html");

	// Add the printer to the current list in the job manager
	KMJobManager::JobType	type = (completed ? KMJobManager::CompletedJobs : KMJobManager::ActiveJobs);
	KMJobManager	*mgr = KMJobManager::self();
	if (prt)
		mgr->addPrinter(prt->printerName(), type);
	else
	{
		QListIterator<KMPrinter *> pit((KMManager::self()->printerList()));
		while (pit.hasNext())
		{
			KMPrinter *itprint = pit.next();
			if (itprint->isVirtual() || itprint->isSpecial())
				continue;
			else
				mgr->addPrinter(itprint->printerName(), type);
		}
	}

	QString	content;
	QString	filename = QString::fromLatin1("jobs.template");
	if (!loadTemplate(filename, content))
	{
		error(KIO::ERR_INTERNAL, i18n("Unable to load template %1", filename));
		return;
	}

	if (prt)
	{
		content = content
				 .arg(i18n("Jobs of %1", prt->printerName()))
				 .arg(i18n("Jobs of %1", prt->printerName()))
				 .arg(prt->isClass () ?
						 buildMenu(i18n("General|Active jobs|Completed jobs").split('|', QString::SkipEmptyParts),
							 QString("?general|?jobs|?completed_jobs").split('|'),
							 (completed ? 2 : 1)) :
						 buildMenu(i18n("General|Driver|Active jobs|Completed jobs").split('|', QString::SkipEmptyParts),
							 QString("?general|?driver|?jobs|?completed_jobs").split('|'),
							 (completed ? 3 : 2)))
				 .arg(QString())
				 .arg(prt->pixmap())
				 .arg(prt->printerName());
	}
	else
	{
		content = content
				 .arg(i18n("All jobs"))
				 .arg(buildMenu(i18n("Active jobs|Completed jobs").split('|', QString::SkipEmptyParts),
							 QString("?jobs|?completed_jobs").split('|'),
							 (completed ? 1 : 0)))
				 .arg("fileprint")
				 .arg(i18n("All jobs"));
	}
	content = content.arg(i18n("ID")).arg(i18n("Owner")).arg(i18n("Printer")).arg(i18n("Name")).arg(i18n("State"));

	QString	jobContent, cellContent("<td>%1</td>\n");
	QListIterator<KMJob *> it(mgr->jobList());
	bool	flag(true);
	while (it.hasNext() && (flag = !flag))
	{
		KMJob *job = it.next();
		jobContent.append("<tr class=\"").append(flag ? "contentyellow" : "contentwhite").append("\">\n");
		jobContent.append(cellContent.arg(job->id()));
		jobContent.append(cellContent.arg(job->owner()));
		jobContent.append(cellContent.arg(job->printer()));
		jobContent.append(cellContent.arg(job->name()));
		jobContent.append(cellContent.arg(job->stateString()));
		jobContent.append("</tr>\n");
	}
	content = content.arg(jobContent);

	// remove the current printer to the current list in the job manager
	if (prt)
		mgr->removePrinter(prt->printerName(), type);
	else
	{
		QListIterator<KMPrinter *> pit((KMManager::self()->printerList()));
		while (pit.hasNext())
		{
			KMPrinter *pitCurrent = pit.next();
			if (pitCurrent->isVirtual() || pitCurrent->isSpecial())
				continue;
			else
				mgr->removePrinter(pitCurrent->printerName(), type);
		}
	}

	data(content.toLocal8Bit());
	finished();
}

void KIO_Print::showDriver(KMPrinter *prt)
{
	mimeType("text/html");

	QString	content;
	QString	filename = QString::fromLatin1("driver.template");
	if (!loadTemplate(filename, content))
	{
		error(KIO::ERR_INTERNAL, i18n("Unable to load template %1", filename));
		return;
	}

	DrMain	*driver = KMManager::self()->loadPrinterDriver(prt, true);
	content = content
			 .arg(i18n("Driver of %1", prt->printerName()))
			 .arg(i18n("Driver of %1", prt->printerName()))
			 .arg(buildMenu(i18n("General|Driver|Active jobs|Completed jobs").split('|', QString::SkipEmptyParts),
						 QString("?general|?driver|?jobs|?completed_jobs").split('|'),
						 1))
			 .arg(QString())
			 .arg(prt->pixmap())
			 .arg(prt->printerName() + "&nbsp;(" + (driver ? driver->get("text") : i18n("No driver found")) + ")");

	if (driver)
		content = content.arg(buildGroupTable(driver, false));
	else
		content = content.arg(QString());

	data(content.toLocal8Bit());
	finished();
}

#include "kio_print.moc"
