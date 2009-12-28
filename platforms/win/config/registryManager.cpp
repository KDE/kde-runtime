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
 
#include "registryManager.h"

#include <QtDebug>
#include <QFile>
#include <QDir>
#include <QObject>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QRect>
#include <QPoint>
#include <QDesktopServices>
#include <QProgressDialog>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#include <klocalizedstring.h>

#ifdef Q_OS_WIN
//for setting HIDDEN file property
#undef _WIN32_IE
#define _WIN32_IE 0x0400
#include <windows.h>
#endif

#if defined(__MINGW32__)
# define WIN32_CAST_CHAR (WCHAR*)
#else
# define WIN32_CAST_CHAR (LPCWSTR)
#endif

/*
The constructor predefines the user-specific root path in registry. For ini
file we're defining the KConfig object's filename*/

RegistryManager::RegistryManager(QWidget *parentPtr)
    : nativeSettings("HKEY_LOCAL_MACHINE", QSettings::NativeFormat)
{
    iniSettings = KSharedConfig::openConfig("kdeplatformrc", KConfig::SimpleConfig, "config");
    KConfigGroup shellGrp( iniSettings , "Custom Shell Options");
    customShell = shellGrp.entryMap();
    parentWidgetPtr = parentPtr;
    qDebug() << "The parentWidgetPtr = " << parentWidgetPtr ;
}

////////////////////////////////////////////////////////////////
// Wallpaper installation part
////////////////////////////////////////////////////////////////

void RegistryManager::installWallpapers() const
{
    QDir wallpaperDir(QDir::cleanPath(KStandardDirs::installPath("icon") + QString("../wallpapers")));
    QFile wpListFile(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + "/.wallpapers");
    wpListFile.open(QIODevice::ReadWrite | QIODevice::Text);
    QTextStream wpListFileStream(&wpListFile);

    QStringList wpList = wpListFileStream.readAll().split('\n', QString::SkipEmptyParts, Qt::CaseInsensitive);
    QStringList wallpapersList = wallpaperDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::NoSort);
    qDebug() << "The list of found wallpapers" << wallpapersList;

    //Getting screen resolution
    QDesktopWidget desktopWidget;
    QRect screen = desktopWidget.screenGeometry(0);
    QPoint bottomRight = screen.bottomRight();
    int rx = bottomRight.rx() + 1;
    int ry = bottomRight.ry() + 1;
    float aspect = (float) rx / (float) ry;
    qDebug() << rx << "x" << ry << " :: aspect ratio = " << aspect;

    //wallpapers are spearated into groups according to their aspect ratio
    //the following aspect ratios:
    // 1.6, 1.25, 1.33333
    QString resolution;
    if(aspect == 1.25) {
        resolution = "1280x1024";
    } else if (aspect > 1.3 && aspect < 1.34) {
        resolution = "1600x1200";
    } else if (aspect == 1.6) {
        resolution = "1920x1200";
    } else {
        //some rare aspect ratio, use the biggest wallpaper
        resolution = "1920x1200";
    }
    qDebug() << "Used " << resolution << " wallpaper resolution";
    
    QString oxyWallpaper, winWallpaper;

    QProgressDialog progress("Installing wallpapers...", "Cancel", 0, wallpapersList.size(), parentWidgetPtr);

    progress.setWindowModality(Qt::WindowModal);
    for (int i = 0; i < wallpapersList.size(); ++i){
    progress.setValue(i);
        oxyWallpaper = wallpaperDir.canonicalPath() + '/' + wallpapersList.at(i) + "/contents/images/" + resolution + ".jpg";
        winWallpaper = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + '/' + wallpapersList.at(i) + ".jpg";

    if (progress.wasCanceled()){
        break;
    } else {
        QFile wpFile(oxyWallpaper);
        wpFile.copy(winWallpaper);
    }

        if (wpList.indexOf(wallpapersList.at(i), 0) == -1){
            wpListFileStream << wallpapersList.at(i) << "\n";
        }
    }
    progress.setValue(wallpapersList.size());

    wpListFile.flush();
    wpListFile.close();

