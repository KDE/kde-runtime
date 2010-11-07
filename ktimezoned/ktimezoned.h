/*
   This file is part of the KDE libraries
   Copyright (c) 2007,2009 David Jarvie <djarvie@kde.org>

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

#ifndef KTIMEZONED_H
#define KTIMEZONED_H

#include "ktimezonedbase.h"

#include <QString>
#include <QByteArray>
class QFile;

#include <kdedmodule.h>
#include <kdirwatch.h>
#include <ksystemtimezone.h>


class KTimeZoned : public KTimeZonedBase
{
        Q_OBJECT

    public:
        KTimeZoned(QObject* parent, const QList<QVariant>&);
        ~KTimeZoned();

    private Q_SLOTS:
        void  zonetab_Changed(const QString& path);
        void  localChanged(const QString& path);

    private:
        // How the local time zone is specified
        enum LocalMethod
        {
            TypeMask = 0x30,
            Link = 0x10,    // specified by a symlink
            File = 0x20,    // specified by a normal file

            Utc,            // use UTC default: no local zone spec was found
            EnvTz,          // specified in TZ environment variable
            TzName,         // specified in tzname via tzset()
            Localtime,      // specified in /etc/localtime
            Timezone,       // specified in /etc/timezone
            RcFile,         // specified in /etc/rc.conf or /etc/rc.local
            DefaultInit,    // specified in /etc/default/init
            EnvTzFile = EnvTz | File,
            EnvTzLink = EnvTz | Link,
            LocaltimeCopy = Localtime | File,
            LocaltimeLink = Localtime | Link
        };
        // Type of zone.tab cache
        enum CacheType
        {
            NoCache,        // zone.tab is the real thing, not a cached version
            Solaris         // Solaris: compiled from files in /usr/share/lib/zoneinfo/src
        };
        typedef QMap<QString, QString> MD5Map;    // zone name, checksum

        /** reimp */
        void  init(bool restart);
        bool  findZoneTab(QFile& f);
        void  readZoneTab(QFile& f);
        void  findLocalZone();
        bool  checkTZ(const char *envZone);
        bool  checkLocaltimeLink();
        bool  checkLocaltimeFile();
        bool  checkTimezone();
        bool  checkRcFile();
        bool  checkDefaultInit();
        void  updateLocalZone();
        bool  matchZoneFile(const QString &path);
        bool  findKey(const QString &path, const QString &key);
        bool  setLocalZone(const QString &zoneName);
        KTimeZone compareChecksum(const KTimeZone&, const QString &referenceMd5Sum, qlonglong size);
        bool  compareChecksum(MD5Map::ConstIterator, const QString &referenceMd5Sum, qlonglong size);
        QString calcChecksum(const QString &zoneName, qlonglong size);

        QString     mZoneinfoDir;       // path to zoneinfo directory
        QString     mZoneTab;           // path to zone.tab file
        QByteArray  mSavedTZ;           // last value of TZ if it's used to set local zone
        KSystemTimeZoneSource *mSource;
        KTimeZones  mZones;             // time zones collection
        QString     mLocalIdFile;       // file containing pointer to local time zone definition
        QString     mLocalIdFile2;      // file containing pointer 2 to local time zone definition
        QString     mLocalZoneDataFile; // zoneinfo file containing local time zone definition
        QString     mLocaltimeMd5Sum;   // MD5 checksum of /etc/localtime
        LocalMethod mLocalMethod;       // how the local time zone is specified
        KDirWatch  *mZonetabWatch;      // watch for zone.tab file changes
        KDirWatch  *mDirWatch;          // watch for time zone definition file changes
        MD5Map      mMd5Sums;           // MD5 checksums of zoneinfo files
        CacheType   mZoneTabCache;      // type of cached simulated zone.tab
        bool        mHaveCountryCodes;  // true if zone.tab contains any country codes
};

#endif
