/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <qstring.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>

#include <klocale.h>
#include <kstddirs.h>
#include <kbugreport.h>

class KAboutData;

#include "debugger.h"
#include "krashconf.h"
#include "toplevel.h"
#include "toplevel.moc"

Toplevel :: Toplevel(const KrashConfig *krashconf, QWidget *parent, const char *name)
  : KDialogBase( Tabbed,
		 krashconf->programName(),
                 User1 | Ok,
                 Ok,
                 parent,
                 name,
                 true, // modal
                 true, // separator
		 i18n("Bug report")
		 ),
    m_krashconf(krashconf)
{
  QHBox *page = addHBoxPage(i18n("General"));
  page->setSpacing(20);

  // picture of konqi
  QLabel *lab = new QLabel(page);
  lab->setBackgroundColor(Qt::white);
  lab->setFrameStyle(QFrame::Panel | QFrame::Raised);
  QPixmap pix(locate("appdata", QString::fromLatin1("pics/konqi.png")));
  lab->setPixmap(pix);

  new QLabel( generateText(), page );

  if (m_krashconf->showDebugger()) {
    page = addHBoxPage(i18n("Debugger"));
    new KrashDebugger(m_krashconf, page);
  }

  showButton( User1, m_krashconf->showBugReport() );

  connect(this, SIGNAL(okClicked()), SLOT(accept()));
}

Toplevel :: ~Toplevel()
{
}

QString Toplevel :: generateText() const
{
  QString str;

  if (!m_krashconf->errorDescriptionText().isEmpty())
    str += i18n("<p><b>Short description</b></p><p>%1</p>")
      .arg(m_krashconf->errorDescriptionText());

  if (!m_krashconf->signalText().isEmpty())
    str += i18n("<p><b>What is this?</b></p><p>%1</p>")
      .arg(m_krashconf->signalText());

  if (!m_krashconf->whatToDoText().isEmpty())
    str += i18n("<p><b>What can I do?</b></p><p>%1</p>")
      .arg(m_krashconf->whatToDoText());

  // check if the string is still empty. if so, display a default.
  if (str.isEmpty())
    str = i18n("<p><b>Application crashed</b></p>"
	       "<p>The program %appname crashed.</p>");

  // scan the string for %appname etc
  expandString(str);

  return str;
}

// starting bug report
void Toplevel :: slotUser1()
{
  QWidget *w = new KBugReport(0, true, m_krashconf->aboutData());
  w->show();
}

// replace some of the strings
void Toplevel :: expandString(QString &str) const
{
  int pos = -1;
  while ( (pos = str.findRev('%', pos)) != -1 ) {
    if (str.mid(pos, 8) == QString::fromLatin1("%appname"))
      str.replace(pos, 8, QString::fromLatin1(m_krashconf->appName()));
    else if (str.mid(pos, 7) == QString::fromLatin1("%signum"))
      str.replace(pos, 7, QString::number(m_krashconf->signalNumber()));
    else if (str.mid(pos, 8) == QString::fromLatin1("%signame"))
      str.replace(pos, 8, m_krashconf->signalName());
    else if (str.mid(pos, 9) == QString::fromLatin1("%progname"))
      str.replace(pos, 9, m_krashconf->programName());

    pos -= 7; // the smallest keyword is 7
  }
}
