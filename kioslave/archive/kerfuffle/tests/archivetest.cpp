/*
 * Copyright (c) 2010 Raphael Kubo da Costa <kubito@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "kerfuffle/archive.h"

#include <qtest_kde.h>

class ArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFileName();
    void testIsPasswordProtected();
    void testOpenNonExistentFile();
};

QTEST_KDEMAIN_CORE(ArchiveTest)

void ArchiveTest::testFileName()
{
    Kerfuffle::Archive *archive = Kerfuffle::factory("/tmp/foo.tar.gz");

    if (!archive) {
        QSKIP("There is no plugin to handle tar.gz files. Skipping test.", SkipSingle);
    }

    QCOMPARE(archive->fileName(), QString("/tmp/foo.tar.gz"));

    archive->deleteLater();
}

void ArchiveTest::testIsPasswordProtected()
{
    Kerfuffle::Archive *archive;

    archive = Kerfuffle::factory(KDESRCDIR "data/archivetest_encrypted.zip");
    if (!archive) {
        QSKIP("There is no plugin to handle zip files. Skipping test.", SkipSingle);
    }

    QVERIFY(archive->isPasswordProtected());

    archive->deleteLater();

    archive = Kerfuffle::factory(KDESRCDIR "data/archivetest_unencrypted.zip");

    QVERIFY(!archive->isPasswordProtected());

    archive->deleteLater();
}

void ArchiveTest::testOpenNonExistentFile()
{
    QSKIP("How should we deal with files that do not exist? Should factory() return NULL?", SkipSingle);
}

#include "archivetest.moc"
