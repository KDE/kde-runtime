/*
 * dialoghelper.cpp
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "dialoghelper.h"

#include <kdebug.h>
#include <KPasswordDialog>

#include <QApplication>
#include <QTimer>
#include <QWeakPointer>

DialogHelper::DialogHelper(const QString & prompt, const QString & errorMessage)
    : m_prompt(prompt)
    , m_errorMessage(errorMessage)
{
    QTimer::singleShot(0, this, SLOT(run()));
}

void DialogHelper::run()
{
    bool notCancelled = false;
    QString password;

    QWeakPointer<KPasswordDialog> dlg = new KPasswordDialog;
    dlg.data()->setPrompt(m_prompt);

    if (!m_errorMessage.isEmpty()) {
        dlg.data()->showErrorMessage(m_errorMessage, KPasswordDialog::PasswordError);
    }

    notCancelled = dlg.data()->exec();
    if (dlg) {
        password = dlg.data()->password();
        dlg.data()->deleteLater();
    }

    QTextStream qout(stdout);
    qout << password;
    qout.flush();
    qApp->exit(notCancelled ? 0 : -1);
}
// vim: sw=4 et sts=4
