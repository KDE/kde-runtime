/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <stdlib.h>

#include <qstring.h>
#include <qlabel.h>
#include <qlayout.h>

#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kaboutdata.h>
#include <kbugreport.h>

#include "debuger.h"
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
  QWidget *page = addPage(i18n("General"));
  QHBoxLayout* hbox = new QHBoxLayout(page);
  hbox->setSpacing(20);
  hbox->setAutoAdd(TRUE);

  // picture of konqi
  QLabel *lab = new QLabel(page);
  lab->setBackgroundColor(Qt::white);
  lab->setFrameStyle(QFrame::Panel | QFrame::Raised);
  QPixmap pix(locate("appdata", QString::fromLatin1("pics/konqi.png")));
  lab->setPixmap(pix);

  new QLabel( generateText(), page );

  if (m_krashconf->showDebuger()) {
    page = addPage(i18n("Debuger"));
    hbox = new QHBoxLayout(page);
    hbox->setAutoAdd(TRUE);
    new KrashDebuger(m_krashconf, page);
  }

  showButton( User1, m_krashconf->showBugReport() );

  connect(this, SIGNAL(okClicked()), kapp, SLOT(quit()));
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
  // FIXME: this is a hack!!
  KInstance *instance = KGlobal::_instance;
  KGlobal::_instance = new KInstance(m_krashconf->aboutData());

  QWidget *w = new KBugReport;
  w->show();

  // restore
  delete KGlobal::_instance;
  KGlobal::_instance = instance;
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
