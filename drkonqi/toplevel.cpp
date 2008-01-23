/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#include "toplevel.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>

#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include "backtrace.h"
#include "drbugreport.h"
#include "debugger.h"
#include "krashconf.h"

#include "toplevel.moc"

Toplevel :: Toplevel(KrashConfig *krashconf, QWidget *parent)
  : KDialog( parent ),
    m_krashconf(krashconf), m_bugreport(0)
{
  setCaption( krashconf->programName() );
  setButtons( User3 | User2 | User1 | Close );
  setDefaultButton( Close );
  showButtonSeparator( true );
  setButtonGuiItem( User1, KGuiItem(i18n("&Bug Report")) );
  setButtonGuiItem( User2, KGuiItem(i18n("&Debugger")) );

  QWidget *mainWidget = new QWidget(this);
  setMainWidget(mainWidget);

  QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(spacingHint());

  QLabel *info = new QLabel(generateText(), this);
  info->setTextInteractionFlags(Qt::TextBrowserInteraction);
  info->setOpenExternalLinks(true);
  info->setWordWrap(true);

  // if possible, try to fit the error description all on one line
  // the +20 pixels is some slop for QLabel
  int suggestedWidth = !m_krashconf->errorDescriptionText().isEmpty() ?
        info->fontMetrics().width(m_krashconf->errorDescriptionText()) + 2 * marginHint() + 20
        : info->sizeHint().width() + 100;
  // ... but try to make sure the dialog fits on an 800x600 screen
  int width = qMin(suggestedWidth, 750);
  info->setMinimumSize(width, info->heightForWidth(width));
  info->setAlignment(Qt::AlignJustify);
  QString styleSheet = QString("QLabel {"
                       "padding: 10px;"
                       "background-image: url(%1);"
                       "background-repeat: no-repleat;"
                       "background-position: right;"
                       "}").arg(KStandardDirs::locate("appdata", QLatin1String("pics/konqi.png")));
  info->setStyleSheet(styleSheet);
  mainLayout->addWidget(info);

  QCheckBox *detailsCheckBox = new QCheckBox("Show details", this);
  connect(detailsCheckBox, SIGNAL(toggled(bool)), SLOT(expandDetails(bool)));
  mainLayout->addWidget(detailsCheckBox);

  if (m_krashconf->showBacktrace())
  {
    m_detailDescriptionLabel = new QLabel(this);
    m_detailDescriptionLabel->setText(QString("<p style=\"margin: 10px;\">%1<br /><br />%2</p>")
                                              .arg(m_krashconf->signalText())
                                              .arg(i18n("Please attach the following backtrace to your bugreport.")));
    m_detailDescriptionLabel->setWordWrap(true);
    m_detailDescriptionLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_detailDescriptionLabel->setAlignment(Qt::AlignJustify);
    m_detailDescriptionLabel->setVisible(false);
    mainLayout->addWidget(m_detailDescriptionLabel);

    m_debugger = new KrashDebugger(m_krashconf, this);
    m_debugger->setVisible(false);
    mainLayout->addWidget(m_debugger);
  }

  showButton( User1, m_krashconf->showBugReport() );
  showButton( User2, m_krashconf->showDebugger() );
  showButton( User3, false );

  connect(this, SIGNAL(closeClicked()), SLOT(accept()));
  connect(this, SIGNAL(user1Clicked()), SLOT(slotUser1()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotUser2()));
  connect(this, SIGNAL(user3Clicked()), SLOT(slotUser3()));
  connect(m_krashconf, SIGNAL(newDebuggingApplication(const QString&)), SLOT(slotNewDebuggingApp(const QString&)));
}

Toplevel :: ~Toplevel()
{
}

void Toplevel :: expandDetails(bool expand)
{
  setUpdatesEnabled(false); // reduce flickering. remember to enable it at the end again.
  m_detailDescriptionLabel->setVisible(expand);
  m_debugger->setVisible(expand);
  QCoreApplication::processEvents(); // force update of the size hint first...
  resize(minimumSizeHint());
  setUpdatesEnabled(true);
}

QString Toplevel :: generateText() const
{
  QString str;

  if (!m_krashconf->errorDescriptionText().isEmpty())
    str += QString("<p style=\"margin-bottom: 6px;\"><b>%1</b></p>").arg(i18n("A Fatal Error Occurred")) +
       m_krashconf->errorDescriptionText();

  if (!m_krashconf->whatToDoText().isEmpty())
    str += QString("<p style=\"margin-top: 6px; margin-bottom: 6px;\"><b>%1</b></p>").arg(i18n("What can I do?")) +
       m_krashconf->whatToDoText();

  // check if the string is still empty. if so, display a default.
  if (str.isEmpty())
    str = i18n("<p>Application crashed</p>"
               "<p>The program %appname crashed.</p>");

  // scan the string for %appname etc
  m_krashconf->expandString(str, false);

  return str;
}

// starting bug report
void Toplevel :: slotUser1()
{
  if (m_bugreport)
    return;

  int i = KMessageBox::No;
  if ( m_krashconf->pid() != 0 )
    i = KMessageBox::warningYesNoCancel
      (this,
       i18n("<p>Do you want to generate a "
            "backtrace? This will help the "
            "developers to figure out what went "
            "wrong.</p>\n"
            "<p>Unfortunately this will take some "
            "time on slow machines.</p>"
            "<p><b>Note: A backtrace is not a "
            "substitute for a proper description "
            "of the bug and information on how to "
            "reproduce it. It is not possible "
            "to fix the bug without a proper "
            "description.</b></p>"),
       i18n("Include Backtrace"),KGuiItem(i18n("Generate")),KGuiItem(i18n("Do Not Generate")));

    if (i == KMessageBox::Cancel) return;

  m_bugreport = new DrKBugReport(this, true, m_krashconf->aboutData());

  if (i == KMessageBox::Yes) {
    QApplication::setOverrideCursor ( Qt::WaitCursor );

    // generate the backtrace
    BackTrace *backtrace = new BackTrace(m_krashconf, this);
    connect(backtrace, SIGNAL(someError()), SLOT(slotBacktraceSomeError()));
    connect(backtrace, SIGNAL(done(const QString &)),
            SLOT(slotBacktraceDone(const QString &)));

    backtrace->start();

    return;
  }

  int result = m_bugreport->exec();
  delete m_bugreport;
  m_bugreport = 0;
  if (result == KDialog::Accepted)
     close();
}

void Toplevel :: slotUser2()
{
  QString str = m_krashconf->debugger();
  m_krashconf->expandString(str, true);

  KProcess proc;
  proc.setShellCommand(str);
  proc.startDetached();
}

void Toplevel :: slotNewDebuggingApp(const QString& launchName)
{
  setButtonText( User3, launchName );
  showButton( User3, true );
}

void Toplevel :: slotUser3()
{
  m_krashconf->acceptDebuggingApp();
}

void Toplevel :: slotBacktraceDone(const QString &str)
{
  // Do not translate.. This will be included in the _MAIL_.
  QString buf = QLatin1String
    ("\n\n\nHere is a backtrace generated by DrKonqi:\n") + str;

  m_bugreport->setText(buf);

  QApplication::restoreOverrideCursor();

  m_bugreport->exec();
  delete m_bugreport;
  m_bugreport = 0;
}

void Toplevel :: slotBacktraceSomeError()
{
  QApplication::restoreOverrideCursor();

  KMessageBox::sorry(this, i18n("It was not possible to generate a backtrace."),
                     i18n("Backtrace Not Possible"));

  m_bugreport->exec();
  delete m_bugreport;
  m_bugreport = 0;
}
