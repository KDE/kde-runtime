/* This file is part of the KDE project

   Copyright (C) 2006-2008 Ralf Habacker <ralf.habacker@freenet.de>
   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "linkfile.h"

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <initguid.h>

#include <QDir>
#include <QFile>

#include <KDebug>
#include <kstandarddirs.h>
#include <kservicegroup.h>
// required by kdesktopfile.h -> should be in kdesktopfile.h 
#include <kconfiggroup.h>
#include <kdesktopfile.h>

#if defined(_MSC_VER)
#define MY_CAST(a) a
#else
// mingw needs char cast
#define MY_CAST(a) (CHAR *)(a)
#endif

/*
    add correct prefix for win32 filesystem functions
    described in msdn, but taken from Qt's qfsfileeninge_win.cpp
*/
static QString longFileName(const QString &path)
{
    QString absPath = QDir::convertSeparators(path);
    QString prefix = QLatin1String("\\\\?\\");
    if (path.startsWith("//") || path.startsWith("\\\\")) {
        prefix = QLatin1String("\\\\?\\UNC\\");
        absPath.remove(0, 2);
    }
    return prefix + absPath;
}

// from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/programmersguide/shell_int/shell_int_programming/shortcuts/shortcut.asp
// CreateLink - uses the Shell's IShellLink and IPersistFile interfaces
//              to create and store a shortcut to the specified object.
//
// Returns true if link <linkName> could be created, otherwise false.
//
// Parameters:
// fileName     - full path to file to create link to
// linkName     - full path to the link to be created
// description  - description of the link (for tooltip)

bool CreateLink(const QString &fileName, const QString &_linkName, const QString &description, const QString &workingDir = QString())
{
    HRESULT hres;
    IShellLinkW* psl;

    QString linkName = longFileName(_linkName);

    LPCWSTR lpszPathObj  = (LPCWSTR)fileName.utf16();
    LPCWSTR lpszPathLink = (LPCWSTR)linkName.utf16();
    LPCWSTR lpszDesc     = (LPCWSTR)description.utf16();
    LPCWSTR lpszWorkDir  = (LPCWSTR)workingDir.utf16();

    CoInitialize(NULL);
    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLinkW, (LPVOID*)&psl);

    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        if(!SUCCEEDED(psl->SetPath(lpszPathObj))) {
            kDebug() << "error setting path for link to " << fileName;
            psl->Release();
            return false;
        }
        if(!SUCCEEDED(psl->SetDescription(lpszDesc))) {
            kDebug() << "error setting description for link to " << description;
            psl->Release();
            return false;
        }
        if(!SUCCEEDED(psl->SetWorkingDirectory(lpszWorkDir))) {
            kDebug() << "error setting working Directory for link to " << workingDir;
            psl->Release();
            return false;
        }


        // Query IShellLink for the IPersistFile interface for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            hres = ppf->Save(lpszPathLink, TRUE);
            // Save the link by calling IPersistFile::Save.
            if(!SUCCEEDED(hres))
                kDebug() << "error saving link to " << linkName;

            ppf->Release();
        }
        psl->Release();
    } else {
        kDebug() << "Error: Got no pointer to the IShellLink interface.";
    }
    CoUninitialize(); // cleanup COM after you're done using its services
    return SUCCEEDED(hres);
}

