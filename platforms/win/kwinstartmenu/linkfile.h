/* This file is part of the KDE Project

   Copyright (C) 2006-2008 Ralf Habacker <ralf.habacker@freenet.de>
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
#ifndef _LINKFILE_H
#define _LINKFILE_H

#include <QList>
#include <QString>

class LinkFile {
    public:
        LinkFile(const QString &_execPath, const QString &_linkPath, const QString &_description, const QString &_workingDir)
        {
            execPath     = _execPath;    
            linkPath     = _linkPath;
            description  = _description; 
            workingDir   = _workingDir;  
        }
        bool exists();
        bool create();
        bool remove();

        QString execPath;    
        QString linkPath;
        QString description; 
        QString workingDir;  
        friend QDebug operator<<(QDebug out, const LinkFile &c);
};

class LinkFiles {
    public:
        static bool scan(QList <LinkFile> &files, const QString &rootDir);
        static bool create(QList <LinkFile> &newFiles);
        static bool cleanup(QList <LinkFile> &newFiles, QList <LinkFile> &oldFiles);
};

#endif
// vim: ts=4 sw=4 et
