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

#include "debugger.h"

#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kstandardguiitem.h>
#include <ktextbrowser.h>
#include <ktemporaryfile.h>

#include "backtrace.h"
#include "krashconf.h"
#include "debugger.moc"

KrashDebugger :: KrashDebugger (const KrashConfig *krashconf, QWidget *parent, const char *name)
  : QWidget( parent ),
    m_krashconf(krashconf),
    m_proctrace(0)
{
  setObjectName(name);

  QVBoxLayout *vbox = new QVBoxLayout( this );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->setMargin( KDialog::marginHint() );

  m_backtrace = new KTextBrowser(this);
  m_backtrace->setFont(KGlobalSettings::fixedFont());

  vbox->addWidget(m_backtrace);

  QWidget *w = new QWidget( this );
  QHBoxLayout *hbox = new QHBoxLayout( w );
  hbox->setMargin( 0 );
  hbox->setSpacing( KDialog::spacingHint() );

  m_status = new QLabel( w );
  m_status->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
  //m_copyButton = new KPushButton( KStandardGuiItem::copy(), w );
  hbox->addWidget(m_status);
  KGuiItem item( i18n( "C&opy" ), QLatin1String( "edit-copy" ) );
  m_copyButton = new KPushButton( item, w );
  connect( m_copyButton, SIGNAL( clicked() ), this, SLOT( slotCopy() ) );
  m_copyButton->setEnabled( false );
  hbox->addWidget(m_copyButton);
  m_saveButton = new KPushButton( m_krashconf->safeMode() ? KStandardGuiItem::save() : KStandardGuiItem::saveAs(), w );
  connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( slotSave() ) );
  m_saveButton->setEnabled( false );
  hbox->addWidget(m_saveButton);

  vbox->addWidget(w);
}

KrashDebugger :: ~KrashDebugger()
{
  // This will SIGKILL gdb and SIGCONT program which crashed.
  //  delete m_proctrace;
}

void KrashDebugger :: slotDone(const QString& str)
{
  m_status->setText(i18n("Done."));
  m_copyButton->setEnabled( true );
  m_saveButton->setEnabled( true );
  m_backtrace->setPlainText( m_prependText + str ); // replace with possibly post-processed backtrace
}

void KrashDebugger :: slotCopy()
{
  m_backtrace->selectAll();
  m_backtrace->copy();
}

void KrashDebugger :: slotSave()
{
  if (m_krashconf->safeMode())
  {
    KTemporaryFile tf;
    tf.setPrefix("/tmp/");
    tf.setSuffix(".kcrash");
    tf.setAutoRemove(false);
    if (tf.open())
    {
      QTextStream textStream( &tf );
      textStream << m_backtrace->toPlainText();
      textStream.flush();
      KMessageBox::information(this, i18n("Backtrace saved to %1", tf.fileName()));
    }
    else
    {
      KMessageBox::sorry(this, i18n("Cannot create a file to save the backtrace in"));
    }
  }
  else
  {
    QString filename = KFileDialog::getSaveFileName(QString(), QString(), this, i18n("Select Filename"));
    if (!filename.isEmpty())
    {
      QFile f(filename);

      if (f.exists()) {
        if (KMessageBox::Cancel ==
            KMessageBox::warningContinueCancel( 0,
              i18n( "A file named \"%1\" already exists. "
                    "Are you sure you want to overwrite it?", filename ),
              i18n( "Overwrite File?" ),
              KGuiItem( i18n( "&Overwrite" ) ) ))
            return;
      }

      if (f.open(QIODevice::WriteOnly))
      {
        QTextStream ts(&f);
        ts << m_backtrace->toPlainText();
        f.close();
      }
      else
      {
        KMessageBox::sorry(this, i18n("Cannot open file %1 for writing", filename));
      }
    }
  }
}

void KrashDebugger :: slotSomeError()
{
  m_status->setText(i18n("Unable to create a valid backtrace."));
  m_backtrace->setPlainText(i18n("This backtrace appears to be of no use.\n"
      "This is probably because your packages are built in a way "
      "which prevents creation of proper backtraces, or the stack frame "
      "was seriously corrupted in the crash.\n\n" )
      + m_backtrace->toPlainText());
}

void KrashDebugger :: slotAppend(const QString &str)
{
  m_status->setText(i18n("Loading backtrace..."));

  // append doesn't work here because it will add a newline as well
  m_backtrace->setPlainText(m_backtrace->toPlainText() + str);
}

void KrashDebugger :: showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  startDebugger();
}

void KrashDebugger :: startDebugger()
{
  // Only start one copy
  if (m_proctrace || !m_backtrace->toPlainText().isEmpty())
    return;

  QString msg;
  bool checks = performChecks( &msg );
  if( !checks && !m_krashconf->disableChecks())
  {
    m_backtrace->setPlainText( m_prependText +
        i18n( "The following options are enabled:\n\n" )
        + msg
        + i18n( "\nAs the usage of these options is not recommended -"
                " because they can, in rare cases, be responsible for KDE problems - a backtrace"
                " will not be generated.\n"
                "You need to turn these options off and reproduce"
                " the problem again in order to get a backtrace.\n" ));
    m_status->setText( i18n( "Backtrace will not be created."));
    return;
  }
  if( !msg.isEmpty())
  {
    m_prependText += msg + '\n';
    m_backtrace->setPlainText( m_prependText );
  }
  m_status->setText(i18n("Loading symbols..."));

  m_proctrace = new BackTrace(m_krashconf, this);

  connect(m_proctrace, SIGNAL(append(const QString &)),
          SLOT(slotAppend(const QString &)));
  connect(m_proctrace, SIGNAL(done(const QString&)), SLOT(slotDone(const QString&)));
  connect(m_proctrace, SIGNAL(someError()), SLOT(slotSomeError()));

  m_proctrace->start();
}

// this function check for "dangerous" settings, returns false
// and message in case some of them are activated
bool KrashDebugger::performChecks( QString* msg )
{
  bool ret = true;
  KConfig kdedcfg( QLatin1String( "kdedrc" ) );
  if( kdedcfg.group("General").readEntry( "DelayedCheck", false))
  {
    // ret = false; it's not that dangerous
    *msg += i18n( "System configuration startup check disabled.\n" );
  }
  return ret;
}
