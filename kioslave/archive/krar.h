/*
 * krar.h
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

#ifndef KRAR_H
#define KRAR_H

#include "kcliarchive.h"

class KRar: public KCliArchive
{
    Q_OBJECT

public:
    KRar(const QString& filename);

    /**
     * Not supported yet, do not use it.
     */
    KRar(QIODevice * dev);

    virtual ~KRar();

protected:
    virtual Kerfuffle::ParameterList parameterList() const;
    virtual bool readListLine(const QString &line);

private:
    enum ReadState {
	ReadStateHeader = 0,
	ReadStateArchiveInformation,
	ReadStateEntryInformation
    };

    enum {
        ParseStateColumnDescription1 = 0,
        ParseStateColumnDescription2,
        ParseStateHeader,
        ParseStateEntryFileName,
        ParseStateEntryDetails,
        ParseStateEntryIgnoredDetails
    } m_parseState;

    QString m_entryFileName;

    bool m_isPasswordProtected;

    int m_remainingIgnoredSubHeaderLines;

    bool m_isUnrarFree;
};

/**
 * A KRarFileEntry represents an file in a zip archive.
 */
class KRarFileEntry : public KArchiveFile
{
public:
    /**
     * Creates a new zip file entry. Do not call this, KRar takes care of it.
     */
    KRarFileEntry(KRar * zip, const QString & name, int permissions, int date,
                 const QString & user, const QString & group, const QString & symlink,
                 const QString & path, qint64 start, qint64 uncompressedSize,
                 int encoding, qint64 compressedSize);

    /**
     * Destructor. Do not call this.
     */
    ~KRarFileEntry();

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
    class KRarFileEntryPrivate;
    KRarFileEntryPrivate * const d;
};

#endif
