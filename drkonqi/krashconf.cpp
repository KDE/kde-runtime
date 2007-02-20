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
  QDBusConnection::sessionBus().registerObject("/krashinfo", this);
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

  KConfigGroup config(KGlobal::config(), "drkonqi");

  // maybe we should check if it's relative?
  QString configname = config.readEntry("ConfigName",
                                        QString("enduser"));

  QString debuggername = config.readEntry("Debugger",
                                           QString("gdb"));

  KConfig debuggers("appdata", QString::fromLatin1("debuggers/%1rc").arg(debuggername),
                    KConfig::NoGlobals );
  const KConfigGroup generalGroup = debuggers.group("General");

  m_debugger = generalGroup.readPathEntry("Exec");
  m_debuggerBatch = generalGroup.readPathEntry("ExecBatch");
  m_tryExec = generalGroup.readPathEntry("TryExec");
  m_backtraceCommand = generalGroup.readEntry("BacktraceCommand");
  m_removeFromBacktraceRegExp = generalGroup.readEntry("RemoveFromBacktraceRegExp");
  m_invalidStackFrameRegExp = generalGroup.readEntry("InvalidStackFrameRegExp");
  m_frameRegExp = generalGroup.readEntry("FrameRegExp");
  m_neededInValidBacktraceRegExp = generalGroup.readEntry("NeededInValidBacktraceRegExp");
  m_kcrashRegExp = generalGroup.readEntry("KCrashRegExp");

  KConfig preset("appdata", QString::fromLatin1("presets/%1rc").arg(configname),
                 KConfig::NoGlobals );

  const KConfigGroup errorDescrGroup = preset.group("ErrorDescription");
  if (errorDescrGroup.readEntry("Enable", false), true)
    m_errorDescriptionText = errorDescrGroup.readEntry("Name");

  const KConfigGroup whatToDoGroup = preset.group("WhatToDoHint");
  if (whatToDoGroup.readEntry("Enable", false))
    m_whatToDoText = whatToDoGroup.readEntry("Name");

  const KConfigGroup presetGeneralGroup = preset.group("General");
  m_showbugreport = presetGeneralGroup.readEntry("ShowBugReportButton", false);
  m_showdebugger = m_showbacktrace = m_pid != 0;
  if (m_showbacktrace)
  {
    m_showbacktrace = presetGeneralGroup.readEntry("ShowBacktraceButton", true);
    m_showdebugger = presetGeneralGroup.readEntry("ShowDebugButton", true);
  }
  m_disablechecks = presetGeneralGroup.readEntry("DisableChecks", false);

  bool b = presetGeneralGroup.readEntry("SignalDetails", true);

  QString str = QString::number(m_signalnum);
  // use group unknown if signal not found
  if (!preset.hasGroup(str))
    str = QLatin1String("unknown");
  const KConfigGroup signalGroup = preset.group(str);
  m_signalName = signalGroup.readEntry("Name");
  if (b)
    m_signalText = signalGroup.readEntry("Comment");
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
