/*  This file is part of the KDE project
    Copyright (C) 2007 David Faure <faure@kde.org>

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

#ifndef TESTKIOARCHIVE_H
#define TESTKIOARCHIVE_H

#include <kio/job.h>
#include <kurl.h>
#include <QObject>

/**
 * This is a unit test for kio_archive
 * See also kdelibs/kio/tests/karchivetest for lower-level KTar and KZip tests.
 */
class TestKioArchive : public QObject
{
    Q_OBJECT

public:
    TestKioArchive() {}

private Q_SLOTS:
    void initTestCase();
    void testListTar();
    void testListRecursive();
    void testExtractFileFromTar();
    void testExtractSymlinkFromTar();
    void testListP7zip();
    void testListP7zipRecursive();
    void testExtractFileFromP7zip();
    // only works with patched p7zip: https://sourceforge.net/tracker/?func=detail&aid=3553730&group_id=111810&atid=660496
    //void testExtractSymlinkFromP7zip();
    void cleanupTestCase();

protected Q_SLOTS: // real slots, not tests
    void slotEntries( KIO::Job*, const KIO::UDSEntryList& lst );

private:
    QString tmpDir() const;
    KUrl tarUrl() const;
    KUrl p7zipUrl() const;
    void copyFromArchive(const KUrl& url, const QString& destPath);

    QStringList m_listResult;
};

#endif
