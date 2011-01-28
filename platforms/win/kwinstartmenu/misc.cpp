/* This file is part of the KDE project

   Copyright (C) 2006-2011 Ralf Habacker <ralf.habacker@freenet.de>
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
#include "misc.h"

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <initguid.h>

#include <QString>
#include <QDir>
#include <QFile>

#undef KDE_NO_WARNING_OUTPUT
#undef QT_NO_DEBUG_OUTPUT
#include <KDebug>

#include <kstandarddirs.h>
#include <kservicegroup.h>
// required by kdesktopfile.h -> should be in kdesktopfile.h 
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kdeversion.h>

GlobalOptions globalOptions;

bool removeDirectory(const QString& aDir)
{
    QDir dir( aDir );
    bool has_err = false;
    if (dir.exists())//QDir::NoDotAndDotDot
    {
        QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | 
        QDir::Dirs | QDir::Files);
        int count = entries.size();
        foreach(QFileInfo entryInfo, entries)
        {
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir())
            {
                has_err = removeDirectory(path);
            }
            else
            {
                QFile file(path);
                if (!file.remove())
                    has_err = true;
            }
        }
        if (!dir.rmdir(dir.absolutePath()))
            has_err = true;
    }
    return(has_err);
}

QString getStartMenuPath(bool bAllUsers)
{
    int idl = bAllUsers ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
    HRESULT hRes;
    WCHAR wPath[MAX_PATH+1];

    hRes = SHGetFolderPathW(NULL, idl, NULL, 0, wPath);
    if (SUCCEEDED(hRes))
    {
        QString s = QString::fromUtf16((unsigned short*)wPath);
        return s;
    }
    return QString();
}

QStringList getInstalledKDEVersions()
{
    QStringList installedVersions; 
    QDir dir(getStartMenuPath());
    if (!dir.exists())
        return installedVersions;

    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs );
    int count = entries.size();
    foreach(QFileInfo entryInfo, entries) {
        if (entryInfo.fileName().startsWith(QLatin1String("KDE ")))
            installedVersions << entryInfo.fileName();
    }
    return installedVersions;
}

QString getKDEStartMenuRootEntry()
{
    QString version = KDE::versionString();
    QStringList versions = version.split(' '); 
    QString addOn = !globalOptions.rootCustomString.isEmpty() ?  globalOptions.rootCustomString + ' ' : "";
#ifdef QT_NO_DEBUG
    QString compileMode = "Release"; 
#else
    QString compileMode = "Debug"; 
#endif
    return "KDE " + versions[0] + ' ' + addOn + compileMode;
}

inline QString getWorkingDir() 
{
    return QDir::toNativeSeparators(KStandardDirs::installPath("exe"));
}

QString getKDEStartMenuPath()
{
    return getStartMenuPath() + '/' + getKDEStartMenuRootEntry();
}

KServiceGroup::Ptr findGroup(const QString &relPath)
{
    QString nextPart;
    QString alreadyFound("Settings/");
    QStringList rest = relPath.split('/');

    kDebug() << "Trying harder to find group " << relPath;
    for ( int i=0; i<rest.count(); i++)
        kDebug() << "Item (" << rest.at(i) << ")";

    while (!rest.isEmpty()) {
        KServiceGroup::Ptr tmp = KServiceGroup::group(alreadyFound);
        if (!tmp || !tmp->isValid())
            return KServiceGroup::Ptr();

        bool found = false;
        foreach (const KSycocaEntry::Ptr &e, tmp->entries(true, true)) {
            if (e->isType(KST_KServiceGroup)) {
                KServiceGroup::Ptr g(KServiceGroup::Ptr::staticCast(e));
                if ((g->caption()==rest.front()) || (g->name()==alreadyFound+rest.front())) {
                kDebug() << "Found group with caption " << g->caption()
                      << " with real name: " << g->name() << endl;
                found = true;
                rest.erase(rest.begin());
                alreadyFound = g->name();
                kDebug() << "ALREADY FOUND: " << alreadyFound;
                break;
                }
            }
        }

        if (!found) {
            kDebug() << "Group with caption " << rest.front() << " not found within "
                  << alreadyFound << endl;
            return KServiceGroup::Ptr();
        }

    }
    return KServiceGroup::group(alreadyFound);
}

bool generateMenuEntries(QList<LinkFile> &files, const KUrl &url, const QString &relPathTranslated)
{
    QString groupPath = url.path( KUrl::AddTrailingSlash );
      groupPath.remove(0, 1); // remove starting '/'

    KServiceGroup::Ptr grp = KServiceGroup::group(groupPath);

    if (!grp || !grp->isValid()) {
        grp = findGroup(groupPath);
        if (!grp || !grp->isValid()) {
            kDebug() << "Unknown settings folder";
            return false;
        }
    }

    unsigned int count = 0;

    foreach (const KSycocaEntry::Ptr &e, grp->entries(true, true)) {
        if (e->isType(KST_KServiceGroup)) {
            KServiceGroup::Ptr g(KServiceGroup::Ptr::staticCast(e));
            // Avoid adding empty groups.
            KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());
            if (subMenuRoot->childCount() == 0)
                continue;

            // Ignore dotfiles.
            if ((g->name().at(0) == '.'))
                continue;

            QString relPath = g->relPath();
            KUrl a = url; 
            a.setPath('/' + relPath);
            generateMenuEntries(files,a,relPathTranslated + '/' + g->caption());
        } else {
            KService::Ptr s(KService::Ptr::staticCast(e));

            // read exec attribute
            KDesktopFile df(KStandardDirs::locate("apps", s->entryPath()));
            if (df.readType() != "Application")
                continue;
                
            QString _exec = df.desktopGroup().readEntry("Exec","");
            QStringList cmd = _exec.split(' ');
            QString exec = cmd[0];
            QStringList arguments;
            if (cmd.size() > 1) {
                // ignore arguments completely when they contain a variable
                if (!(_exec.contains("%i") || _exec.contains("%u") || _exec.contains("%U") || _exec.contains("%c"))) {
                    arguments = cmd;
                    arguments.removeFirst();
                }
            }                

            // create executable path 
            QString execPath = KStandardDirs::findExe(exec);
            if (execPath.isEmpty()) { 
                kDebug() << "could not find executable for" << exec; 
                continue;
            }
            
            QString linkPath = getKDEStartMenuPath();

            if (globalOptions.useCategories)
                linkPath +=  relPathTranslated;
            linkPath += '/';
                            
            QString linkName = s->name();
            if (!s->genericName().isEmpty() && s->genericName() != s->name()) 
                linkName += " (" + s->genericName().replace('/','-') + ')';
            
            QString linkFilePath = linkPath + linkName + ".lnk";
            QFileInfo fi(linkFilePath);
            QDir d(fi.absolutePath());
            if(!d.exists()) {
                if(!d.mkpath(fi.absolutePath())) {
                    kDebug() << "Can't create directory " << d.absolutePath();
                    continue;
                }
            }
            QString workingDir = getWorkingDir();
            QString description = s->genericName();

            files.append(LinkFile(QStringList() << execPath << arguments,linkFilePath,description,workingDir));
        }
        count++;
    }
    return true;
}

/**
 check start menu for older kde installations and remove obsolate or non existing ones 

 This is done by the following rules: 
    1. If no entry in an installation points to an existing executable, this installation 
       has been removed. The relating start menu entries could be deleted.
    2. if start menu entries for non current kde installation have the same working 
       dir as the current installation, this should be removed too
*/
void removeObsolateInstallations()
{
    QStringList installedReleases = getInstalledKDEVersions();
    QString currentVersion = getKDEStartMenuRootEntry();
    kDebug() << "installed releases" << installedReleases;
    QFileInfo currentWorkDir(getWorkingDir());

    foreach(const QString &release,installedReleases) 
    {
        // skip current version 
        if (release == currentVersion)
            continue;
        // get all link files for a specific release
        QList<LinkFile> allReleasesFiles;
        LinkFiles::scan(allReleasesFiles, getStartMenuPath() + '/' + release);
        bool available = false;
        bool sameWorkingDir = false;
        foreach(LinkFile lf, allReleasesFiles)    //krazy:exclude=foreach
        {
            lf.read(); // this in not done by the LinkFile class by default 
            QFileInfo fi(lf.execPath());
            if (fi.exists())
                available = true;
            QFileInfo f1(lf.workingDir());
			// compare harmonized pathes
            if (f1.absoluteFilePath() == currentWorkDir.absoluteFilePath())
                sameWorkingDir = true;
        }
        if (!available || sameWorkingDir)
        {
            kDebug() << "removing obsolate installation" << getStartMenuPath() + '/' + release;
            removeDirectory(getStartMenuPath() + '/' + release);
        }
    }
}

void updateStartMenuLinks()
{
    removeObsolateInstallations();

    // generate list of installed linkfiles 
    QList<LinkFile> oldFiles;
    LinkFiles::scan(oldFiles, getKDEStartMenuPath());

    // create list of currently available link files 
    QList<LinkFile> newFiles;
    generateMenuEntries(newFiles,KUrl("applications:/"));

    // remove obsolate links 
    LinkFiles::cleanup(newFiles,oldFiles);
    // create new links 
    LinkFiles::create(newFiles);
}

void removeStartMenuLinks()
{
    removeObsolateInstallations();
    removeDirectory(getKDEStartMenuPath());
}

 // vim: ts=4 sw=4 et
