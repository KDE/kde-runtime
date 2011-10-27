/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "indexcleanertest.h"
#include "../indexcleaner.h"
#include "../fileindexerconfig.h"
#include "fileindexerconfigutils.h"

#include <KConfig>
#include <KConfigGroup>
#include <KTempDir>
#include <KDebug>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QScopedPointer>

#include <QtTest>

#include "qtest_kde.h"

using namespace Nepomuk::Test;


void IndexCleanerTest::testConstructExcludeFolderFilter()
{
    // create the full folder hierarchy which includes some special cases:
    KTempDir* mainDir = createTmpFolders(QStringList()
                                         << indexedRootDir
                                         << indexedSubDir
                                         << indexedSubSubDir
                                         << excludedSubSubDir
                                         << hiddenSubSubDir
                                         << ignoredSubFolderToIndexedHidden
                                         << indexedSubFolderToIndexedHidden
                                         << indexedSubDirToExcluded
                                         << indexedHiddenSubDirToExcluded
                                         << excludedRootDir
                                         << hiddenSubDir
                                         << indexedHiddenSubDir
                                         << ignoredRootDir
                                         << excludedRootDir);

    const QString dirPrefix = mainDir->name();

    // write the config
    writeIndexerConfig(QStringList()
                       << dirPrefix + indexedRootDir
                       << dirPrefix + indexedSubFolderToIndexedHidden
                       << dirPrefix + indexedHiddenSubDir
                       << dirPrefix + indexedSubDirToExcluded
                       << dirPrefix + indexedHiddenSubDirToExcluded,
                       QStringList()
                       << dirPrefix + excludedRootDir
                       << dirPrefix + excludedSubDir
                       << dirPrefix + excludedSubSubDir,
                       QStringList(),
                       false);

    // create our test config object
    QScopedPointer<Nepomuk::FileIndexerConfig> cfg(new Nepomuk::FileIndexerConfig());

    const QString expectedFilter
            = QString::fromLatin1("FILTER("
                                  "(?url!=<file://%1d1>) && "
                                  "(?url!=<file://%1d1/.sd4>) && "
                                  "(?url!=<file://%1d1/sd1/.ssd3/isfh2>) && "
                                  "(?url!=<file://%1d1/sd2/.isde2>) && "
                                  "(?url!=<file://%1d1/sd2/isde1>) && "
                                  "(!REGEX(STR(?url),'^file://%1d1/') || "
                                  "REGEX(STR(?url),'^file://%1d1/sd1/ssd2/') || "
                                  "(REGEX(STR(?url),'^file://%1d1/sd2/') && "
                                  "!REGEX(STR(?url),'^file://%1d1/sd2/.isde2/') && "
                                  "!REGEX(STR(?url),'^file://%1d1/sd2/isde1/')))) .")
              .arg(dirPrefix);
    QCOMPARE(Nepomuk::IndexCleaner::constructExcludeFolderFilter(cfg.data()),
             expectedFilter);
}

QTEST_KDEMAIN_CORE(IndexCleanerTest)

#include "indexcleanertest.moc"
