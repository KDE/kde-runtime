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

#include "kcliarchive.h"
#include "kerfuffle/archive.h"

#include <unistd.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>
#include <ktemporaryfile.h>

using namespace Kerfuffle;

class TemporaryPath
{
public:
    TemporaryPath(const QString & path): m_path(path)
    {
    }

    ~TemporaryPath()
    {
         kDebug(7109) << "removing" << m_path;
         QFile::remove(m_path);
         QDir().rmpath(m_path);
         int i = m_path.lastIndexOf(QLatin1Char('/'));
         if (i != -1) {
             QDir().mkpath(m_path.left(i));
         }
    }

    QString path()
    {
        return m_path;
    }

private:
    QString m_path;
};

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
    QIODevice * tmpFile;
};

KCliArchive::KCliArchive(const QString & fileName)
    : KArchive(fileName),
      CliInterface(0, QVariantList() << QVariant(QFileInfo(fileName).absoluteFilePath())),
      m_archiveType(ArchiveType7z),
      d(new KCliArchivePrivate)
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
    kDebug(7109);
    if(isOpen()) {
        close();
    }
    delete d;
}

bool KCliArchive::del( const QString & name, bool isFile )
{
    Q_UNUSED(isFile);

    // accept WriteOnly and ReadWrite
    if (!(mode() & QIODevice::WriteOnly)) {
        kWarning(7109) << "You must open the archive for writing";
        return false;
    }

    const QString programPath(KStandardDirs::findExe(m_param.value(DeleteProgram).toString()));
    if (programPath.isEmpty()) {
        kError(7109) << "Failed to locate program" << m_param.value(DeleteProgram).toString() << "in PATH.";
        return false;
    }

    createTmpDir();
    TemporaryPath tmpPath(tmpDir);

    // this is equivalent to "7z d archive.7z dir/file"
    QStringList args;
    args.append(QLatin1String("d"));
    args.append(this->fileName()); // archive where we will delete the file from.
    args.append(name);
    return runProcess2(programPath, args);
}

bool KCliArchive::openArchive(QIODevice::OpenMode mode)
{
    if (mode == QIODevice::WriteOnly) {
        return true;
    }

    // CliInterface is not ready to allow several KProcess running in parallel.
    // We cannot run CliInterface::list() in parallel with other methods of CliInterface,
    // like CliInterface::copyFiles(), which is used to add symbolic links in KCliArchive::addEntry().
    // We will save the symbolic link list and add them after CliInterface::list() finishes.
    symLinksToAdd.clear();
    bool ret = list();
    if (ret) {
        foreach(const Kerfuffle::ArchiveEntry & entry, symLinksToAdd) {
            addEntry(entry);
        }
    }
    symLinksToAdd.clear();
    return ret;
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

    kDebug(7109) << "closed";

    return true;
}

