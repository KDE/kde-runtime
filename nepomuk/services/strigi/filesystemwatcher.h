/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _FILE_SYSTEM_WATCHER_H_
#define _FILE_SYSTEM_WATCHER_H_

#include <QtCore/QObject>
#include <QtCore/QDateTime>


/**
 * Watches a file system by periodically checking the
 * folder modification date.
 */
class FileSystemWatcher : public QObject
{
    Q_OBJECT

public:
    FileSystemWatcher( QObject* parent = 0 );
    ~FileSystemWatcher();

    QStringList folders() const;
    bool watchRecursively() const;
    int interval() const;

public Q_SLOTS:
    void setFolders( const QStringList& folders );

    /**
     * Enabled by default.
     */
    void setWatchRecursively( bool );

    void setInterval( int seconds );

    /**
     * Actually start checking the folders. Once the FileSystemWatcher
     * is started it will check the configured folders every \p interval()
     * seconds.
     *
     * Be aware that changing the settings on a started FileSystemWatcher
     * will have no effect until it is restarted.
     */
    void start( const QDateTime& startTime = QDateTime::currentDateTime() );

    void stop();

Q_SIGNALS:
    /**
     * Emitted if a folder is dirty, i.e. its contents
     * changed.
     */
    void dirty( const QString& folder );

private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void checkFolders() )
};

#endif
