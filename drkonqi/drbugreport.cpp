/*****************************************************************
 * drkonki - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <qmultilineedit.h>

#include "drbugreport.h"

DrKBugReport::DrKBugReport(QWidget *parent, bool modal,
			   const KAboutData *aboutData)
  : KBugReport(parent, modal, aboutData)
{
}

void DrKBugReport::setText(const QString &str)
{
  m_lineedit->setText(str);
}
