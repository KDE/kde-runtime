/*
 * kcliarchive.cpp
 *
 * Copyright (C) 2012 basysKom GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "kp7zip.h"
#include "kerfuffle/archive.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>
#include <ktemporaryfile.h>

using namespace Kerfuffle;

class KCliArchive::KCliArchivePrivate
{
public:
    KCliArchivePrivate()
        : currentFile(0),
          process(0),
          tmpFile(0)
    {}

    QList<KCliArchiveFileEntry *> fileList;
    KCliArchiveFileEntry * currentFile; // file currently being added to this archive
    KProcess * process; // used to pipe data to the 7z command when adding a file to the archive.
    KTemporaryFile * tmpFile; // just to prevent a Q_ASSERT in KArchive::open.
};

KCliArchive::KCliArchive(const QString & fileName)
    : KArchive(fileName), CliInterface(0, QVariantList() << QVariant(QFileInfo(fileName).absoluteFilePath())), d(new KCliArchivePrivate)
{
}

KCliArchive::KCliArchive(QIODevice * dev)
    : KArchive(dev), CliInterface(0, QVariantList()), d(new KCliArchivePrivate)
{
    // TODO: check if it is possible to implement QIODevice support.
    kDebug(7109) << "QIODevice is not supported";
}

KCliArchive::~KCliArchive()
{
    kDebug();
    if(isOpen()) {
        close();
    }
    delete d;
}

bool KCliArchive::writeData(const char * data, qint64 size)
{
    kDebug(7109);
    Q_ASSERT(d->process);

    if (d->process->write(data, size) != size) {
        return false;
    }

    kDebug(7109) << "data written";

    return true;
}

bool KCliArchive::del( const QString & name, bool isFile )
{
    Q_UNUSED(isFile);

    const QString programPath(KStandardDirs::findExe(m_param.value(DeleteProgram).toString()));
    if (programPath.isEmpty()) {
        kError(7109) << "Failed to locate program" << m_param.value(DeleteProgram).toString() << "in PATH.";
        return false;
    }

    // this is equivalent to "7z d archive.7z dir/file"
    d->process = new KProcess();
    QStringList args;
    args.append(QLatin1String("d"));
    args.append(this->fileName()); // archive where we will delete the file from.
    args.append(name);
    d->process->setProgram(programPath, args);
    kDebug(7109) << "starting '" << programPath << args << "'";
    d->process->start();

    if (!d->process->waitForStarted()) {
         return false;
    }
    kDebug(7109) << " started";

    if (!d->process->waitForFinished()) {
         return false;
    }

    if (d->process->exitCode() != 0) {
        kDebug(7109) << "exitCode" << d->process->exitCode();
        return false;
    }

    return true;
}

bool KCliArchive::openArchive(QIODevice::OpenMode mode)
{
    if (mode == QIODevice::WriteOnly) {
        return true;
    }

    return list();
}

bool KCliArchive::closeArchive()
{
    if (mode() & QIODevice::WriteOnly) {
        if (d->tmpFile) {
            delete d->tmpFile;
            d->tmpFile = 0L;
        }
        setDevice(0);
    }

    kDebug() << "closed";

    return true;
}

bool KCliArchive::createDevice(QIODevice::OpenMode mode)
{
    kDebug();
    bool b = false;

    if (mode == QIODevice::WriteOnly) {
        // the real device will be set in doPrepareWriting.
        // this is to prevent a Q_ASSERT in KArchive::open.
        if (!d->tmpFile) {
            d->tmpFile = new KTemporaryFile();
        }
        b = d->tmpFile->open();
        setDevice(d->tmpFile);
    } else {
        b = KArchive::createDevice(mode);
    }
    return b;
}

void KCliArchive::addEntry(const Kerfuffle::ArchiveEntry & archiveEntry)
{
    KArchiveEntry * entry;
    int permissions = 0100644;
    QString name = archiveEntry[FileName].toString();
    QString entryName;

    //kDebug(7109) << "adding" << name << archiveEntry[IsDirectory].toBool();

    // TODO: test if we really need the '/' at the end of all directory paths.
    if (archiveEntry[IsDirectory].toBool()) {
        name = name.remove(name.length()-1, 1);
    }

    int pos = name.lastIndexOf(QLatin1Char('/'));
    if (pos == -1) {
        entryName = name;
    } else {
        entryName = name.mid(pos + 1);
    }
    Q_ASSERT(!entryName.isEmpty());

    if (archiveEntry[IsDirectory].toBool()) {
        QString path = QDir::cleanPath( name );
        const KArchiveEntry * ent = rootDir()->entry(path);
        permissions = S_IFDIR | 0755;

        if (ent && ent->isDirectory()) {
            //kDebug(7109) << "directory" << name << "already exists, NOT going to add it again";
            entry = 0;
        } else {
            //kDebug(7109) << "creating KArchiveDirectory, entryName= " << entryName << ", name=" << name;
            entry = new KArchiveDirectory(this, entryName, permissions, archiveEntry[Timestamp].toDateTime().toTime_t(), rootDir()->user(), rootDir()->group(), QString());
        }
    } else {
        //kDebug(7109) << "creating KCliArchiveFileEntry, entryName= " << entryName << ", name=" << name;
        entry = new KCliArchiveFileEntry(this, entryName, permissions, archiveEntry[Timestamp].toDateTime().toTime_t(),
                                 rootDir()->user(), rootDir()->group(),
                                 QString() /*symlink*/, name, 0 /*dataoffset*/,
                                 archiveEntry[Size].toInt(), 0 /*cmethod*/, archiveEntry[CompressedSize].toInt());
    }

    if (entry) {
        if (pos == -1) {
            rootDir()->addEntry(entry);
        } else {
            // In some tar files we can find dir/./file => call cleanPath
            QString path = QDir::cleanPath(name.left(pos));
            // Ensure container directory exists, create otherwise
            KArchiveDirectory * tdir = findOrCreate(path);
            tdir->addEntry(entry);
        }
    }
}

