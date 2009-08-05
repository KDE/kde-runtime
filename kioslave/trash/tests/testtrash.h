/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

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

#ifndef TESTTRASH_H
#define TESTTRASH_H

#include <QObject>

class TestTrash : public QObject
{
    Q_OBJECT

public:
    TestTrash() {}

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testIcons();

    void urlTestFile();
    void urlTestDirectory();
    void urlTestSubDirectory();

    void trashFileFromHome();
    void trashPercentFileFromHome();
    void trashUtf8FileFromHome();
    void trashUmlautFileFromHome();
    void testTrashNotEmpty();
    void trashFileFromOther();
    void trashFileIntoOtherPartition();
    void trashFileOwnedByRoot();
    void trashSymlinkFromHome();
    void trashSymlinkFromOther();
    void trashBrokenSymlinkFromHome();
    void trashDirectoryFromHome();
    void trashDotDirectory();
    void trashReadOnlyDirFromHome();
    void trashDirectoryFromOther();
    void trashDirectoryOwnedByRoot();

    void tryRenameInsideTrash();

    void statRoot();
    void statFileInRoot();
    void statDirectoryInRoot();
    void statSymlinkInRoot();
    void statFileInDirectory();

    void copyFileFromTrash();
    void copyFileInDirectoryFromTrash();
    void copyDirectoryFromTrash();
    void copySymlinkFromTrash();

    void moveFileFromTrash();
    void moveFileInDirectoryFromTrash();
    void moveDirectoryFromTrash();
    void moveSymlinkFromTrash();

    void listRootDir();
    void listRecursiveRootDir();
    void listSubDir();

    void delRootFile();
    void delFileInDirectory();
    void delDirectory();

    void getFile();
    void restoreFile();
    void restoreFileFromSubDir();
    void restoreFileToDeletedDirectory();

    void emptyTrash();
    void testTrashSize();

protected Q_SLOTS:
    void slotEntries( KIO::Job*, const KIO::UDSEntryList& );

private:
    void trashFile( const QString& origFilePath, const QString& fileId );
    void trashSymlink( const QString& origFilePath, const QString& fileName, bool broken );
    void trashDirectory( const QString& origPath, const QString& fileName );
    void copyFromTrash( const QString& fileId, const QString& destPath, const QString& relativePath = QString() );
    void moveFromTrash( const QString& fileId, const QString& destPath, const QString& relativePath = QString() );

    QString homeTmpDir() const;
    QString otherTmpDir() const;
    QString utf8FileName() const;
    QString umlautFileName() const;
    QString readOnlyDirPath() const;

    QString m_trashDir;

    QString m_otherPartitionTopDir;
    QString m_otherPartitionTrashDir;
    bool m_tmpIsWritablePartition;
    int m_tmpTrashId;
    int m_otherPartitionId;

    int m_entryCount;
    QStringList m_listResult;
};

#endif
