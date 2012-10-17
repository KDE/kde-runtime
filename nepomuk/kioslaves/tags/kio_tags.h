/*
   Copyright 2012 Vishesh Handa <me@vhanda.in>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _NEPOMUK_KIO_TAGS_H_
#define _NEPOMUK_KIO_TAGS_H_

#include <kio/forwardingslavebase.h>

namespace Nepomuk2 {
    class TagsProtocol : public KIO::SlaveBase
    {

    public:
        TagsProtocol(const QByteArray& pool_socket, const QByteArray& app_socket);
        virtual ~TagsProtocol();

        /**
         * List all files and folders tagged with the corresponding tag.
         */
        void listDir( const KUrl& url );

        /**
         * Results in the creation of a new tag.
         */
        void mkdir( const KUrl &url, int permissions );

        /**
         * Will be forwarded for files.
         */
        void get( const KUrl& url );

        /**
         * Not supported.
         */
        void put( const KUrl& url, int permissions, KIO::JobFlags flags );

        /**
         * Files and folders can be copied to the virtual folders resulting
         * is assignment of the corresponding tag.
         */
        void copy( const KUrl& src, const KUrl& dest, int permissions, KIO::JobFlags flags );

        /**
         * File renaming will be forwarded.
         * Folder renaming results in renaming of the tag.
         */
        void rename( const KUrl& src, const KUrl& dest, KIO::JobFlags flags );

        /**
         * File deletion means remocing the tag
         * Folder deletion will result in deletion of the tag.
         */
        void del( const KUrl& url, bool isfile );

        /**
         * Files will be forwarded.
         * Folders will be created as virtual folders.
         */
        void mimetype( const KUrl& url );

        /**
         * Files will be forwarded.
         * Folders will be created as virtual folders.
         */
        void stat( const KUrl& url );

    private:
        QHash<QUrl, QString> m_tagUriIdentHash;
        QHash<QString, QUrl> m_tagIdentUriHash;

        QUrl fetchUri( const QString& label );
        QString fetchIdentifer( const QUrl& uri );
    };
}

#endif // _NEPOMUK_KIO_TAGS_H_
