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
#include <QStringList>

class LinkFile {
    public:
        /// create instance 
        LinkFile(const QString &_execPath, const QString &_linkPath, const QString &_description, const QString &_workingDir)
        {
            m_execPath     = _execPath;    
            m_execParams   = QString();
            m_linkPath     = _linkPath;
            m_description  = _description; 
            m_workingDir   = _workingDir;  
        }
        LinkFile(const QStringList &_execPath, const QString &_linkPath, const QString &_description, const QString &_workingDir)
        {
            if (_execPath.size() > 0)
                m_execPath     = _execPath[0];
            if (_execPath.size() > 1)
                m_execParams   = _execPath[1];    
            m_linkPath     = _linkPath;
            m_description  = _description; 
            m_workingDir   = _workingDir;  
        }
        
        /// check if link file exists
        bool exists();
        /// create link file from instance data 
        bool create();
        /// remove link file
        bool remove();
        /// read link file content into instance 
        bool read();

        QString execPath()    { return m_execPath; }    
        QString linkPath()     { return m_linkPath; }
        QString description() { return m_description; }
        QString workingDir()  { return m_workingDir; }  

        friend QDebug operator<<(QDebug out, const LinkFile &c);

    protected:
        QString m_execPath;    
        QString m_execParams;    
        QString m_linkPath;
        QString m_description; 
        QString m_workingDir;  

};

class LinkFiles {
    public:
        static bool scan(QList <LinkFile> &files, const QString &rootDir);
        static bool create(QList <LinkFile> &newFiles);
        static bool cleanup(QList <LinkFile> &newFiles, QList <LinkFile> &oldFiles);
};

#endif
// vim: ts=4 sw=4 et
