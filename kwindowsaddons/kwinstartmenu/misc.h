/* This file is part of the KDE Project

   Copyright (C) 2006-2011 Ralf Habacker <ralf.habacker@freenet.de>
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

#include <KApplication>
#include <KUrl>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

/** 
 contains global options 
*/
class KWinStartMenuSettings {
public: 
    KWinStartMenuSettings()
    {
        // we use global config, not user specific
        QString installRoot = KStandardDirs::installPath("config");
        m_config = new KConfig(installRoot + "kwinstartmenurc", KConfig::SimpleConfig);
        m_group = new KConfigGroup(m_config, "General");
    }

    ~KWinStartMenuSettings()
    {
        delete m_config;
        delete m_group;
    }

    bool useCategories()
    {
        return m_group->readEntry("UseCategories", true);
    }

    void setUseCategories(bool mode)
    {
        m_group->writeEntry("UseCategories", mode);
        m_config->sync();
    }

    QString customString() const
    {
        return m_group->readEntry("CustomString", "");
    }

    void setCustomString(const QString &s)
    {
        m_group->writeEntry("CustomString", s);
        m_config->sync();
    }

    QString nameString() const
    {
        return m_group->readEntry("NameString","KDE");
    }

    void setNameString(const QString &s)
    {
        m_group->writeEntry("NameString", s);
        m_config->sync();
    }

    QString versionString() const
    {
        return m_group->readEntry("VersionString", "");
    }

    void setVersionString(const QString &s)
    {
        m_group->writeEntry("VersionString", s);
        m_config->sync();
    }

    bool enabled()
    {
        return m_group->readEntry("Enabled", true);
    }

    void setEnabled(bool mode)
    {
        m_group->writeEntry("Enabled", mode);
        m_config->sync();
    }

protected:
    KConfig *m_config;
    KConfigGroup *m_group;
};

extern KWinStartMenuSettings settings;

bool removeDirectory(const QString& aDir);
QString getStartMenuPath(bool bAllUsers=false);
QString getKDEStartMenuPath();
bool generateMenuEntries(QList<LinkFile> &files, const KUrl &url, const QString &relPathTranslated=QString());

// unconditional install start menu links for  current installation
void installStartMenuLinks();

// update start menu links for current installation
void updateStartMenuLinks();

// remove start menu links from current installation
void removeStartMenuLinks();

/** 
  remove unused start menu links from different installation
  This function searches for start menu entries not backed by a real installation
*/
void cleanupStartMenuLinks();

#endif
// vim: ts=4 sw=4 et