bool LinkFile::read()
{
    LPCWSTR szShortcutFile = (LPCWSTR)m_linkPath.utf16();
    WCHAR szTarget[MAX_PATH];
    WCHAR szWorkingDir[MAX_PATH];
    WCHAR szDescription[MAX_PATH];

    IShellLink*    psl     = NULL;
    IPersistFile*  ppf     = NULL;
    bool           bResult = false;

#   if !defined(UNICODE)
        WCHAR wsz[MAX_PATH];
        if (0 == MultiByteToWideChar(CP_ACP, 0, MY_CAST(szShortcutFile), -1, wsz, MAX_PATH) )
            goto cleanup;
#   else
        LPCWSTR wsz = szShortcutFile;
#   endif

    if (FAILED( CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **) &psl) ))
        goto cleanup;

    if (FAILED( psl->QueryInterface(IID_IPersistFile, (void **) &ppf) ))
        goto cleanup;

    if (FAILED( ppf->Load(wsz, STGM_READ) ))
        goto cleanup;

    if (NOERROR != psl->GetPath(MY_CAST(szTarget), MAX_PATH, NULL, 0) )
        goto cleanup;
    m_execPath = QString::fromUtf16((const ushort*)szTarget);

    if (NOERROR != psl->GetWorkingDirectory(MY_CAST(szWorkingDir), MAX_PATH) )
        goto cleanup;
    m_workingDir = QString::fromUtf16((const ushort*)szWorkingDir);

    if (NOERROR != psl->GetDescription(MY_CAST(szDescription), MAX_PATH) )
        goto cleanup;
    m_description = QString::fromUtf16((const ushort*)szDescription);

    bResult = true;

cleanup:
    if (ppf) ppf->Release();
    if (psl) psl->Release();
    return bResult;
}

bool LinkFile::create()
{
    QString execPath;
    //  the create link api wraps the whole execpath  with '"' when there are spaces in the string, 
    //  e.g. between the path and the first parameter. To avoid this wrapping the path with '"' is required 
    // add parameter list to executable path 
    // -> disabled parameter support for now, because i had no luck to figure out the rule 
    // how to create a valid execpath *with* parameters 

#ifdef ENABLE_EXECPATH_COMMANDLINE_PARAMETER
    if (!m_execParams.isEmpty()) {
        QString execParams = m_execParams.replace("%i","").replace("%u","").replace("%c",m_description);
        execPath = m_execPath + " " + execParams.trimmed();
    }
    else
#endif
        execPath = m_execPath;

    return CreateLink(execPath,m_linkPath,m_description,m_workingDir);
}

bool LinkFile::remove()
{
    bool ret = QFile::remove(m_linkPath);
    QFileInfo fi(m_linkPath);
    QDir d;
    d.rmpath(fi.absolutePath());
    return ret;
}

bool LinkFile::exists()
{
    return QFile::exists(m_linkPath);
}


bool LinkFiles::scan(QList <LinkFile> &files, const QString &path)
{
    QDir aDir(path);
    bool has_err = false;
    if (aDir.exists())//QDir::NoDotAndDotDot
    {
        QFileInfoList entries = aDir.entryInfoList(QDir::NoDotAndDotDot | 
        QDir::Dirs | QDir::Files);
        int count = entries.size();
        foreach(QFileInfo entryInfo, entries)
        {
            QString _path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir())
            {
                has_err = scan(files,_path);
            }
            else
            {
                if (_path.toLower().endsWith(".lnk"))
                    files.append(LinkFile("",_path,"",""));
            }
        }
    }
    return has_err;
}

bool LinkFiles::create(QList <LinkFile> &newFiles)
{
    // create new link files 
    foreach(LinkFile linkFile, newFiles)
    {
        if (!linkFile.exists())
        {
            if (linkFile.create())
                kDebug() << "created" << linkFile;
            else
                kDebug() << "failed to create" << linkFile;
        }
    }
    return true;
}

bool LinkFiles::cleanup(QList <LinkFile> &newFiles, QList <LinkFile> &oldFiles)
{
    // delete not available linkfiles 
    foreach(LinkFile oldFile, oldFiles)
    {
        QString oldPath = QDir::fromNativeSeparators ( oldFile.linkPath().toLower() );
        bool found = false;
        foreach(LinkFile newFile, newFiles)
        {
            QString newPath = QDir::fromNativeSeparators ( newFile.linkPath().toLower());
            if (newPath == oldPath) 
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            kDebug() << "deleted" << oldFile << oldFile.remove();
        }
    }
    return true;
} 

QDebug operator<<(QDebug out, const LinkFile &c)
{
    out.space() << "LinkFile ("
        << "execPath"     << c.m_execPath
        << "linkPath" << c.m_linkPath
        << "description"  << c.m_description
        << "workingDir"   << c.m_workingDir
        << ")";
    return out;
}

 // vim: ts=4 sw=4 et
