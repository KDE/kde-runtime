/*****************************************************************
 * drkonqi - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <kconfig.h>
#include <kglobal.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "krashconf.h"

KrashConfig :: KrashConfig()
{
  readConfig();
}

KrashConfig :: ~KrashConfig()
{
  delete m_aboutData;
}

QString KrashConfig :: debugger() const
{
  return m_debugger;
}

void KrashConfig :: readConfig()
{
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  m_signalnum = args->getOption( "signal" ).toInt();
  m_pid = args->getOption( "pid" ).toInt();

  QCString programname = args->getOption("programname");
  if (programname.isEmpty()) programname.setStr(I18N_NOOP("unknown"));
  // leak some memory... Well. It's only done once anyway :-)
  const char * progname = qstrdup(programname);
  m_aboutData = new KAboutData(args->getOption("appname"),
		    progname,
		    args->getOption("appversion"),
		    0, 0, 0, 0,	0,
		    args->getOption("bugaddress"));

  KConfig *config = KGlobal::config();
  config->setGroup(QString::fromLatin1("drkonqi"));

  // maybe we should check if it's relative?
  QString configname = config->readEntry(QString::fromLatin1("ConfigName"),
					 QString::fromLatin1("enduser"));

  QString debuggername = config->readEntry(QString::fromLatin1("Debugger"),
					   QString::fromLatin1("gdb"));

  KConfig debuggers(QString::fromLatin1("debuggers/%1rc").arg(debuggername),
		    true, false, "appdata");

  debuggers.setGroup(QString::fromLatin1("General"));
  m_debugger = debuggers.readEntry(QString::fromLatin1("Exec"));

  KConfig preset(QString::fromLatin1("presets/%1rc").arg(configname), 
		 true, false, "appdata");

  preset.setGroup(QString::fromLatin1("ErrorDescription"));
  if (preset.readBoolEntry(QString::fromLatin1("Enable"), true))
    m_errorDescriptionText = preset.readEntry(QString::fromLatin1("Name"));

  preset.setGroup(QString::fromLatin1("WhatToDoHint"));
  if (preset.readBoolEntry(QString::fromLatin1("Enable")))
    m_whatToDoText = preset.readEntry(QString::fromLatin1("Name"));

  preset.setGroup(QString::fromLatin1("General"));
  m_showbugreport = preset.readBoolEntry(QString::fromLatin1("ShowBugReportButton"), true);
  m_showbacktrace = m_pid != 0;
  if (m_showbacktrace) {
    m_showbacktrace = preset.readBoolEntry(QString::fromLatin1("ShowBacktraceButton"), true);
    m_showdebugger = preset.readBoolEntry(QString::fromLatin1("ShowDebugButton"), false);
  }

  bool b = preset.readBoolEntry(QString::fromLatin1("SignalDetails"), true);

  QString str = QString::number(m_signalnum);
  // use group unknown if signal not found
  if (!preset.hasGroup(str)) str = QString::fromLatin1("unknown");
  preset.setGroup(str);
  m_signalName = preset.readEntry(QString::fromLatin1("Name"));
  if (b)
    m_signalText = preset.readEntry(QString::fromLatin1("Comment"));
}
