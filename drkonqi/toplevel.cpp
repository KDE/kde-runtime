/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <qfile.h>
#include <qstring.h>
#include <qlabel.h>
#include <qlayout.h>

#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kbugreport.h>

#include "toplevel.h"
#include "toplevel.moc"

Toplevel :: Toplevel(int _signalnum, const QString _appname, QWidget *parent, const char *name)
  : KDialogBase( Plain,
		 _appname,
                 User1 | Ok,
                 Ok,
                 parent,
                 name,
                 true, // modal
                 true, // separator
		 i18n("Bug report")
		 )
{
  QWidget *page = plainPage();
  QHBoxLayout* hbox = new QHBoxLayout(page);
  hbox->setSpacing(20);
  hbox->setAutoAdd(TRUE);

  appname = _appname;
  signalnum = _signalnum;

  // parse the configuration files
  readConfig();

  // picture of konqi
  QLabel *lab = new QLabel(page);
  lab->setFrameStyle(QFrame::Panel | QFrame::Raised);
  QPixmap pix(locate("data", QString::fromLatin1("kdeui/pics/aboutkde.png")));
  lab->setPixmap(pix);

  new QLabel( generateText(), page );

  connect(this, SIGNAL(okClicked()), kapp, SLOT(quit()));
}

Toplevel :: ~Toplevel()
{
}

QString Toplevel :: generateText() const
{
  QString str;

  if (!errordescription.isEmpty()) {
    str += i18n("<b>Short description</b><br>");
    str += errordescription;
  }

  if (!signaldetails.isEmpty()) {
    str += i18n("<br><b>What is this?</b><br>");
    str += signaldetails;
  }

  if (!whattodohint.isEmpty()) {
    str += i18n("<br><b>What can I do?</b><br>");
    str += whattodohint;
  }

#if 0
  str += i18n("<b>Application messages</b><br>\n");
  str += i18n("The application might have provided more information about "
	      "the problem, maybe where it occured and why. Click on "
	      "\"More...\" for more informations."
	      "<br><hr>\n"
	      "<b>Hint:</b> You can customize this dialog in the control "
	      "panel.");
#endif

  // check if the string is still empty. if so, display a default.
  if (str.isEmpty())
    str = i18n("<b>Application crashed</b><br>\n"
	       "The program %appname crashed.");

  // scan the string for %appname etc
  expandString(str);

  return str;
}

// starting bug report
void Toplevel :: slotUser1()
{
  // this doesn't work -- appdata isn't set to the old application.
  QWidget *w = new KBugReport();
  w->show();
}

void Toplevel :: readConfig()
{
  KConfig *config = KGlobal::config();
  config->setGroup(QString::fromLatin1("drkonqi"));

  // maybe we should check if it's relative?
  QString configname = config->readEntry(QString::fromLatin1("ConfigName"),
					 QString::fromLatin1("enduser"));

  KConfig preset(QString::fromLatin1("presets/%1rc").arg(configname), 
		 true, false, "appdata");

  preset.setGroup(QString::fromLatin1("ErrorDescription"));
  if (preset.readBoolEntry(QString::fromLatin1("Enable"), true))
    errordescription = preset.readEntry(QString::fromLatin1("Name"));

  preset.setGroup(QString::fromLatin1("WhatToDoHint"));
  if (preset.readBoolEntry(QString::fromLatin1("Enable")))
    whattodohint = preset.readEntry(QString::fromLatin1("Name"));

  preset.setGroup(QString::fromLatin1("General"));
  bool b = preset.readBoolEntry(QString::fromLatin1("SignalDetails"), true);
  QString str = QString::number(signalnum);
  // use group unknown if signal not found
  if (!preset.hasGroup(str)) str = QString::fromLatin1("unknown");
  preset.setGroup(str);
  signal = preset.readEntry(QString::fromLatin1("Name"));
  if (b)
    signaldetails = preset.readEntry(QString::fromLatin1("Comment"));
}

// replace some of the strings
void Toplevel :: expandString(QString &str) const
{
  int pos = -1;
  while ( (pos = str.findRev('%', pos)) != -1 ) {
    if (str.mid(pos, 8) == QString::fromLatin1("%appname"))
      str.replace(pos, 8, appname);
    else if (str.mid(pos, 7) == QString::fromLatin1("%signum"))
      str.replace(pos, 7, QString::number(signalnum));
    else if (str.mid(pos, 8) == QString::fromLatin1("%signame"))
      str.replace(pos, 8, signal);

    pos -= 7; // the smallest keyword is 7
  }
}
