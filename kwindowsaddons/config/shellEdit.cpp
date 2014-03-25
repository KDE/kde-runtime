/***************************************************************************
 *   Copyright (C) 2009 Eduard Sukharev <kraplax@mail.ru>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <shellEdit.h>
#include <QDebug>
#include <KLocale>

ShellEdit::ShellEdit(const QString &shellName = i18n("Custom desktop shell"), const QString& shellDescription = i18n("This shell is reserved for user's custom shell. Press \"Setup\" button to setup your favourite shell."), const QString& shellCmd = "myshell_cmd.exe")
{
	setupUi(this);
	
	shellNameEdit->setText(shellName);
	shellCmdEdit->setText(shellCmd);
	shellDescrEdit->setText(shellDescription);
}

ShellEdit::~ShellEdit()
{
}
