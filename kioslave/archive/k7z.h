/*
 * k7z.h
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

#ifndef K7Z_H
#define K7Z_H

#include "kerfuffle/cliinterface.h"

#include <karchive.h>

class K7z: public KArchive
{
public:
    K7z(const QString& filename);

    /**
     * Not supported yet, do not use it.
     */
    K7z(QIODevice * dev);

    virtual ~K7z();

    /**
     * Write data to a file that has been created using prepareWriting().
     * @param data a pointer to the data
     * @param size the size of the chunk
     * @return true if successful, false otherwise
     */
    virtual bool writeData(const char * data, qint64 size);

    /**
     * Delete a file or directory from the archive.
     * @param path the relative path of the file or the directory in the archive.
     * @param isFile true if the path is a file, false if it is a directory.
     * @return true if successfull, false otherwise.
     */
    bool del( const QString & name, bool isFile );

protected:

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

private:
    class K7zPrivate;
    K7zPrivate * const d;
    friend class Cli7zPlugin;
    friend class K7zFileEntry;

    void addEntry(const Kerfuffle::ArchiveEntry & archiveEntry);
    void copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, Kerfuffle::ExtractionOptions options);
};

/**
 * A K7zFileEntry represents an file in a zip archive.
 */
class K7zFileEntry : public KArchiveFile
{
public:
    /**
     * Creates a new zip file entry. Do not call this, K7z takes care of it.
     */
    K7zFileEntry(K7z * zip, const QString & name, int permissions, int date,
                 const QString & user, const QString & group, const QString & symlink,
                 const QString & path, qint64 start, qint64 uncompressedSize,
                 int encoding, qint64 compressedSize);

    /**
     * Destructor. Do not call this.
     */
    ~K7zFileEntry();

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

private:
    class K7zFileEntryPrivate;
    K7zFileEntryPrivate * const d;
};

#endif
