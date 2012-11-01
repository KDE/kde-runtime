/*
 * dialoghelper.h
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

#ifndef DIALOG_HELPER_H
#define DIALOG_HELPER_H

#include <QObject>

class DialogHelper: public QObject
{
Q_OBJECT
public:
    explicit DialogHelper(const QString & prompt, const QString & errorMessage = QString());

private:
    QString m_prompt;
    QString m_errorMessage;

private slots:
    void run();
};

#endif
// vim: sw=4 et sts=4