#ifdef Q_OS_WIN
    SetFileAttributesW(WIN32_CAST_CHAR wpListFile.fileName().utf16(), FILE_ATTRIBUTE_HIDDEN);
#endif
}

void RegistryManager::uninstallWallpapers() const
{
    QDir wallpaperDir(QDir::cleanPath(KStandardDirs::installPath("icon") + QString("../wallpapers")));
    
    QStringList wallpapersList = wallpaperDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::NoSort);
    qDebug() << "The list of installed wallpapers:" << wallpapersList;
    
    QString winWallpaper;
    for (int i = 0; i < wallpapersList.size(); ++i){
        winWallpaper = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + '/' + wallpapersList.at(i) + ".jpg";
        QFile wpFile(winWallpaper);
        wpFile.remove();
    }
    QFile wpListFile(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + "/.wallpapers");
    wpListFile.remove();
}

Qt::CheckState RegistryManager::isWallpapersInstalled() const
{
    QFile wpListFile(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + "/.wallpapers");

    if (!wpListFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        return Qt::Unchecked;
    } else {
        QTextStream wpListFileStream(&wpListFile);
        QStringList wpList = wpListFileStream.readAll().split('\n', QString::SkipEmptyParts, Qt::CaseInsensitive);
        
        QDir wallpaperDir(QDir::cleanPath(KStandardDirs::installPath("icon") + QString("../wallpapers")));
        QStringList wallpapersList = wallpaperDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::NoSort);
        
        if (wallpapersList == wpList){
            return Qt::Checked;
        } else {
            return Qt::PartiallyChecked;
        }
    }
    wpListFile.flush();
    wpListFile.close();
}

