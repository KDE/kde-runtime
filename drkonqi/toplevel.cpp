/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * toplevel.cpp
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <qstring.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>

#include <kapp.h>
#include <klocale.h>

#include "toplevel.h"
#include "toplevel.moc"

Toplevel :: Toplevel(int _signalnum, const QCString appname, QWidget *parent, const char *name)
  : KDialog(parent, name, TRUE)
{
  QVBoxLayout* vbox = new QVBoxLayout(this,8);
  vbox->setAutoAdd(TRUE);

  signalnum = _signalnum;

  // this should depend on the signalnum
  QString signal = getSignal();

  setCaption( i18n("Application error: %1").arg(signal) );

  QString str;
  str = i18n("<b>Short description</b><br>"
	     "The application \"%1\" just received the %2 signal (%3).<br>"
	     "<b>What is this?</b><br>"
	     "An application mostly receives the %4 due to a bug in "
	     "the application. The application was asked to save its "
	     "documents.<br>"
	     "<b>What can I do?</b><br>"
	     "You might want to send a bug reaport to the author. Please "
	     "include as much information as possible, maybe the original "
	     "documents. If you have a way to reproduce the error, include "
	     "this also. Click on the button \"Bug raport\".<br>"
	     "<b>Application messages</b><br>"
	     "The application might have provided more information about "
	     "the problem, maybe where it occured and why. Click on "
	     "\"More...\" for more informations."
	     "<br><hr>"
	     "<b>Hint:</b> You can customize this dialog in the control panel.")
    .arg(appname)
    .arg(signal)
    .arg(signalnum)
    .arg(signal);

  new QLabel( str, this );

  QHBox* hbox = new QHBox(this);
  QPushButton* ok = new QPushButton(i18n("OK"), hbox);

  connect(ok, SIGNAL(clicked()), kapp, SLOT(quit()));
}

Toplevel :: ~Toplevel()
{
}

QString Toplevel :: getSignal()
{
  switch (signalnum)
    {
    case 11:
      return QString::fromLatin1("SIGSEGV");
    default:
      return i18n("Unknown (%1)").arg(signalnum);
    }
}
