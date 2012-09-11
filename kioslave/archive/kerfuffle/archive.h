/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "kerfuffle_export.h"

#include <QString>
#include <QStringList>
#include <QHash>

class KJob;

namespace Kerfuffle
{
class ListJob;
class ExtractJob;
class DeleteJob;
class AddJob;

/**
 * Meta data related to one entry in a compressed archive.
 *
 * When creating a plugin, information about every single entry in
 * an archive is contained in an ArchiveEntry, and metadata
 * is set with the entries in this enum.
 *
 * Please notice that not all archive formats support all the properties
 * below, so set those that are available.
 */
enum EntryMetaDataType {
    FileName = 0,        /**< The entry's file name */
    InternalID,          /**< The entry's ID for Ark's internal manipulation */
    Permissions,         /**< The entry's permissions */
    Owner,               /**< The user the entry belongs to */
    Group,               /**< The user group the entry belongs to */
    Size,                /**< The entry's original size */
    CompressedSize,      /**< The compressed size for the entry */
    Link,                /**< The entry is a symbolic link */
    LinkTarget,          /**< Target of the symbolic link (only for tar archives) */
    Ratio,               /**< The compression ratio for the entry */
    CRC,                 /**< The entry's CRC */
    Method,              /**< The compression method used on the entry */
    Version,             /**< The archiver version needed to extract the entry */
    Timestamp,           /**< The timestamp for the current entry */
    IsDirectory,         /**< The entry is a directory */
    Comment,
    IsPasswordProtected, /**< The entry is password-protected */
    Custom = 1048576
};

typedef QHash<int, QVariant> ArchiveEntry;

/**
These are the extra options for doing the compression. Naming convention
is CamelCase with either Global, or the compression type (such as Zip,
Rar, etc), followed by the property name used
 */
typedef QHash<QString, QVariant> CompressionOptions;
typedef QHash<QString, QVariant> ExtractionOptions;

class KERFUFFLE_EXPORT Archive : public QObject
{
    Q_OBJECT

public:
    virtual ~Archive() {}

    virtual QString fileName() const = 0;
    virtual bool isReadOnly()  const = 0;

    virtual KJob*       open() = 0;
    virtual KJob*       create() = 0;
    virtual ListJob*    list() = 0;
    virtual DeleteJob*  deleteFiles(const QList<QVariant> & files) = 0;

    /**
     * Compression options that should be handled by all interfaces:
     *
     * GlobalWorkDir - Change to this dir before adding the new files.
     * The path names should then be added relative to this directory.
     *
     * TODO: find a way to actually add files to specific locations in
     * the archive
     * (not supported yet) GlobalPathInArchive - a path relative to the
     * archive root where the files will be added under
     *
     */
    virtual AddJob*     addFiles(const QStringList & files, const CompressionOptions& options = CompressionOptions()) = 0;

    virtual ExtractJob* copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options = ExtractionOptions()) = 0;

    virtual bool isSingleFolderArchive() = 0;
    virtual QString subfolderName() = 0;
    virtual bool isPasswordProtected() = 0;

    virtual void setPassword(QString password) = 0;

};

KERFUFFLE_EXPORT Archive* factory(const QString & filename, const QString & fixedMimeType = QString());
KERFUFFLE_EXPORT QStringList supportedMimeTypes();
KERFUFFLE_EXPORT QStringList supportedWriteMimeTypes();
} // namespace Kerfuffle


#endif // ARCHIVE_H
