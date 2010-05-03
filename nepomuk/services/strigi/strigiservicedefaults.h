/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _STRIGI_SERVICE_DEFAULTS_H_
#define _STRIGI_SERVICE_DEFAULTS_H_

#include <QtCore/QStringList>

namespace Nepomuk {
    const char* s_defaultExcludeFilters[]
    = { "po", "CVS", ".svn", ".git", "core-dumps",
        "lost+found", ".*", "*~", "*.part",
        "*.o", "*.la", "*.lo", "*.loT", "*.in",
        "*.csproj", "*.m4", "*.rej", "*.gmo",
        "*.orig", "*.pc", "*.omf", "*.aux",
        "*.tmp", "*.po", "*.vm*", "*.nvram",
        "*.rcore", "lzo", "autom4te", "conftest",
        "confstat", "Makefile", "SCCS", "litmain.sh",
        "libtool", "config.status", "confdefs.h",
        "CMakeFiles", "cmake_install.cmake",
        "CTestTestfile.cmake", "*.moc", "moc_*.cpp",
        "*.torrent", 0 };

    QStringList defaultExcludeFilterList()
    {
        QStringList l;
        for ( int i = 0; s_defaultExcludeFilters[i]; ++i )
            l << QLatin1String( s_defaultExcludeFilters[i] );
        return l;
    }
}

#endif
