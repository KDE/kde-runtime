/*
 *   kdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "faxctrl.h"
#include "kdeprintfax.h"
#include "defcmds.h"

#include <ktextedit.h>
#include <QFile>
#include <QTextStream>
#include <kpushbutton.h>
#include <QLayout>
#include <QRegExp>
#include <kprinter.h>
#include <Qt3Support/Q3SimpleRichText>
#include <QPainter>
#include <QtCore/QStack>
#include <QTextDocument>

#include <k3process.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdialog.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kwindowsystem.h>
#include <kemailsettings.h>
#include <kdebug.h>
#include <kstandardguiitem.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <stdlib.h>
#include <stdarg.h>
#include <krandom.h>

#define quote(x) K3Process::quote(x)

static QString stripNumber( const QString& s )
{
	KConfigGroup conf(KGlobal::config(), "Personal");

	// removes any non-numeric character, except ('+','*','#') (hope it's supported by faxing tools)
	QString strip_s = s;
	strip_s.replace( QRegExp( "[^\\d+*#]" ), "" );
	if ( strip_s.indexOf( '+' ) != -1 && conf.readEntry( "ReplaceIntChar", false) )
		strip_s.replace( "+", conf.readEntry( "ReplaceIntCharVal" ) );
	return strip_s;
}

static QString tagList( int n, ... )
{
	QString t;

	va_list ap;
	va_start( ap, n );
	for ( int i=0; i<n; i++ )
	{
		QString tag = va_arg( ap, const char* );
		tag.append( "(_(\\w|\\{[^\\}]*\\}))?" );
		if ( t.isEmpty() )
			t = tag;
		else
			t.append( "|" ).append( tag );
	}

	return t;
}

static QString processTag( const QString& match, const QString& value )
{
	QString v;
	int p = match.indexOf( '_' );
	if ( p != -1 )
	{
		if ( value.isEmpty() )
			v = "";
		else
		{
			if ( match[ p+1 ] == '{' )
			{
				v = match.mid( p+2, match.length()-p-3 );
				v.replace( "@@", quote( value ) );
			}
			else
				v = ( "-" + match.mid( p+1 ) + " " + quote( value ) );
		}
	}
	else
		v = quote( value );
	return v;
}

static bool isTag( const QString& m, const QString& t )
{
	return ( m == t || m.startsWith( t+"_" ) );
}

static QString replaceTags( const QString& s, const QString& tags, KdeprintFax *fax = NULL, const KdeprintFax::FaxItem& item = KdeprintFax::FaxItem() )
{
	// unquote variables (they will be replaced with quoted values later)

	QStack<bool> stack;
	KSharedConfig::Ptr conf = KGlobal::config();

	QString cmd = s;

	bool issinglequote=false;
	bool isdoublequote=false;
	QRegExp re_noquote("(\\$\\(|\\)|\\(|\"|'|\\\\|`|"+tags+")");
	QRegExp re_singlequote("('|"+tags+")");
	QRegExp re_doublequote("(\\$\\(|\"|\\\\|`|"+tags+")");
	for	( int i = re_noquote.indexIn(cmd);
		i != -1;
		i = (issinglequote?re_singlequote.indexIn(cmd,i)
			:isdoublequote?re_doublequote.indexIn(cmd,i)
			:re_noquote.indexIn(cmd,i))
	)
	{
		if (cmd[i]=='(') // (...)
		{
			// assert(isdoublequote == false)
			stack.push(isdoublequote);
			i++;
		}
		else if (cmd[i]=='$') // $(...)
		{
			stack.push(isdoublequote);
			isdoublequote = false;
			i+=2;
		}
		else if (cmd[i]==')') // $(...) or (...)
		{
			if (!stack.isEmpty())
				isdoublequote = stack.pop();
			else
				qWarning("Parse error.");
			i++;
		}
		else if (cmd[i]=='\'')
		{
			issinglequote=!issinglequote;
			i++;
		}
		else if (cmd[i]=='"')
		{
			isdoublequote=!isdoublequote;
			i++;
		}
		else if (cmd[i]=='\\')
			i+=2;
		else if (cmd[i]=='`')
		{
			// Replace all `...` with safer $(...)
			cmd.replace (i, 1, "$(");
			QRegExp re_backticks("(`|\\\\`|\\\\\\\\|\\\\\\$)");
			for (	int i2=re_backticks.indexIn(cmd,i+2);
				i2!=-1;
				i2=re_backticks.indexIn(cmd,i2)
			)
			{
				if (cmd[i2] == '`')
				{
					cmd.replace (i2, 1, ")");
					i2=cmd.length(); // leave loop
				}
				else
				{	// remove backslash and ignore following character
					cmd.remove (i2, 1);
					i2++;
				}
			}
			// Leave i unchanged! We need to process "$("
		}
		else
		{
			QString match, v;

			// get match
			if (issinglequote)
				match=re_singlequote.cap();
			else if (isdoublequote)
				match=re_doublequote.cap();
			else
				match=re_noquote.cap();

			// substitute %variables
			// settings
			if ( isTag( match, "%dev" ) )
			{
				conf->setGroup("Fax");
				v = processTag( match, conf->readEntry("Device", "modem") );

			}
			else if (isTag( match, "%server" ))
			{
				conf->setGroup( "Fax" );
				v = conf->readEntry("Server");
				if (v.isEmpty())
					v = getenv("FAXSERVER");
				if (v.isEmpty())
					v = QString::fromLatin1("localhost");
				v = processTag( match, v );
			}
			else if (isTag( match, "%page" ))
			{
				conf->setGroup( "Fax" );
				v = processTag( match, conf->readEntry( "Page", KGlobal::locale()->pageSize() == QPrinter::A4 ? "a4" : "letter" ) );
			}
			else if (match == "%res" )
			{
				conf->setGroup( "Fax" );
				if (conf->readEntry("Resolution", "High") == "High")
					v ="";
				else
					v = "-l";
			}
			else if (isTag( match, "%user" ))
			{
				conf->setGroup("Personal");
				v = processTag(match, conf->readEntry("Name", QString::fromLocal8Bit(getenv("USER"))));
			}
			else if (isTag( match, "%from" ))
			{
				conf->setGroup( "Personal" );
				v = processTag(match, conf->readEntry("Number"));
			}
			else if (isTag( match, "%email" ))
			{
				KEMailSettings	e;
				v = processTag(match, e.getSetting(KEMailSettings::EmailAddress));
			}
			// arguments
			else if (isTag( match, "%number" ))
				v = processTag( match, stripNumber( item.number) );
                        else if (isTag( match, "%rawnumber" ))
                                v = processTag( match, item.number );
			else if (isTag( match, "%name" ))
				v = processTag(match, item.name);
			else if (isTag( match, "%comment" ))
				v = processTag(match, fax->comment());
			else if (isTag( match, "%enterprise" ))
				v = processTag(match, item.enterprise);
			else if ( isTag( match, "%time" ) )
				v = processTag( match, fax->time() );
			else if ( isTag( match, "%subject" ) )
				v = processTag( match, fax->subject() );

			// %variable inside of a quote?
			if (isdoublequote)
				v='"'+v+'"';
			else if (issinglequote)
				v="'"+v+"'";

			cmd.replace (i, match.length(), v);
			i+=v.length();
		}
	}

	return cmd;
}

FaxCtrl::FaxCtrl(QWidget *parent, const char *name)
: QObject(parent)
{
  setObjectName( name );

	m_process = new K3Process();
	m_process->setUseShell(true);
	connect(m_process, SIGNAL(receivedStdout(K3Process*,char*,int)), SLOT(slotReceivedStdout(K3Process*,char*,int)));
	connect(m_process, SIGNAL(receivedStderr(K3Process*,char*,int)), SLOT(slotReceivedStdout(K3Process*,char*,int)));
	connect(m_process, SIGNAL(processExited(K3Process*)), SLOT(slotProcessExited(K3Process*)));
	connect(this, SIGNAL(faxSent(bool)), SLOT(cleanTempFiles()));
	m_logview = 0;
}

FaxCtrl::~FaxCtrl()
{
	slotCloseLog();
	delete m_process;
}

bool FaxCtrl::send(KdeprintFax *f)
{
	m_command = faxCommand();
	if (m_command.isEmpty())
		return false;

	// replace tags common to all fax "operations"
	m_command = replaceTags( m_command, tagList( 10, "%dev", "%server", "%page", "%res", "%user", "%from", "%email", "%comment", "%time", "%subject" ), f );

	m_log.clear();
	m_filteredfiles.clear();
	cleanTempFiles();
	m_files = f->files();
	m_faxlist = f->faxList();

	addLogTitle( i18n( "Converting input files to PostScript" ) );
	filter();

	return true;
}

void FaxCtrl::slotReceivedStdout(K3Process*, char *buffer, int len)
{
	QByteArray str(buffer, len);
	kDebug() << "Received stdout: " << str << endl;
	addLog(QString(str));
}

void FaxCtrl::slotProcessExited(K3Process*)
{
	// we exited a process: if there's still entries in m_files, this was a filter
	// process, else this was the fax process
	bool	ok = (m_process->normalExit() && ((m_process->exitStatus() & (m_files.count() > 0 ? 0x1 : 0xFFFFFFFF)) == 0));
	if ( ok )
	{
		if ( m_files.count() > 0 )
		{
			// remove first element
			m_files.erase(m_files.begin());
			if (m_files.count() > 0)
				filter();
			else
				sendFax();
		}
		else if ( !m_faxlist.isEmpty() )
			sendFax();
		else
			faxSent( true );
	}
	else
	{
		emit faxSent(false);
	}
}

QString FaxCtrl::faxCommand()
{
	KConfigGroup conf(KGlobal::config(), "System");
	QString	sys = conf.readPathEntry("System", "efax");
	QString cmd;
	if (sys == "hylafax")
		cmd = conf.readPathEntry("HylaFax", hylafax_default_cmd);
	else if (sys == "mgetty")
		cmd = conf.readPathEntry("Mgetty", mgetty_default_cmd);
	else if ( sys == "other" )
		cmd = conf.readPathEntry( "Other", QString() );
	else
		cmd = conf.readPathEntry("EFax", efax_default_cmd);
	if (cmd.startsWith("%exe_"))
		cmd = defaultCommand(cmd);
	return cmd;
}

QString FaxCtrl::pageSize()
{
	KConfigGroup conf(KGlobal::config(), "Fax");
	return conf.readEntry("Page", KGlobal::locale()->pageSize() == QPrinter::A4 ? "a4" : "letter");
}

void FaxCtrl::sendFax()
{
	if ( m_command.indexOf( "%files" ) != -1 )
	{
		// replace %files tag
		QString	filestr;
		for (QStringList::ConstIterator it=m_filteredfiles.begin(); it!=m_filteredfiles.end(); ++it)
			filestr += (quote(*it)+" ");
		m_command.replace("%files", filestr);
	}

	if ( !m_faxlist.isEmpty() )
	{
		KdeprintFax::FaxItem item = m_faxlist.first();
		m_faxlist.erase(m_faxlist.begin());

		addLogTitle( i18n( "Sending fax to %1 (%2)", item.number, item.name ) );

                QString cmd = replaceTags( m_command, tagList( 4, "%number", "%name", "%enterprise", "%rawnumber" ), NULL, item );
		m_process->clearArguments();
		*m_process << cmd;
		addLog(i18n("Sending to fax using: %1", cmd));
		if (!m_process->start(K3Process::NotifyOnExit, K3Process::AllOutput))
			emit faxSent(false);
		else
			emit message(i18n("Sending fax to %1...", item.number ));
	}
}

void FaxCtrl::filter()
{
	if (m_files.count() > 0)
	{
		QString	mimeType = KMimeType::findByUrl(KUrl(m_files[0]), 0, true)->name();
		if (mimeType == "application/postscript" || mimeType == "image/tiff")
		{
			emit message(i18n("Skipping %1...", m_files[0]));
			m_filteredfiles.prepend(m_files[0]);
			m_files.erase(m_files.begin());
			filter();
		}
		else
		{
			QString	tmp = KStandardDirs::locateLocal("tmp","kdeprintfax_") + KRandom::randomString(8);
			m_filteredfiles.prepend(tmp);
			m_tempfiles.append(tmp);
			m_process->clearArguments();
			*m_process << KStandardDirs::locate("data","kdeprintfax/anytops") << "-m" << quote(KStandardDirs::locate("data","kdeprintfax/faxfilters"))
				<< QString::fromLatin1("--mime=%1").arg(mimeType)
				<< "-p" << pageSize()
				<<  quote(m_files[0]) << quote(tmp);
			if (!m_process->start(K3Process::NotifyOnExit, K3Process::AllOutput))
				emit faxSent(false);
			else
				emit message(i18n("Filtering %1...", m_files[0]));
		}
	}
	else
	{
		sendFax();
	}
}

bool FaxCtrl::abort()
{
	if (m_process->isRunning())
		return m_process->kill();
	else
		return false;
}

void FaxCtrl::viewLog(QWidget *)
{
	if (!m_logview)
	{
		QWidget	*topView = new QWidget(0, Qt::WType_TopLevel|Qt::WStyle_DialogBorder|Qt::WDestructiveClose);
    topView->setObjectName( "LogView" );
		m_logview = new KTextEdit(topView);
		m_logview->setAcceptRichText( false );
		QPalette tempPalette( m_logview->viewport()->palette() );
		tempPalette.setBrush( QPalette::Background, Qt::white );
		m_logview->viewport()->setPalette( tempPalette );
		//m_logview->setReadOnly(true);
		//m_logview->setWordWrap(QTextEdit::NoWrap);
		QPushButton	*m_clear = new KPushButton(KStandardGuiItem::clear(), topView);
		QPushButton	*m_close = new KPushButton(KStandardGuiItem::close(), topView);
		QPushButton *m_print = new KPushButton( KStandardGuiItem::print(), topView );
		QPushButton *m_save = new KPushButton( KStandardGuiItem::saveAs(), topView );
		m_close->setDefault(true);
		connect(m_clear, SIGNAL(clicked()), SLOT(slotClearLog()));
		connect(m_close, SIGNAL(clicked()), SLOT(slotCloseLog()));
		connect(m_logview, SIGNAL(destroyed()), SLOT(slotCloseLog()));
		connect( m_print, SIGNAL( clicked() ), SLOT( slotPrintLog() ) );
		connect( m_save, SIGNAL( clicked() ), SLOT( slotSaveLog() ) );

		QVBoxLayout	*l0 = new QVBoxLayout(topView);
    l0->setMargin(10);
    l0->setSpacing(10);
		l0->addWidget(m_logview);
		QHBoxLayout	*l1 = new QHBoxLayout();
    l1->setMargin(0);
    l1->setSpacing(10);
		l0->addLayout(l1);
		l1->addStretch(1);
		l1->addWidget( m_save );
		l1->addWidget( m_print );
		l1->addWidget(m_clear);
		l1->addWidget(m_close);

		m_logview->setPlainText(m_log);

		topView->resize(450, 350);
		topView->show();
	}
	else
	{
		KWindowSystem::activateWindow(m_logview->parentWidget()->winId());
	}
}

void FaxCtrl::addLogTitle( const QString& s )
{
	QString t( s );
	t.prepend( '\n' ).append( '\n' );
	addLog( t, true );
}

void FaxCtrl::addLog(const QString& s, bool isTitle)
{
	QString t = Qt::escape(s);
	if ( isTitle )
		t.prepend( "<font color=red><b>" ).append( "</b></font>" );
	m_log.append( t + '\n' );
	if (m_logview)
		m_logview->append(t);
}

QString FaxCtrl::faxSystem()
{
	KConfigGroup conf(KGlobal::config(), "System");
	QString	s = conf.readEntry("System", "efax");
	s[0] = s[0].toUpper();
	return s;
}

void FaxCtrl::cleanTempFiles()
{
	for (QStringList::ConstIterator it=m_tempfiles.begin(); it!=m_tempfiles.end(); ++it)
		QFile::remove(*it);
	m_tempfiles.clear();
}

void FaxCtrl::slotClearLog()
{
	m_log.clear();
	if (m_logview)
		m_logview->clear();
}

void FaxCtrl::slotCloseLog()
{
	const QObject	*obj = sender();
	if (m_logview)
	{
		QTextEdit	*view = m_logview;
		m_logview = 0;
		if (obj && obj->inherits("QPushButton"))
			delete view->parentWidget();
kDebug() << "slotClose()" << endl;
	}
}

void FaxCtrl::slotPrintLog()
{
	if ( m_logview )
	{
		KPrinter printer;
		printer.setDocName( i18n( "Fax log" ) );
		printer.setDocFileName( "faxlog" );
		if ( printer.setup( m_logview->topLevelWidget(), i18n( "Fax Log" ) ) )
		{
			QPainter painter( &printer );
			QRect body( 0, 0, printer.width(), printer.height() ), view( body );
			//QString txt = m_logview->text();
			QString txt = m_log;

			txt.replace( '\n', "<br>" );
			txt.prepend( "<h2>" + i18n( "KDEPrint Fax Tool Log" ) + "</h2>" );

			kDebug() << "Log: " << txt << endl;
			Q3SimpleRichText richText( txt, m_logview->font() );

			richText.setWidth( &painter, body.width() );
			do
			{
				richText.draw( &painter, body.left(), body.top(), view, QColorGroup( m_logview->palette() ) );
				view.translate( 0, body.height() );
				painter.translate( 0, -body.height() );
				if ( view.top() >= richText.height() )
					break;
				printer.newPage();
			} while ( true );
		}
	}
}

void FaxCtrl::slotSaveLog()
{
	if ( m_logview )
	{
		QString filename = KFileDialog::getSaveFileName( QString(), QString(), m_logview );
		if ( !filename.isEmpty() )
		{
			QFile f( filename );
			if ( f.open( QIODevice::WriteOnly ) )
			{
				QTextStream t( &f );
				t << i18n( "KDEPrint Fax Tool Log" ) << endl;
				t << m_logview->toPlainText() << endl;
				f.close();
			}
			else
				KMessageBox::error( m_logview, i18n( "Cannot open file for writing." ) );
		}
	}
}

#include "faxctrl.moc"
