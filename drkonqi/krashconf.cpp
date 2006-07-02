/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#include <kconfig.h>
#include <kglobal.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstartupinfo.h>
#include <kmacroexpander.h>

#include <QHash>
#include <QtDBus/QtDBus>

#include "krashconf.h"

KrashConfig :: KrashConfig()
{
  QDBus::sessionBus().registerObject("/krashinfo", this);
  new KrashAdaptor(this);
  readConfig();
}

KrashConfig :: ~KrashConfig()
{
  delete m_aboutData;
}

void KrashConfig :: registerDebuggingApplication(const QString& launchName)
{
  emit newDebuggingApplication( launchName );
}

void KrashConfig :: acceptDebuggingApp()
{
  emit acceptDebuggingApplication();
}

void KrashConfig :: readConfig()
{
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  m_signalnum = args->getOption( "signal" ).toInt();
  m_pid = args->getOption( "pid" ).toInt();
  m_startedByKdeinit = args->isSet("kdeinit");
  m_safeMode = args->isSet("safer");
  m_execname = args->getOption( "appname" );
  if ( !args->getOption( "apppath" ).isEmpty() )
    m_execname.prepend( args->getOption( "apppath" ) + '/' );

  QByteArray programname = args->getOption("programname");
  if (programname.isEmpty())
    programname = I18N_NOOP("unknown");
  // leak some memory... Well. It's only done once anyway :-)
  const char * progname = qstrdup(programname);
  m_aboutData = new KAboutData(args->getOption("appname"),
                               progname,
                               args->getOption("appversion"),
                               0, // shortDescription
                               KAboutData::License_Unknown,
                               0, 0, 0,
                               args->getOption("bugaddress"));

  QByteArray startup_id( args->getOption( "startupid" ));
  if (!startup_id.isEmpty())
  { // stop startup notification
#ifdef Q_WS_X11
    KStartupInfoId id;
    id.initId( startup_id );
    KStartupInfo::sendFinish( id );
#endif
  }

  KConfig *config = KGlobal::config();
  config->setGroup("drkonqi");

  // maybe we should check if it's relative?
  QString configname = config->readEntry("ConfigName",
                                         QString("enduser"));

  QString debuggername = config->readEntry("Debugger",
                                           QString("gdb"));

  KConfig debuggers(QString::fromLatin1("debuggers/%1rc").arg(debuggername),
                    true, false, "appdata");

  debuggers.setGroup("General");
  m_debugger = debuggers.readPathEntry("Exec");
  m_debuggerBatch = debuggers.readPathEntry("ExecBatch");
  m_tryExec = debuggers.readPathEntry("TryExec");
  m_backtraceCommand = debuggers.readEntry("BacktraceCommand");
  m_removeFromBacktraceRegExp = debuggers.readEntry("RemoveFromBacktraceRegExp");
  m_invalidStackFrameRegExp = debuggers.readEntry("InvalidStackFrameRegExp");
  m_frameRegExp = debuggers.readEntry("FrameRegExp");
  m_neededInValidBacktraceRegExp = debuggers.readEntry("NeededInValidBacktraceRegExp");
  m_kcrashRegExp = debuggers.readEntry("KCrashRegExp");

  KConfig preset(QString::fromLatin1("presets/%1rc").arg(configname),
                 true, false, "appdata");

  preset.setGroup("ErrorDescription");
  if (preset.readEntry("Enable", QVariant(false)).toBool(), true)
    m_errorDescriptionText = preset.readEntry("Name");

  preset.setGroup("WhatToDoHint");
  if (preset.readEntry("Enable", QVariant(false)).toBool())
    m_whatToDoText = preset.readEntry("Name");

  preset.setGroup("General");
  m_showbugreport = preset.readEntry("ShowBugReportButton", QVariant(false)).toBool();
  m_showdebugger = m_showbacktrace = m_pid != 0;
  if (m_showbacktrace)
  {
    m_showbacktrace = preset.readEntry("ShowBacktraceButton", QVariant(true)).toBool();
    m_showdebugger = preset.readEntry("ShowDebugButton", QVariant(true)).toBool();
  }
  m_disablechecks = preset.readEntry("DisableChecks", QVariant(false)).toBool();

  bool b = preset.readEntry("SignalDetails", QVariant(true)).toBool();

  QString str = QString::number(m_signalnum);
  // use group unknown if signal not found
  if (!preset.hasGroup(str))
    str = QLatin1String("unknown");
  preset.setGroup(str);
  m_signalName = preset.readEntry("Name");
  if (b)
    m_signalText = preset.readEntry("Comment");
}

// replace some of the strings
void KrashConfig :: expandString(QString &str, bool shell, const QString &tempFile) const
{
  QHash<QString,QString> map;
  map[QLatin1String("appname")] = QString::fromLatin1(appName());
  map[QLatin1String("execname")] = startedByKdeinit() ? QString::fromLatin1("kdeinit") : m_execname;
  map[QLatin1String("signum")] = QString::number(signalNumber());
  map[QLatin1String("signame")] = signalName();
  map[QLatin1String("progname")] = programName();
  map[QLatin1String("pid")] = QString::number(pid());
  map[QLatin1String("tempfile")] = tempFile;
  if (shell)
    str = KMacroExpander::expandMacrosShellQuote( str, map );
  else
    str = KMacroExpander::expandMacros( str, map );
}

#include "krashconf.moc"
