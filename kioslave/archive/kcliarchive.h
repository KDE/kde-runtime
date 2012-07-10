/*
 * kcliarchive.h
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

#ifndef KCLIARCHIVE_H
#define KCLIARCHIVE_H

#include "kerfuffle/cliinterface.h"

#include <karchive.h>

class KCliArchive: public KArchive, public Kerfuffle::CliInterface
{
public:
    KCliArchive(const QString& filename);

    /**
     * Not supported yet, do not use it.
     */
    KCliArchive(QIODevice * dev);

    virtual ~KCliArchive();

    /**
     * Delete a file or directory from the archive.
     * @param path the relative path of the file or the directory in the archive.
     * @param isFile true if the path is a file, false if it is a directory.
     * @return true if successfull, false otherwise.
     */
    bool del( const QString & name, bool isFile );

protected:
    enum ArchiveType {
	ArchiveType7z = 0,
	ArchiveTypeBZip2,
	ArchiveTypeGZip,
	ArchiveTypeTar,
	ArchiveTypeZip,
	ArchiveTypeRar
    };

    ArchiveType m_archiveType;

    /**
     * Opens the archive for reading.
     * Parses the directory listing of the archive
     * and creates the KArchiveDirectory/KArchiveFile entries.
     * @param mode the mode of the file
     */
    virtual bool openArchive(QIODevice::OpenMode mode);

    /**
     * Closes the archive
     */
    virtual bool closeArchive();

    /**
     * create the QIODevice to read from & write to the archive.
     */
    virtual bool createDevice(QIODevice::OpenMode mode);

    // Reimplemented from KArchive
    virtual bool doWriteDir(const QString & name, const QString & user,
                            const QString & group, mode_t perm, time_t atime,
                            time_t mtime, time_t ctime);

    // Reimplemented from KArchive
    virtual bool doWriteSymLink(const QString & name, const QString & target,
                                const QString & user, const QString & group,
                                mode_t perm, time_t atime, time_t mtime, time_t ctime);

    // Reimplemented from KArchive
    virtual bool doPrepareWriting(const QString & name, const QString & user,
                                  const QString & group, qint64 size, mode_t perm,
                                  time_t atime, time_t mtime, time_t ctime);

    /**
     * Write data to a file that has been created using prepareWriting().
     * @param size the size of the file
     * @return true if successful, false otherwise
     */
    virtual bool doFinishWriting(qint64 size);

    virtual void virtual_hook(int id, void * data);

    void addEntry(const Kerfuffle::ArchiveEntry & archiveEntry);

    QList<Kerfuffle::ArchiveEntry> symLinksToAdd;
    QString tmpDir;
    QString createTmpDir();
    friend class KCliArchiveFileEntry;

private:
    class KCliArchivePrivate;
    KCliArchivePrivate * const d;
};

/**
 * A KCliArchiveFileEntry represents an file in a archive.
 */
class KCliArchiveFileEntry : public QObject, public KArchiveFile
{
    Q_OBJECT

public:
    /**
     * Creates a new file entry. Do not call this, KCliArchive takes care of it.
     */
    KCliArchiveFileEntry(KCliArchive * cliArchive, const QString & name, int permissions, int date,
                 const QString & user, const QString & group, const QString & symlink,
                 const QString & path, qint64 start, qint64 uncompressedSize,
                 int encoding, qint64 compressedSize);

    /**
     * Destructor. Do not call this.
     */
    ~KCliArchiveFileEntry();

    /// Name with complete path - KArchiveFile::name() is the filename only (no path)
    const QString & path() const;

    /**
     * @return the content of this file.
     * Call data() with care (only once per file), this data isn't cached.
     */
    virtual QByteArray data() const;

    /**
     * This method returns a QIODevice to read the file contents.
     * This is obviously for reading only.
     * Note that the ownership of the device is being transferred to the caller,
     * who will have to delete it.
     * The returned device auto-opens (in readonly mode), no need to open it.
     */
    virtual QIODevice * createDevice() const;

private Q_SLOTS:
    void _k_destroyed(QObject * object);

private:
    class KCliArchiveFileEntryPrivate;
    KCliArchiveFileEntryPrivate * const d;
};

#endif
