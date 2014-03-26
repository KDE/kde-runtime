/***************************************************************************
 *   Copyright (C) 2009 Eduard Sukharev <kraplax@mail.ru>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef REGISTRYMANAGER_H
#define REGISTRYMANAGER_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include <KConfig>

//The following class is supposed to deal with all registry I/O
class RegistryManager
{
public:
    typedef enum { Plasma, Native, Custom } Shell;
    RegistryManager(QWidget *);
    Shell getCurShell();
    QMap<QString, QString> customShell;
    QMap<QString, QString> getCustomShell();
    void setCustomShell(QMap<QString, QString>);
    void setCurShell(Shell);
    
    void installCPlEntry() const;
    void uninstallCPlEntry() const;
    bool isCPlEntryInstalled() const;
    
    void installCursors() const;
    void uninstallCursors() const;
    bool isCursorsInstalled() const;
    
    void installWallpapers() const;
    void uninstallWallpapers() const;
/*
this will return Qt::Unchecked if wallpapers haven't been installed at all,
Qt::PartiallyChecked if wallpapers need to be updated
Qt::Checked  if wallpapers are up-to-date
*/
    Qt::CheckState isWallpapersInstalled() const;
    
    void setUseNativeDialogs(bool);
    bool isNativeDialogsUsed();

    void setLoadAtLogin(bool);
    bool isLoadedAtLogin();

private:
    QSettings nativeSettings;
    KSharedConfig::Ptr iniSettings;
    QWidget * parentWidgetPtr;
};

#endif /* REGISTRYMANAGER_H */
