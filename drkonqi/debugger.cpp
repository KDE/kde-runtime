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

#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>

#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <ktextbrowser.h>
#include <ktempfile.h>

#include "backtrace.h"
#include "krashconf.h"
#include "debugger.h"
#include "debugger.moc"

KrashDebugger :: KrashDebugger (const KrashConfig *krashconf, QWidget *parent, const char *name)
  : QWidget( parent, name ),
    m_krashconf(krashconf),
    m_proctrace(0)
{
  QVBoxLayout *vbox = new QVBoxLayout( this, 0, KDialog::marginHint() );
  vbox->setAutoAdd(TRUE);

  m_backtrace = new KTextBrowser(this);
  m_backtrace->setTextFormat(Qt::PlainText);
  m_backtrace->setFont(KGlobalSettings::fixedFont());

  QWidget *w = new QWidget( this );
  ( new QHBoxLayout( w, 0, KDialog::marginHint() ) )->setAutoAdd( true );
  m_status = new QLabel( w );
  m_status->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
  //m_copyButton = new KPushButton( KStdGuiItem::copy(), w );
  KGuiItem item( i18n( "C&opy" ), QString::fromLatin1( "editcopy" ) );
  m_copyButton = new KPushButton( item, w );
  connect( m_copyButton, SIGNAL( clicked() ), this, SLOT( slotCopy() ) );
  m_copyButton->setEnabled( false );
  m_saveButton = new KPushButton( m_krashconf->safeMode() ? KStdGuiItem::save() : KStdGuiItem::saveAs(), w );
  connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( slotSave() ) );
  m_saveButton->setEnabled( false );
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
  m_backtrace->setText(str); // replace with possibly post-processed backtrace
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
    KTempFile tf(QString::fromAscii("/tmp/"), QString::fromAscii(".kcrash"), 0666);
    if (!tf.status())
    {
      *tf.textStream() << m_backtrace->text();
      KMessageBox::information(this, i18n("Backtrace saved to %1").arg(tf.name()));
    }
    else
    {
      KMessageBox::sorry(this, i18n("Cannot create a file in which to save the backtrace"));
    }
  }
  else
  {
    QString filename = KFileDialog::getSaveFileName(QString::null, QString::null, this, i18n("Select Filename"));
    if (!filename.isEmpty())
    {
      QFile f(filename);
      
      if (f.exists()) {
        if (KMessageBox::Cancel == 
            KMessageBox::warningContinueCancel( 0,
              i18n( "A file named \"%1\" already exists. "
                    "Are you sure you want to overwrite it?" ).arg( filename ),
              i18n( "Overwrite File?" ),
              i18n( "&Overwrite" ) ))
            return;       
      }           
      
      if (f.open(IO_WriteOnly))
      {
        QTextStream ts(&f);
        ts << m_backtrace->text();
        f.close();
      }
      else
      {
        KMessageBox::sorry(this, i18n("Cannot open file %1 for writing").arg(filename));
      }
    }
  }
}

void KrashDebugger :: slotSomeError()
{
  m_status->setText(i18n("Unable to create a valid backtrace."));
  m_backtrace->setText(i18n("This backtrace appears to be of no use.\n"
      "This is probably because your packages are built in a way "
      "which prevents creation of proper backtraces, or the stack frame "
      "was seriously corrupted in the crash.\n\n" )
      + m_backtrace->text());
}

void KrashDebugger :: slotAppend(const QString &str)
{
  m_status->setText(i18n("Loading backtrace..."));

  // append doesn't work here because it will add a newline as well
  m_backtrace->setText(m_backtrace->text() + str);
}

void KrashDebugger :: showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  startDebugger();
}

void KrashDebugger :: startDebugger()
{
  // Only start one copy
  if (m_proctrace)
    return;

  m_status->setText(i18n("Loading symbols..."));

  m_proctrace = new BackTrace(m_krashconf, this);

  connect(m_proctrace, SIGNAL(append(const QString &)),
          SLOT(slotAppend(const QString &)));
  connect(m_proctrace, SIGNAL(done(const QString&)), SLOT(slotDone(const QString&)));
  connect(m_proctrace, SIGNAL(someError()), SLOT(slotSomeError()));

  m_proctrace->start();
}

