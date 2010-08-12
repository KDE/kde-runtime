/******************************************************************
 *
 * kdbgwin - Helper application for DrKonqi
 *
 * This file is part of the KDE project
 *
 * Copyright (C) 2010 Ilie Halip <lupuroshu@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>
 *****************************************************************/

#include "suggester.h"
#include <KMessageBox>

void PackageSuggester::OnMissingSymbol(const QString& module)
{
    if (IsKdeModule(module))
    {
        m_modules << module;
    }
}

void PackageSuggester::OnFinished()
{
    QString str = QString::fromLatin1
    (
        "The information displayed here might not be useful because the following "
        "files have no corresponding debugging information: "
    );
    QStringList::iterator i;
    for (i = m_modules.begin(); i != m_modules.end(); i++)
    {
        str.append(*i);
        str.append(", ");
    }
    str = str.mid(0, str.length() - 2);
    str.append(". Do you wish to install debug packages now?");
    //MessageBoxA(NULL, whatToShow.toLatin1().constData(), "", MB_OK);
    KMessageBox::warningYesNo(NULL, str);
}

// I need the file name for the current module, which is the executable
// itself. GetCurrentProcess() returns a HANDLE, which doesn't seem to
// work when cast to a HMODULE, so, get the __ImageBase, which is castable
// to a HMODULE, and works for the current process (not thread!)
extern "C" IMAGE_DOS_HEADER __ImageBase;

bool PackageSuggester::IsKdeModule(const QString& module)
{
    // get path to current process, strip executable,
    // check if module contains the path
    // again the MinGW wchar_t issue (which it treats as 4 bytes), thus the ushort
    if (m_kdeDirPath.isEmpty())
    {
        ushort modulePath[MAX_PATH] = {0};
        GetModuleFileName((HMODULE) &__ImageBase, (LPWCH) modulePath, MAX_PATH);
        
        PathRemoveFileSpec((LPWSTR) modulePath);
        PathRemoveBackslash((LPWSTR) modulePath);
        PathRemoveFileSpec((LPWSTR) modulePath);
        PathRemoveBackslash((LPWSTR) modulePath);
        m_kdeDirPath = QString::fromUtf16(modulePath);
    }
    assert(!m_kdeDirPath.isEmpty());
    if (module.startsWith(m_kdeDirPath))
    {
        return true;
    }
    else
    {
        return false;
    }
}

QString GetPackage(const QString& module)
{
    QString package;
    // scan m_kdeDirPath+"\manifest" for *.mft files
    return package;
}
