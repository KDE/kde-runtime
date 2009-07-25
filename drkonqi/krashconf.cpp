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

#include "krashconf.h"

#include <config-drkonqi.h>

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KCmdLineArgs>
#include <KLocalizedString>
#include <KDebug>
#include <KStartupInfo>
#include <kmacroexpander.h>

#include <QtCore/QHash>
#include <QtDBus/QtDBus>

#ifdef HAVE_STRSIGNAL
# include <clocale>
# include <cstring>
#endif

KrashConfig :: KrashConfig(QObject *parent)
        : QObject(parent)
{
    readConfig();
}

KrashConfig :: ~KrashConfig()
{
    delete m_aboutData;
}

void KrashConfig :: readConfig()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    m_signalnum = args->getOption("signal").toInt();
    m_pid = args->getOption("pid").toInt();
    m_startedByKdeinit = args->isSet("kdeinit");
    m_safeMode = args->isSet("safer");
    m_execname = m_startedByKdeinit ? QLatin1String("kdeinit4") : args->getOption("appname");
    if (!args->getOption("apppath").isEmpty()) {
        m_execname.prepend(args->getOption("apppath") + '/');
    }

    QString programname = args->getOption("programname");
    if (programname.isEmpty()) {
        programname = I18N_NOOP2("@info unknown application", "unknown");
    }
    m_aboutData = new KAboutData(args->getOption("appname").toUtf8(),
                                 0,
                                 ki18nc("unknown application", programname.toUtf8()),
                                 args->getOption("appversion").toUtf8(),
                                 KLocalizedString(), // shortDescription
                                 KAboutData::License_Unknown,
                                 KLocalizedString(), KLocalizedString(), 0,
                                 args->getOption("bugaddress").toUtf8());

    QString startup_id(args->getOption("startupid"));
    if (!startup_id.isEmpty()) { // stop startup notification
#ifdef Q_WS_X11
        KStartupInfoId id;
        id.initId(startup_id.toLocal8Bit());
        KStartupInfo::sendFinish(id);
#endif
    }

    //load user settings
    KConfigGroup config(KGlobal::config(), "drkonqi");
    m_showdebugger = config.readEntry("ShowDebugButton", false);
    m_debuggerName = config.readEntry("Debugger", QString("gdb"));

    //for compatibility with drkonqi 1.0, if "ShowDebugButton" is not specified in the config
    //and the old "ConfigName" key exists and is set to "developer", we show the debug button.
    if (!config.hasKey("ShowDebugButton") &&
            config.readEntry("ConfigName") == "developer") {
        m_showdebugger = true;
    }

    //load debugger information from config file
    KConfig debuggers(QString::fromLatin1("debuggers/%1rc").arg(m_debuggerName),
                      KConfig::NoGlobals, "appdata");
    const KConfigGroup generalGroup = debuggers.group("General");

    m_debuggerCommand = generalGroup.readPathEntry("Exec", QString());
    m_debuggerBatchCommand = generalGroup.readPathEntry("ExecBatch", QString());
    m_tryExec = generalGroup.readPathEntry("TryExec", QString());
    m_backtraceCommand = generalGroup.readEntry("BacktraceCommand");
    
    m_isKDEBugzilla = (m_aboutData->bugAddress() == QLatin1String("submit@bugs.kde.org"));
    m_isReportMail = (m_aboutData->bugAddress().contains('@') && !m_isKDEBugzilla);
}

QString KrashConfig::signalName() const
{
#ifdef HAVE_STRSIGNAL
    const char * oldLocale = std::setlocale(LC_MESSAGES, NULL);
    char * savedLocale;
    if (oldLocale) {
        savedLocale = strdup(oldLocale);
    } else {
        savedLocale = NULL;
    }
    std::setlocale(LC_MESSAGES, "C");
    const char *name = strsignal(m_signalnum);
    std::setlocale(LC_MESSAGES, savedLocale);
    free(savedLocale);
    return QString::fromLocal8Bit(name != NULL ? name : "Unknown");
#else
    switch (m_signalnum) {
    case 4: return QString("SIGILL");
    case 6: return QString("SIGABRT");
    case 8: return QString("SIGFPE");
    case 11: return QString("SIGSEGV");
    default: return QString("Unknown");
    }
#endif
}

bool KrashConfig::isKDEBugzilla() const
{
    return m_isKDEBugzilla;
}

bool KrashConfig::isReportMail() const
{
    return m_isReportMail;
}

// replace some of the strings
void KrashConfig :: expandString(QString &str, ExpandStringUsage usage, const QString &tempFile) const
{
    QHash<QString, QString> map;
    map[QLatin1String("appname")] = (usage == ExpansionUsageRichText) ?
                                    i18nc("@info/rich","<application>%1</application>", appName()) :
                                    appName();

    map[QLatin1String("execname")] = (usage == ExpansionUsageRichText) ?
                                     i18nc("@info/rich","<command>%1</command>", executableName()) :
                                     executableName();

    map[QLatin1String("signum")] = QString::number(signalNumber());
    map[QLatin1String("signame")] = signalName();

    map[QLatin1String("progname")] = (usage == ExpansionUsageRichText) ?
                                     i18nc("@info/rich","<command>%1</command>", programName()) :
                                     programName();

    map[QLatin1String("pid")] = QString::number(pid());

    map[QLatin1String("tempfile")] = (usage == ExpansionUsageRichText) ?
                                     i18nc("@info/rich","<filename>%1</filename>", tempFile) :
                                     tempFile;

    if (usage == ExpansionUsageShell) {
        str = KMacroExpander::expandMacrosShellQuote(str, map);
    } else {
        str = KMacroExpander::expandMacros(str, map);
    }
}

#include "krashconf.moc"
