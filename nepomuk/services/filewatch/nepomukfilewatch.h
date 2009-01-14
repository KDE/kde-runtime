/* This file is part of the KDE Project
   Copyright (c) 2007-2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _NEPOMUK_FILE_WATCH_H_
#define _NEPOMUK_FILE_WATCH_H_

#include <Nepomuk/Service>

#include <QtCore/QUrl>
#include <QtCore/QVariant>

namespace Soprano {
    class Model;
    namespace Client {
        class DBusClient;
    }
}

class KUrl;

namespace Nepomuk {
    class FileWatch : public Service
    {
        Q_OBJECT

    public:
        FileWatch( QObject* parent, const QVariantList& );
        ~FileWatch();

    Q_SIGNALS:
        // helper signals for async dbus connections
        void fileMoved( const QString& from, const QString& to );
        void filesDeleted( const QStringList& path );

    private Q_SLOTS:
        void slotFileMoved( const QString& from, const QString& to );
        void slotFileDeleted( const QString& path );
        void slotFilesDeleted( const QStringList& path );

    private:
        void removeMetaData( const KUrl& );
        void updateMetaData( const KUrl& from, const KUrl& to );

        QUrl m_strigiParentUrlUri;
    };
}

#endif