////////////////////////////////////////////////////////////////
// Control Panel entry installation part
////////////////////////////////////////////////////////////////
void RegistryManager::installCPlEntry() const
{
    QSettings settingsHKLM("HKEY_LOCAL_MACHINE", QSettings::NativeFormat);
    QSettings settingsHKCR("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
    if (!isCPlEntryInstalled()){
        qDebug() << "There's no Control Panel entry now, installing";
        settingsHKLM.setValue("SOFTWARE/Microsoft/Windows/CurrentVersion/Explorer/ControlPanel/NameSpace/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/Default", "KDE systemsettings");
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/Default", "KDE System Settings");
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/LocalizedString", i18n("KDE System Settings"));
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/InfoTip", i18n("All KDE settings in one place"));
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/System.ApplicationName", "KDE.systemsettings");
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/System.ControlPanel.Category", "1,6");
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/DefaultIcon/Default", QDir::toNativeSeparators(KStandardDirs::locate("exe", "systemsettings")));
        settingsHKCR.setValue("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/Shell/Open/Command/Default", QDir::toNativeSeparators(KStandardDirs::locate("exe", "systemsettings")));
        qDebug() << QDir::toNativeSeparators(KStandardDirs::locate("exe", "systemsettings"));
    } else {
        qDebug() << "There's already a Control Panel entry";
    }
}

void RegistryManager::uninstallCPlEntry() const
{
    QSettings settingsHKLM("HKEY_LOCAL_MACHINE", QSettings::NativeFormat);
    QSettings settingsHKCR("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
    if (isCPlEntryInstalled()){
        qDebug() << "There's a Control Panel entry now, removing";
        settingsHKLM.remove("SOFTWARE/Microsoft/Windows/CurrentVersion/Explorer/ControlPanel/NameSpace/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}");
        settingsHKCR.remove("CLSID/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}");
    } else {
        qDebug() << "There's no Control Panel entry installed";
    }
}

bool RegistryManager::isCPlEntryInstalled() const
{
    QSettings settingsHKLM("HKEY_LOCAL_MACHINE", QSettings::NativeFormat);
    bool isInstalled;

    if (settingsHKLM.value("SOFTWARE/Microsoft/Windows/CurrentVersion/Explorer/ControlPanel/NameSpace/{FADD6487-C270-4fae-A6C5-7BCF90E47C56}/Default").toString().isEmpty()){
        isInstalled = false;
    } else {
        isInstalled = true;
    }
    return isInstalled;
}

////////////////////////////////////////////////////////////////
// Cursors installation part
////////////////////////////////////////////////////////////////
void RegistryManager::installCursors() const
{
    qDebug() << "Installing Cursors schemes";
    QString cursorSchemeRegVal;
    QSettings schemeRegistry("HKEY_CURRENT_USER\\Control Panel\\Cursors\\Schemes", QSettings::NativeFormat);
    QDir cursorDir(QDir::cleanPath(KStandardDirs::installPath("icon") + QString("../cursors/oxygen")));

    QStringList cursorSchemeList = cursorDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::NoSort);

    QString schemeName;
    for (int i = 0; i < cursorSchemeList.size(); ++i){
        schemeName = cursorSchemeList.at(i);
        cursorSchemeRegVal = cursorDir.path() + '/' + schemeName + "/left_ptr.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/help.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/progress.ani,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/wait.ani,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/cross.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/text.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/pencil.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/circle.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/size_ver.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/size_hor.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/size_fdiag.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/size_bdiag.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/fleur.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/up_arrow.cur,";
        cursorSchemeRegVal += cursorDir.path() + '/' + schemeName + "/pointer.cur";
        cursorSchemeRegVal = cursorSchemeRegVal.replace('/', "\\\\");

        schemeRegistry.setValue(schemeName, cursorSchemeRegVal);
    }
}

void RegistryManager::uninstallCursors() const
{
    qDebug() << "Uninstalling Cursors schemes";
    QSettings schemeRegistry("HKEY_CURRENT_USER\\Control Panel\\Cursors\\Schemes", QSettings::NativeFormat);
    QStringList schemesList = schemeRegistry.allKeys();
    
    for (int i = 0; i < schemesList.size(); ++i){
        if(schemesList.at(i).contains("oxy-")){
            schemeRegistry.remove(schemesList.at(i));
        }
    }
}

bool RegistryManager::isCursorsInstalled() const
{
    QSettings schemeRegistry("HKEY_CURRENT_USER\\Control Panel\\Cursors\\Schemes", QSettings::NativeFormat);
    QStringList schemesList = schemeRegistry.allKeys();
    bool isInstalled;

    if (schemesList.contains("oxy-oxygen", Qt::CaseInsensitive)){
        isInstalled = true;
    } else {
        isInstalled = false;
    }
    return isInstalled;
}

////////////////////////////////////////////////////////////////
// File dialog switching part
////////////////////////////////////////////////////////////////
void RegistryManager::setUseNativeDialogs(bool isNative)
{
    KConfig kdeGlobSettings("kdeglobals");
    KConfigGroup shellGrp( &kdeGlobSettings , "KFileDialog Settings");
    shellGrp.writeEntry("Native", isNative);
}

bool RegistryManager::isNativeDialogsUsed()
{
    KConfig kdeGlobSettings("kdeglobals");
    KConfigGroup shellGrp( &kdeGlobSettings , "KFileDialog Settings");
    return shellGrp.readEntry("Native", true);
}

////////////////////////////////////////////////////////////////
// Enabling/disabling KDE load at user login
////////////////////////////////////////////////////////////////
void RegistryManager::setLoadAtLogin(bool startAtLogin)
{
    KConfig platformSetting("kdeplatformrc");
    QSettings settingsHKCU("HKEY_CURRENT_USER", QSettings::NativeFormat);
    if (!isLoadedAtLogin()){
        qDebug() << "KDE isn't set up to be started at user login! Setting...";
        settingsHKCU.setValue("SOFTWARE/Microsoft/Windows/CurrentVersion/Run/KDE", QString("\""+QDir::toNativeSeparators ( KStandardDirs::locate("exe", "kdeinit4"))+"\""));
    } else {
        qDebug() << "KDE is set up to be started at user login! Removing...";
        settingsHKCU.remove("SOFTWARE/Microsoft/Windows/CurrentVersion/Run/KDE");
    };
}

bool RegistryManager::isLoadedAtLogin()
{
    QSettings settingsHKCU("HKEY_CURRENT_USER", QSettings::NativeFormat);
    if (settingsHKCU.value("SOFTWARE/Microsoft/Windows/CurrentVersion/Run/KDE").isNull()) {
        return false;
    } else {
        return true;
    };
}

////////////////////////////////////////////////////////////////
// Shell switching part
////////////////////////////////////////////////////////////////
RegistryManager::Shell RegistryManager::getCurShell()
{
    QString shellCmd;
    shellCmd = nativeSettings.value("Software/Microsoft/Windows NT/CurrentVersion/Winlogon/Shell").toString();
    //Check if user has it's own settings for shell. If not - we take system-wide and set it as user's current one
    if(shellCmd.isEmpty()){
        QSettings nativeSystemSettings("HKEY_LOCAL_MACHINE", QSettings::NativeFormat);
        shellCmd = nativeSystemSettings.value("Software/Microsoft/Windows NT/CurrentVersion/Winlogon/Shell").toString();
        nativeSettings.setValue("Software/Microsoft/Windows NT/CurrentVersion/Winlogon/Shell", shellCmd);
    }

    Shell currentShell;

    if(shellCmd.toLower() == "explorer.exe"){
        currentShell = Native;
    } else if (shellCmd == KStandardDirs::locate("exe", "plasma")) {
        currentShell = Plasma;
    } else {
        currentShell = Custom;
    }

    return currentShell;
}

void RegistryManager::setCurShell(Shell shell)
{
    QString shellCmd;
    if(shell == Native){
        shellCmd = "explorer.exe";
    } else if (shell == Plasma) {
        shellCmd = KStandardDirs::locate("exe", "plasma");
    } else {
        shellCmd = customShell.value("Command");
    }

    //We should say custom shell options anyways, regardless of whether user decided to chose it, or not.
    setCustomShell(customShell);

    nativeSettings.setValue("Software/Microsoft/Windows NT/CurrentVersion/Winlogon/Shell", shellCmd);
    if(shell != Native){
        if(nativeSettings.value("Software/Microsoft/Windows/CurrentVersion/Explorer/BrowseNewProcess").toString() != "yes"){
            nativeSettings.setValue("Software/Microsoft/Windows/CurrentVersion/Explorer/BrowseNewProcess", "yes");
        }
        if(nativeSettings.value("Software/Microsoft/Windows/CurrentVersion/Explorer/DesktopProcess").toInt() != 1 ){
            nativeSettings.setValue("Software/Microsoft/Windows/CurrentVersion/Explorer/DesktopProcess", 1);
        }
    } else {
        if(nativeSettings.value("Software/Microsoft/Windows/CurrentVersion/Explorer/BrowseNewProcess").toString() != "no"){
            nativeSettings.setValue("Software/Microsoft/Windows/CurrentVersion/Explorer/BrowseNewProcess", "no");
        }
        if(nativeSettings.value("Software/Microsoft/Windows/CurrentVersion/Explorer/DesktopProcess").toInt() != 0 ){
            nativeSettings.setValue("Software/Microsoft/Windows/CurrentVersion/Explorer/DesktopProcess", 0);
        }
    }
}

void RegistryManager::setCustomShell(QMap<QString, QString> custShell)
{
    iniSettings = KSharedConfig::openConfig("kdeplatformrc", KConfig::SimpleConfig, "config");
    KConfigGroup shellGrp;

    QMap<QString, QString>::const_iterator i = custShell.constBegin();
    shellGrp = iniSettings->group("Custom Shell Options");
    while (i != custShell.constEnd()) {
        shellGrp.writeEntry(i.key(), i.value());
        ++i;
    }
    shellGrp.sync();
}