bool KCliArchive::createDevice(QIODevice::OpenMode mode)
{
    kDebug(7109);
    bool b = false;

    if (mode == QIODevice::WriteOnly) {
        // the real device will be set in doPrepareWriting.
        // this is to prevent a Q_ASSERT in KArchive::open.
        if (!d->tmpFile) {
            KTemporaryFile * temp = new KTemporaryFile();
            b = temp->open();
            d->tmpFile = temp;
        }
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

        QString symLinkTarget;

        // we need to patch 7z to print the "IsLink" line for symlinks in 7z archives.
        if (archiveEntry[Link].toBool()) {
            //kDebug(7109) << name << "is a symbolic link";

            // only tar archives provide the symlink's target info during listing.
            symLinkTarget = archiveEntry[LinkTarget].toString();

            // zip archives do not support symbolic links.
            // krar.cpp does not detect symbolic links yet.
            if (m_archiveType == ArchiveType7z && symLinkTarget.isEmpty()) {
                createTmpDir();
                QString filePath = tmpDir + name;
                TemporaryPath tmpPath(filePath);
                QString destDir = filePath;
                int temp = filePath.lastIndexOf(QLatin1Char('/'));
                if (temp != -1) {
                    destDir = filePath.left(temp);
                }
                QDir().mkpath(destDir);
            
                //kDebug(7109) << "extracting symlink to" << filePath;
    
                QList<QVariant> filesToExtract;
                filesToExtract.append(QVariant::fromValue(name));
                copyFiles(filesToExtract, destDir, ExtractionOptions());
                //kDebug(7109) << "reading symlink's target";
    
                QByteArray symlink = filePath.toUtf8();
                char buf[256];
                int bytesRead = readlink(symlink.constData(), buf, 256);
                if (bytesRead > -1) {
                    buf[bytesRead] = '\0';
                    symLinkTarget = QString(buf);
                    kDebug(7109) << "symlink target for" << name << "is" << symLinkTarget;
                } else {
                    kDebug(7109) << "error reading symLinkTarget for" << symlink;
                }
            }
        }
        entry = new KCliArchiveFileEntry(this, entryName, permissions, archiveEntry[Timestamp].toDateTime().toTime_t(),
                                 rootDir()->user(), rootDir()->group(),
                                 symLinkTarget, name, 0 /*dataoffset*/,
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

bool KCliArchive::runProcess2(const QString & programPath, const QStringList & args, const bool waitForFinished)
{
    if (d->process) {
        d->process->waitForFinished();
        delete d->process;
    }

    d->process = new KProcess();
    d->process->setWorkingDirectory(tmpDir); // 7z command creates temporary files in the current working directory.
    d->process->setProgram(programPath, args);
    kDebug(7109) << "starting '" << programPath << args << "'";
    d->process->start();

    if (!d->process->waitForStarted()) {
         return false;
    }
    kDebug(7109) << " started";

    if (!waitForFinished) {
        return true;
    }

    if (!d->process->waitForFinished()) {
         return false;
    }

    if (d->process->exitCode() != 0) {
        kDebug(7109) << "exitCode" << d->process->exitCode();
        return false;
    }

    return true;
}

bool KCliArchive::doWriteDir(const QString &name, const QString &user, const QString &group,
                       mode_t perm, time_t atime, time_t mtime, time_t ctime)
{
    kDebug(7109);

    // accept WriteOnly and ReadWrite
    if (!(mode() & QIODevice::WriteOnly)) {
        kWarning(7109) << "You must open the archive for writing";
        return false;
    }

    const QString programPath(KStandardDirs::findExe(m_param.value(AddProgram).toString()));
    if (programPath.isEmpty()) {
        kError(7109) << "Failed to locate program" << m_param.value(AddProgram).toString() << "in PATH.";
        return false;
    }

    // we need to create the directory to be added in the filesystem before launching the command line process.
    createTmpDir();
    QDir().mkpath(tmpDir + name);
    TemporaryPath tmpPath(tmpDir + name);

    if (!QDir(tmpDir + name).exists()) {
        kDebug(7109) << "error creating temporary directory " << (tmpDir + name);
        return false;
    }

    // this is equivalent to "7z a archive.7z dir/subdir/"
    QStringList args;
    args.append(QLatin1String("a"));
    args.append(this->fileName()); // archive where we will create the new directory.
    args.append(name);
    return runProcess2(programPath, args);
}

bool KCliArchive::doWriteSymLink(const QString &name, const QString &target,
                         const QString &user, const QString &group,
                         mode_t perm, time_t atime, time_t mtime, time_t ctime)
{
    kDebug(7109) << name << target;

    // accept WriteOnly and ReadWrite
    if (!(mode() & QIODevice::WriteOnly)) {
        kWarning(7109) << "You must open the archive for writing";
        return false;
    }

    const QString programPath(KStandardDirs::findExe(m_param.value(AddProgram).toString()));
    if (programPath.isEmpty()) {
        kError(7109) << "Failed to locate program" << m_param.value(AddProgram).toString() << "in PATH.";
        return false;
    }

    // we need to create the symlink to be added in the filesystem before launching the command line process.
    createTmpDir();
    TemporaryPath tmpPath(tmpDir + name);
    QString subdir;
    int i = name.lastIndexOf(QLatin1Char('/'));
    if (i != -1) {
        subdir = name.left(i);
    }
    QDir().mkpath(tmpDir + subdir);

    if (!QDir(tmpDir + subdir).exists()) {
        kDebug(7109) << "error creating temporary directory " << (tmpDir + subdir);
        return false;
    }

    QByteArray t = target.toUtf8();
    QByteArray l = tmpDir.toUtf8() + name.toUtf8();
    if (symlink(t.constData(), l.constData()) < 0) {
        kDebug(7109) << "error creating temporary symlink " << (tmpDir + name);
        return false;
    }

    // this is equivalent to "7z a archive.7z dir/subdir/symlink"
    QStringList args;
    args.append(QLatin1String("a"));
    args.append(this->fileName()); // archive where we will create the symlink.
    args.append(name);
    return runProcess2(programPath, args);
}

bool KCliArchive::doPrepareWriting(const QString & name, const QString & user,
                           const QString & group, qint64 /*size*/, mode_t perm,
                           time_t atime, time_t mtime, time_t ctime)
{
    kDebug(7109);
    if (!isOpen()) {
        kWarning(7109) << "You must open the archive before writing to it";
        return false;
    }

    // accept WriteOnly and ReadWrite
    if (!(mode() & QIODevice::WriteOnly)) {
        kWarning(7109) << "You must open the archive for writing";
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

    // TODO: maybe use KSaveFile to make this process more secure (KSaveFile is used to implement rollback).

    if (m_archiveType == ArchiveType7z) {
        const QString programPath(KStandardDirs::findExe(m_param.value(AddProgram).toString()));
        if (programPath.isEmpty()) {
            kError(7109) << "Failed to locate program" << m_param.value(AddProgram).toString() << "in PATH.";
            return false;
        }

        createTmpDir();

        // this is equivalent to "cat dir/file | 7z -sidir/file a archive.7z",
        // which only works for 7z archive type.
        QStringList args;
        args.append(QLatin1String("-si") + name); // name is the filename relative to the archive.
        args.append(QLatin1String("a")); // TODO: use "u" if file already exists.
        args.append(this->fileName()); // archive we will add the new file to.
        bool ret = runProcess2(programPath, args, false);
    
        if (!ret) {
             TemporaryPath tmpPath(tmpDir);
             return false;
        }
        kDebug(7109) << " started";

        if (!d->tmpFile) {
            delete d->tmpFile;
            d->tmpFile = 0;
        }
        setDevice(d->process);
        return ret;
    }

    if (d->tmpFile) {
        delete d->tmpFile;
    }

    createTmpDir();
    QString tmpFilePath = tmpDir + name;
    QDir().mkpath(tmpFilePath.left(tmpFilePath.lastIndexOf(QLatin1Char('/'))));
    d->tmpFile = new QFile(tmpFilePath);

    kDebug(7109) << "Opening" << tmpFilePath << "for writing";
    if (!d->tmpFile->open(QIODevice::WriteOnly)) {
        TemporaryPath tmpPath(tmpFilePath);
        return false;
    }

    setDevice(d->tmpFile);

    return true;
}

bool KCliArchive::doFinishWriting(qint64 size)
{
    kDebug(7109);
    Q_ASSERT(d->currentFile);
    d->currentFile->setSize(size);
    TemporaryPath tmpPath(tmpDir + d->currentFile->path());

    if (m_archiveType != ArchiveType7z) {
        d->tmpFile->close();
        CompressionOptions options;
        options[QLatin1String("GlobalWorkDir")] = tmpDir;

        bool ret = addFiles(QStringList() << d->currentFile->path(), options);


        return ret;
    }

    Q_ASSERT(d->process);

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

    kDebug(7109) << "finished";

    return ret;
}

void KCliArchive::virtual_hook(int id, void * data)
{
    kDebug(7109);
    KArchive::virtual_hook(id, data);
}

QString KCliArchive::createTmpDir()
{
    QString temp = KGlobal::dirs()->findDirs("tmp", "")[0];
    if (temp.isEmpty()) {
        temp = QLatin1String("/tmp/");
    }
    temp += QString("KCliArchive%1").arg(QCoreApplication::applicationPid());

    int i = 0;
    tmpDir = temp;
    QDir dir(tmpDir);
    dir.rmpath(tmpDir);
    while (QDir(tmpDir).exists()) {
        tmpDir = temp + QString("_%1").arg(++i);
        dir.rmpath(tmpDir);
    }
    tmpDir += QLatin1Char('/');
    dir.mkpath(tmpDir);

    return tmpDir;
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
 : QObject(0),
   KArchiveFile(cliArchive, name, permissions, date, user, group, symlink, start, uncompressedSize),
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

        QString filePath = d->q->tmpDir + path();
        QFile::remove(filePath);
        QDir().rmpath(filePath.left(filePath.lastIndexOf(QLatin1Char('/'))));
    }

    return arr;
}

QIODevice* KCliArchiveFileEntry::createDevice() const
{
    d->q->createTmpDir();
    QString filePath = d->q->tmpDir + path();
    QString destDir = filePath;
    int temp = filePath.lastIndexOf(QLatin1Char('/'));
    if (temp != -1) {
        destDir = filePath.left(temp);
    }
    QDir().mkpath(destDir);

    kDebug(7109) << "temporary file is about to be saved in" << filePath;

    QList<QVariant> filesToExtract;
    filesToExtract.append(QVariant::fromValue(path()));

    d->q->copyFiles(filesToExtract, destDir, ExtractionOptions());

    // according to the documentation the caller of this method is responsible
    // for deleting the QIODevice returned here.

    QFile * file = new QFile(filePath);
    file->setProperty("filePath", QVariant::fromValue(filePath));
    connect(file, SIGNAL(destroyed(QObject*)), SLOT(_k_destroyed(QObject*)));
    return file;
}

void KCliArchiveFileEntry::_k_destroyed(QObject * object)
{
    QString filePath = object->property("filePath").toString();
    kDebug(7109) << "removing" << filePath;
 
    QFile::remove(filePath);
    QDir().rmpath(filePath.left(filePath.lastIndexOf(QLatin1Char('/'))));
}
