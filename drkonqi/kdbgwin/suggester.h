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

#pragma once

#include "common.h"

/**
 * \brief Package suggester
 *
 * Whenever kdbgwin loads a dll from the kde/bin directory, and there
 * are no symbols to load, an instance of this class is signaled to
 * suggest to the user to install debug symbols for the package containing
 * the dll. For that to work, it needs to parse the *.mft files inside
 * kde/manifest, which contain file lists for each package.
 */
class PackageSuggester : public QObject
{
    Q_OBJECT
protected:
    QString m_kdeDirPath;
    QStringList m_modules;

    typedef QMap<QString, QString> TFilePackageMap;
    TFilePackageMap m_filepack;
public slots:

    /// Will be called whenever the current generator isn't able to load symbols
    /// for a specific module
    void OnMissingSymbol(const QString& module);

    /// Will be called when the generator finishes it's work. Suggester can kick
    /// in, check which packages are needed, and alert the user
    void OnFinished();
private:

    /// Returns true if module is a dll which belongs to KDE. Actually, this is a bit
    /// more complicated. We're in KDEROOT/bin/kdbgwin.exe. This function strips the
    /// kdbgwin.exe and bin, and checks if module contains KDEROOT. This doesn't
    /// necessarily mean it's a KDE dll (could be Qt, C runtime, dependencies, etc).
    bool IsKdeModule(const QString& module);

    /// Get the name of the package which contains this module
    QString GetPackage(const QString& module);
};
