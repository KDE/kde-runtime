/* This file is part of the KDE Project

   Copyright (C) 2006-2010 Ralf Habacker <ralf.habacker@freenet.de>
   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef _MISC_H
#define _MISC_H

#include "linkfile.h"

#include <QString>
class QDir;

#include <KUrl>

/** 
 contains global options 
*/
class GlobalOptions {
public: 
    GlobalOptions() : useCategories(true) {}

    bool useCategories;
};

extern GlobalOptions globalOptions;

bool removeDirectory(const QString& aDir);
QString getStartMenuPath(bool bAllUsers=false);
QString getKDEStartMenuPath();
bool generateMenuEntries(QList<LinkFile> &files, const KUrl &url, const QString &relPathTranslated=QString());
void updateStartMenuLinks();
void removeStartMenuLinks();

#endif
// vim: ts=4 sw=4 et
