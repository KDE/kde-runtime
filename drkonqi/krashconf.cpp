/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id:$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <kconfig.h>
#include <kglobal.h>
#include <kaboutdata.h>

#include "krashconf.h"

KrashConfig :: KrashConfig(KAboutData *aboutData, int signal, int pid)
  : m_aboutData(aboutData), m_pid(pid), m_signalnum(signal)
{
  readConfig();
}

void KrashConfig :: readConfig()
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
    m_errorDescriptionText = preset.readEntry(QString::fromLatin1("Name"));

  preset.setGroup(QString::fromLatin1("WhatToDoHint"));
  if (preset.readBoolEntry(QString::fromLatin1("Enable")))
    m_whatToDoText = preset.readEntry(QString::fromLatin1("Name"));

  preset.setGroup(QString::fromLatin1("General"));
  m_showbugreport = preset.readBoolEntry(QString::fromLatin1("ShowBugReportbutton"), true);
  m_showdebuger = m_pid != 0;
  if (m_showdebuger)
    m_showdebuger = preset.readBoolEntry(QString::fromLatin1("ShowDebugButton"), true);

  bool b = preset.readBoolEntry(QString::fromLatin1("SignalDetails"), true);

  QString str = QString::number(m_signalnum);
  // use group unknown if signal not found
  if (!preset.hasGroup(str)) str = QString::fromLatin1("unknown");
  preset.setGroup(str);
  m_signalName = preset.readEntry(QString::fromLatin1("Name"));
  if (b)
    m_signalText = preset.readEntry(QString::fromLatin1("Comment"));
}