bool KCliArchive::doWriteDir(const QString &name, const QString &user, const QString &group,
                       mode_t perm, time_t atime, time_t mtime, time_t ctime)
{
    kDebug(7109);

    if (d->process) {
        d->process->waitForFinished();
        delete d->process;
    }

    // we need to create the directory to be added in the filesystem before running the 7z command.
    QString tmpDir = KGlobal::dirs()->findDirs("tmp", "")[0];
    if (tmpDir.isEmpty()) {
        tmpDir = QLatin1String("/tmp/");
    }
    tmpDir += QString("KCliArchive%1").arg(QCoreApplication::applicationPid());

    int i = 0;
    QString temp = tmpDir;
    while (QDir(temp).exists()) {
        temp = tmpDir + QString("_%1").arg(++i);
    }
    tmpDir = temp + QLatin1Char('/');
    QDir().mkpath(tmpDir + name);

    if (!QDir(tmpDir + name).exists()) {
        kDebug(7109) << "error creating temporary directory " << (tmpDir + name);
        return false;
    }

    const QString programPath(KStandardDirs::findExe(m_param.value(AddProgram).toString()));
    if (programPath.isEmpty()) {
        kError(7109) << "Failed to locate program" << m_param.value(AddProgram).toString() << "in PATH.";
        return false;
    }

    // this is equivalent to "7z a archive.7z dir/subdir/"
    d->process = new KProcess();
    d->process->setWorkingDirectory(tmpDir);
    QStringList args;
    args.append(QLatin1String("a"));
    args.append(this->fileName()); // archive where we will create the new directory.
    args.append(name);
    d->process->setProgram(programPath, args);
    kDebug(7109) << "starting '" << programPath << args << "'";
    d->process->start();

    if (!d->process->waitForStarted()) {
         return false;
    }
    kDebug(7109) << " started";

    if (!d->process->waitForFinished()) {
         return false;
    }

    bool ret = true;
    if (d->process->exitCode() != 0) {
        kDebug(7109) << "exitCode" << d->process->exitCode();
        ret = false;
    }

    QDir().rmpath(tmpDir + name);

    //setDevice(d->process);

    return ret;
}

bool KCliArchive::doWriteSymLink(const QString &name, const QString &target,
                         const QString &user, const QString &group,
                         mode_t perm, time_t atime, time_t mtime, time_t ctime)
{
    kDebug(7109);

    // TODO: maybe copy name to d->currentFile.

    return true;
}

