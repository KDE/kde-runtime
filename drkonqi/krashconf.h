/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifndef KRASHCONF_H
#define KRASHCONF_H

#include <kaboutdata.h>
#include <qstring.h>

class KrashConfig
{
 public:
  KrashConfig(KAboutData *aboutData, int signal, int pid);
  void readConfig();
  
  QString programName() const { return m_aboutData->programName(); };
  const char* appName() const { return m_aboutData->appName(); };
  const KAboutData *aboutData() const { return m_aboutData; };
  int signalNumber() const { return m_signalnum; };
  int pid() const { return m_pid; };
  bool showDebugger() const { return m_showdebugger; };
  bool showBugReport() const { return m_showbugreport; };
  QString signalName() const { return m_signalName; };
  QString signalText() const { return m_signalText; };
  QString whatToDoText() const { return m_whatToDoText; }
  QString errorDescriptionText() const { return m_errorDescriptionText; };

 private:
  KAboutData *m_aboutData;
  int m_pid;
  int m_signalnum;
  bool m_showdebugger;
  bool m_showbugreport;
  QString m_signalName;
  QString m_signalText;
  QString m_whatToDoText;
  QString m_errorDescriptionText;
};

#endif
