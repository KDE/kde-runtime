/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Parts of this code were originally under the following license:

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
*/
#include "drkonqi.h"
#include "systeminformation.h"
#include "crashedapplication.h"
#include "drkonqibackends.h"

#include <QtCore/QPointer>
#include <QtCore/QTextStream>
#include <QtCore/QTimerEvent>
#include <KMessageBox>
#include <KFileDialog>
#include <KTemporaryFile>
#include <KCmdLineArgs>
#include <KIO/NetAccess>
#include <KCrash>
#include <KDebug>

DrKonqi::DrKonqi()
{
    m_backend = new KCrashBackend();
    m_systemInformation = new SystemInformation();
}

DrKonqi::~DrKonqi()
{
    delete m_systemInformation;
    delete m_backend;
}

//static
DrKonqi *DrKonqi::instance()
{
    static DrKonqi *drKonqiInstance = NULL;
    if (!drKonqiInstance) {
        drKonqiInstance = new DrKonqi();
    }
    return drKonqiInstance;
}

//based on KCrashDelaySetHandler from kdeui/util/kcrash.cpp
class EnableCrashCatchingDelayed : public QObject
{
public:
    EnableCrashCatchingDelayed() {
        startTimer(10000); // 10 s
    }
protected:
    void timerEvent(QTimerEvent *event) {
        kDebug() << "Enabling drkonqi crash catching";
        KCrash::setDrKonqiEnabled(true);
        killTimer(event->timerId());
        this->deleteLater();
    }
};

bool DrKonqi::init()
{
    if (!instance()->m_backend->init()) {
        cleanup();
        return false;
    } else { //all ok, continue initialization
        // Set drkonqi to handle its own crashes, but only if the crashed app
        // is not drkonqi already. If it is drkonqi, delay enabling crash catching
        // to prevent recursive crashes (in case it crashes at startup)
        if (crashedApplication()->fakeExecutableBaseName() != "drkonqi") {
            kDebug() << "Enabling drkonqi crash catching";
            KCrash::setDrKonqiEnabled(true);
        } else {
            new EnableCrashCatchingDelayed;
        }
        return true;
    }
}

void DrKonqi::cleanup()
{
    delete instance();
}

//static
SystemInformation *DrKonqi::systemInformation()
{
    return instance()->m_systemInformation;
}

//static
DebuggerManager* DrKonqi::debuggerManager()
{
    return instance()->m_backend->debuggerManager();
}

//static
CrashedApplication *DrKonqi::crashedApplication()
{
    return instance()->m_backend->crashedApplication();
}

//static
void DrKonqi::saveReport(const QString & reportText, QWidget *parent)
{
    if (KCmdLineArgs::parsedArgs()->isSet("safer")) {
        KTemporaryFile tf;
        tf.setSuffix(".kcrash");
        tf.setAutoRemove(false);

        if (tf.open()) {
            QTextStream textStream(&tf);
            textStream << reportText;
            textStream.flush();
            KMessageBox::information(parent, i18nc("@info",
                                                   "Report saved to <filename>%1</filename>.",
                                                   tf.fileName()));
        } else {
            KMessageBox::sorry(parent, i18nc("@info","Could not create a file in which to save the report."));
        }
    } else {
        QString defname = crashedApplication()->fakeExecutableBaseName() + '-'
                            + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".kcrash";
        if (defname.contains('/')) {
            defname = defname.mid(defname.lastIndexOf('/') + 1);
        }

        QWeakPointer<KFileDialog> dlg = new KFileDialog(defname, QString(), parent);
        dlg.data()->setSelection(defname);
        dlg.data()->setCaption(i18nc("@title:window","Select Filename"));
        dlg.data()->setOperationMode(KFileDialog::Saving);
        dlg.data()->setMode(KFile::File);
        dlg.data()->setConfirmOverwrite(true);
        dlg.data()->exec();

        if (dlg.isNull()) {
            //Dialog is invalid, it was probably deleted (ex. via DBus call)
            //return and do not crash
            return;
        }
        
        KUrl fileUrl = dlg.data()->selectedUrl();
        delete dlg.data();

        if (fileUrl.isValid()) {
            KTemporaryFile tf;
            if (tf.open()) {
                QTextStream ts(&tf);
                ts << reportText;
                ts.flush();
            } else {
                KMessageBox::sorry(parent, i18nc("@info","Cannot open file <filename>%1</filename> "
                                                         "for writing.", tf.fileName()));
                return;
            }

            if (!KIO::NetAccess::upload(tf.fileName(), fileUrl, parent)) {
                KMessageBox::sorry(parent, KIO::NetAccess::lastErrorString());
            }
        }
    }
}

