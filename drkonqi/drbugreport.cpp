/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <qmultilineedit.h>

#include <kmessagebox.h>
#include <klocale.h>

#include "drbugreport.moc"
#include "drbugreport.h"

DrKBugReport::DrKBugReport(QWidget *parent, bool modal,
			   const KAboutData *aboutData)
  : KBugReport(parent, modal, aboutData)
{
}

void DrKBugReport::setText(const QString &str)
{
  m_lineedit->setText(str);
  startstring = str.simplifyWhiteSpace();
}

void DrKBugReport::slotOk()
{
  if (!startstring.isEmpty() &&
      m_lineedit->text().simplifyWhiteSpace() == startstring)
  {
    QString msg = i18n("You have to edit the description\n"
		       "before the report can be sent.");
    KMessageBox::error(this,msg);
    return;
  }                            
  KBugReport::slotOk();
}

