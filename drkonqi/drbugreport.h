/*****************************************************************
 * drkonki - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

class KAboutData;

#include <kbugreport.h>

class DrKBugReport : public KBugReport
{
 public:
  /**
   * Constructor.
   */
  DrKBugReport(QWidget *parent = 0, bool modal = true,
	       const KAboutData *aboutData = 0);

 public:
  /**
   * Allows the debugger to set the default text in the editor.
   */
  void setText(const QString &str);
};