bool KCliArchive::doPrepareWriting(const QString & name, const QString & user,
                           const QString & group, qint64 /*size*/, mode_t perm,
                           time_t atime, time_t mtime, time_t ctime)
{
    kDebug(7109);
    if (!isOpen()) {
        kWarning(7109) << "You must open the 7z file before writing to it";
        return false;
    }

    // accept WriteOnly and ReadWrite
    if (!(mode() & QIODevice::WriteOnly)) {
        kWarning(7109) << "You must open the 7z file for writing";
        return false;
    }

    // delete entries in the filelist with the same fileName as the one we want
    // to save, so that we don't have duplicate file entries when viewing the zip
    // with konqi...
    // CAUTION: the old file itself is still in the zip and won't be removed !!!
    QMutableListIterator<KCliArchiveFileEntry *> it(d->fileList);
    kDebug(7109) << "fileName to write: " << name;
    
    while(it.hasNext()) {
        it.next();

        kDebug(7109) << "prepfileName: " << it.value()->path();
        if (name == it.value()->path()) {
            kDebug(7109) << "removing following entry: " << it.value()->path();
            delete it.value();
            it.remove();
        }
    }

    // Find or create parent dir
    KArchiveDirectory * parentDir = rootDir();
    QString fileName(name);
    int i = name.lastIndexOf(QLatin1Char('/'));
    if (i != -1) {
        QString dir = name.left(i);
        fileName = name.mid(i + 1);
        kDebug(7109) << "ensuring" << dir << "exists. fileName=" << fileName;
        parentDir = findOrCreate(dir);
    }

    // construct a KCliArchiveFileEntry and add it to list
    KCliArchiveFileEntry * e = new KCliArchiveFileEntry(this, fileName, perm, mtime, user, group, QString(),
                                        name, 0 /*dataoffset*/,
                                        0 /*size unknown yet*/, 0 /*cmethod*/, 0 /*csize unknown yet*/);
    
    parentDir->addEntry(e);
    d->fileList.append(e);
    d->currentFile = e;

    if (d->process) {
        d->process->waitForFinished();
        delete d->process;
    }

    // TODO: maybe use KSaveFile to make this process more secure (KSaveFile is used to implement rollback).

    const QString programPath(KStandardDirs::findExe(m_param.value(AddProgram).toString()));
    if (programPath.isEmpty()) {
        kError(7109) << "Failed to locate program" << m_param.value(AddProgram).toString() << "in PATH.";
        return false;
    }

    // this is equivalent to "cat dir/file | 7z -sidir/file a archive.7z"
    d->process = new KProcess();
    QStringList args;
    args.append(QLatin1String("-si") + name); // name is the filename relative to the archive.
    args.append(QLatin1String("a")); // TODO: use "u" if file already exists.
    args.append(this->fileName()); // archive we will add the new file to.
    d->process->setProgram(programPath, args);
    kDebug(7109) << "starting '" << programPath << args << "'";
    d->process->start();

    if (!d->process->waitForStarted()) {
         return false;
    }
    kDebug(7109) << " started";
    setDevice(d->process);

    return true;
}

bool KCliArchive::doFinishWriting(qint64 size)
{
    kDebug(7109);

    Q_ASSERT(d->currentFile);
    Q_ASSERT(d->process);
    d->currentFile->setSize(size);

    // TODO: calcule the compressed size.
    //d->currentFile->setCompressedSize(csize);

    d->process->closeWriteChannel();

    if (!d->process->waitForFinished()) {
         return false;
    }

    bool ret = true;
    if (d->process->exitCode() != 0) {
        kDebug(7109) << "exitCode" << d->process->exitCode();
        ret = false;
    }

    d->process->deleteLater();
    d->process = 0L;
    d->currentFile = 0L;

    kDebug() << "finished";

    return ret;
}

void KCliArchive::virtual_hook(int id, void * data)
{
    kDebug(7109);
    KArchive::virtual_hook(id, data);
}

/***************************/
class KCliArchiveFileEntry::KCliArchiveFileEntryPrivate
{
public:
    KCliArchiveFileEntryPrivate()
    : crc(0),
      compressedSize(0),
      headerStart(0),
      encoding(0)
    {}
    unsigned long crc;
    qint64        compressedSize;
    qint64        headerStart;
    int           encoding;
    QString       path;

    KCliArchive * q;
};

KCliArchiveFileEntry::KCliArchiveFileEntry(KCliArchive * cliArchive, const QString & name, int permissions, int date,
                           const QString & user, const QString & group, const QString & symlink,
                           const QString & path, qint64 start, qint64 uncompressedSize,
                           int encoding, qint64 compressedSize)
 : KArchiveFile(cliArchive, name, permissions, date, user, group, symlink, start, uncompressedSize),
   d(new KCliArchiveFileEntryPrivate)
{
    d->path = path;
    d->encoding = encoding;
    d->compressedSize = compressedSize;
    d->q = cliArchive;
}

KCliArchiveFileEntry::~KCliArchiveFileEntry()
{
    delete d;
}

const QString &KCliArchiveFileEntry::path() const
{
    return d->path;
}

QByteArray KCliArchiveFileEntry::data() const
{
    QIODevice * dev = createDevice();
    QByteArray arr;

    if (dev) {
        arr = dev->readAll();
        delete dev;
    }

    return arr;
}

QIODevice* KCliArchiveFileEntry::createDevice() const
{
    QList<QVariant> filesToExtract;
    filesToExtract.append(QVariant::fromValue(path()));

    QString tmpDir = KGlobal::dirs()->findDirs("tmp", "")[0];
    if (tmpDir.isEmpty()) {
        tmpDir = QLatin1String("/tmp/");
    }

    QString filePath = tmpDir + path();
    QString destDir = filePath;
    int temp = filePath.lastIndexOf(QLatin1Char('/'));
    if (temp != -1) {
        destDir = filePath.left(temp);
    }
    QDir().mkpath(destDir);

    kDebug(7109) << "temporary file is about to be saved in" << destDir;

    // 7z command stalls asking what to do when file already exists, so make
    // sure it does not.
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
    }

    d->q->copyFiles(filesToExtract, destDir, ExtractionOptions());

    // according to the documentation the caller of this method is responsible
    // for deleting the QIODevice returned here.
    return new QFile(filePath);
}
