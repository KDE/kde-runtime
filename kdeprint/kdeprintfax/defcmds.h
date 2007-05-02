/*
 *   kdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef DEFCMDS_H
#define DEFCMDS_H

#include <QString>

#define efax_default_cmd	"%exe_fax %user_{NAME=\"@@\"} %dev_{DEV=@@} PAGE=%page %from_{FROM=@@} send %res %number %files"
#define hylafax_default_cmd    "%exe_sendfax -n %server_h %res %page_s %time_a %enterprise_x %comment_c %email_f -d %name_{@@@}%number %files"
#define	mgetty_default_cmd	"%exe_faxspool %user_F %email_f %name_D %time_t %number %files"

QString defaultCommand(const QString& cmd);

#endif
