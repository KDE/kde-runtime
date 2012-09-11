/*  This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

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

#ifndef KIO_ARCHIVE_H
#define KIO_ARCHIVE_H

#include <sys/types.h>

#include <kio/global.h>
#include <kio/slavebase.h>

class KArchive;
class KArchiveDirectory;
class KArchiveEntry;

class ArchiveProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
    ArchiveProtocol( const QByteArray &pool, const QByteArray &app );
    virtual ~ArchiveProtocol();

    // browsing operations
    virtual void listDir( const KUrl & url );
    virtual void stat( const KUrl & url );

    // directory operations
    //virtual void copy( const KUrl& src, const KUrl &dest, int permissions, KIO::JobFlags flags );
    virtual void del( const KUrl& kurl, bool isFile);
    virtual void mkdir( const KUrl& kurl, int permissions );
    //virtual void rename( const KUrl& src, const KUrl& dest, KIO::JobFlags flags );

    // file operations
    virtual void get( const KUrl& kurl );
    virtual void put( const KUrl& kurl, int permissions, KIO::JobFlags flags );
    virtual void close();

private:
    void createRootUDSEntry( KIO::UDSEntry & entry );
    void createUDSEntry( const KArchiveEntry * tarEntry, KIO::UDSEntry & entry );
    QString relativePath( const QString & fullPath );

    /**
     * \brief find, check and open the archive file
     * \param url The URL of the archive
     * \param path Path where the archive really is (returned value)
     * \param errNum KIO error number (undefined if the function returns true)
     * \return true if file was found, false if there was an error
     */
    bool checkNewFile( const KUrl & url, QString & path, KIO::Error& errorNum );

    KArchive * m_archiveFile;
    QString m_archiveName;
    QString m_user, m_group;
    time_t m_mtime;
};

#endif // KIO_ARCHIVE_H
