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

#include <kDebug>
#include <kstandarddirs.h>
#include <kservicegroup.h>
// required by kdesktopfile.h -> should be in kdesktopfile.h 
#include <kconfiggroup.h>
#include <kdesktopfile.h>

bool removeDirectory(QDir &aDir)
{
    bool has_err = false;
    if (aDir.exists())//QDir::NoDotAndDotDot
    {
        QFileInfoList entries = aDir.entryInfoList(QDir::NoDotAndDotDot | 
        QDir::Dirs | QDir::Files);
        int count = entries.size();
        foreach(QFileInfo entryInfo, entries)
        {
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir())
            {
                has_err = removeDirectory(QDir(path));
            }
            else
            {
                QFile file(path);
                if (!file.remove())
                    has_err = true;
            }
        }
        if (!aDir.rmdir(aDir.absolutePath()))
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

KServiceGroup::Ptr findGroup(const QString &relPath)
{
	QString nextPart;
	QString alreadyFound("Settings/");
	QStringList rest = relPath.split( '/');

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

bool generateMenuEntries(QList<LinkFile> &files, KUrl &url, const QString &relPathTranslated)
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
            a.setPath("/" + relPath);
            generateMenuEntries(files,a,relPathTranslated + "/" + g->caption());
		} else {
			KService::Ptr s(KService::Ptr::staticCast(e));

            // read exec attribute
            KDesktopFile df(KStandardDirs::locate("apps", s->entryPath()));
            if (df.readType() != "Application")
                continue;
                
            QString exec = df.desktopGroup().readEntry("Exec","");
            QStringList cmd = exec.split(" ");
            if (cmd.size() >= 1)
                exec = cmd[0];

            // create executable path 
            QString execPath = KStandardDirs::findExe(exec);
            if (execPath.isEmpty()) { 
                kDebug() << "could not find executable for" << exec; 
                continue;
            }
            
            QString linkPath = getStartMenuPath() + "/KDE" + relPathTranslated + "/";
            QString linkName = s->name();
            if (!s->genericName().isEmpty() && s->genericName() != s->name()) 
                linkName += " (" + s->genericName() + ")";
            
            QString linkFilePath = linkPath + linkName + ".lnk";
            QFileInfo fi(linkFilePath);
            QDir d(fi.absolutePath());
            if(!d.exists()) {
                if(!d.mkpath(fi.absolutePath())) {
                    kDebug() << "Can't create directory " << d.absolutePath();
                    continue;
                }
            }
            QString workingDir = QDir::toNativeSeparators(KStandardDirs::installPath("exe"));
            QString description = "";

            files.append(LinkFile(execPath,linkFilePath,description,workingDir));
		}
		count++;
	}
    return true;
}

void updateStartMenuLinks()
{
    // generate list of installed linkfiles 
    QList<LinkFile> oldFiles;
    LinkFiles::scan(oldFiles, getStartMenuPath() + "/KDE");
    kDebug() << "oldFiles" << oldFiles;

    // create list of currently available link files 
    QList<LinkFile> newFiles;
    generateMenuEntries(newFiles,KUrl("applications:/"));
    kDebug() << "newFiles" << newFiles;

    // remove obsolate links 
    LinkFiles::cleanup(newFiles,oldFiles);
    // create new links 
    LinkFiles::create(newFiles);
}

void removeStartMenuLinks()
{
    removeDirectory(QDir(getStartMenuPath() + "/KDE"));
}

 // vim: ts=4 sw=4 et
