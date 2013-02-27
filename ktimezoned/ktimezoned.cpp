/*
   This file is part of the KDE libraries
   Copyright (c) 2005-2010 David Jarvie <djarvie@kde.org>
   Copyright (c) 2005 S.R.Haque <srhaque@iee.org>.

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

#include "ktimezoned.moc"
#include "ktimezonedbase.moc"

#include <climits>
#include <cstdlib>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>
#include <QtDBus/QtDBus>

#include <kglobal.h>
#include <klocale.h>
#include <kcodecs.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>
#include <ktemporaryfile.h>
#include <kdebug.h>
#include <kconfiggroup.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>

K_PLUGIN_FACTORY(KTimeZonedFactory,
                 registerPlugin<KTimeZoned>();
    )
K_EXPORT_PLUGIN(KTimeZonedFactory("ktimezoned"))

// The maximum allowed length for reading a zone.tab line. This is set to
// provide plenty of leeway, given that the maximum length of lines in a valid
// zone.tab will be around 100 - 120 characters.
const int MAX_ZONE_TAB_LINE_LENGTH = 2000;

// Config file entry names
const char ZONEINFO_DIR[]   = "ZoneinfoDir";   // path to zoneinfo/ directory
const char ZONE_TAB[]       = "Zonetab";       // path & name of zone.tab
const char ZONE_TAB_CACHE[] = "ZonetabCache";  // type of cached simulated zone.tab
const char LOCAL_ZONE[]     = "LocalZone";     // name of local time zone


KTimeZoned::KTimeZoned(QObject* parent, const QList<QVariant>& l)
  : KTimeZonedBase(parent, l),
    mSource(0),
    mZonetabWatch(0),
    mDirWatch(0)
{
    init(false);
}

KTimeZoned::~KTimeZoned()
{
    delete mSource;
    mSource = 0;
    delete mZonetabWatch;
    mZonetabWatch = 0;
    delete mDirWatch;
    mDirWatch = 0;
}

void KTimeZoned::init(bool restart)
{
    if (restart)
    {
        kDebug(1221) << "KTimeZoned::init(restart)";
        delete mSource;
        mSource = 0;
        delete mZonetabWatch;
        mZonetabWatch = 0;
        delete mDirWatch;
        mDirWatch = 0;
    }

    KConfig config(QLatin1String("ktimezonedrc"));
    if (restart)
        config.reparseConfiguration();
    KConfigGroup group(&config, "TimeZones");
    mZoneinfoDir     = group.readEntry(ZONEINFO_DIR);
    mZoneTab         = group.readEntry(ZONE_TAB);
    mConfigLocalZone = group.readEntry(LOCAL_ZONE);
    QString ztc      = group.readEntry(ZONE_TAB_CACHE, QString());
    mZoneTabCache    = (ztc == "Solaris") ? Solaris : NoCache;
    if (mZoneinfoDir.length() > 1 && mZoneinfoDir.endsWith('/'))
        mZoneinfoDir.truncate(mZoneinfoDir.length() - 1);   // strip trailing '/'

    // For Unix, read zone.tab.

    QString oldZoneinfoDir = mZoneinfoDir;
    QString oldZoneTab     = mZoneTab;
    CacheType oldCacheType = mZoneTabCache;

    // Open zone.tab if we already know where it is
    QFile f;
    if (!mZoneTab.isEmpty() && !mZoneinfoDir.isEmpty())
    {
        f.setFileName(mZoneTab);
        if (!f.open(QIODevice::ReadOnly))
            mZoneTab.clear();
        else if (mZoneTabCache != NoCache)
        {
            // Check whether the cached zone.tab is still up to date
#ifdef __GNUC__
#warning Implement checking whether Solaris cached zone.tab is up to date
#endif
        }
    }

    if (mZoneTab.isEmpty() || mZoneinfoDir.isEmpty())
    {
        // Search for zone.tab
        if (!findZoneTab(f))
            return;
        mZoneTab = f.fileName();

        if (mZoneinfoDir != oldZoneinfoDir
        ||  mZoneTab != oldZoneTab
        ||  mZoneTabCache != oldCacheType)
        {
            // Update config file and notify interested applications
            group.writeEntry(ZONEINFO_DIR, mZoneinfoDir);
            group.writeEntry(ZONE_TAB, mZoneTab);
            QString ztc;
            switch (mZoneTabCache)
            {
                case Solaris:  ztc = "Solaris";  break;
                default:  break;
            }
            group.writeEntry(ZONE_TAB_CACHE, ztc);
            group.sync();
            QDBusMessage message = QDBusMessage::createSignal("/Daemon", "org.kde.KTimeZoned", "configChanged");
            QDBusConnection::sessionBus().send(message);
        }
    }

    // Read zone.tab and create a collection of KTimeZone instances
    readZoneTab(f);

    mZonetabWatch = new KDirWatch(this);
    mZonetabWatch->addFile(mZoneTab);
    connect(mZonetabWatch, SIGNAL(dirty(const QString&)), SLOT(zonetab_Changed(const QString&)));

    // Find the local system time zone and set up file monitors to detect changes
    findLocalZone();
}

// Check if the local zone has been updated, and if so, write the new
// zone to the config file and notify interested parties.
void KTimeZoned::updateLocalZone()
{
    if (mConfigLocalZone != mLocalZone)
    {
        KConfig config(QLatin1String("ktimezonedrc"));
        KConfigGroup group(&config, "TimeZones");
        mConfigLocalZone = mLocalZone;
        group.writeEntry(LOCAL_ZONE, mConfigLocalZone);
        group.sync();

        QDBusMessage message = QDBusMessage::createSignal("/Daemon", "org.kde.KTimeZoned", "configChanged");
        QDBusConnection::sessionBus().send(message);
    }
}

/*
 * Find the location of the zoneinfo files and store in mZoneinfoDir.
 * Open or if necessary create zone.tab.
 */
bool KTimeZoned::findZoneTab(QFile& f)
{
#if defined(SOLARIS) || defined(USE_SOLARIS)
    const QString ZONE_TAB_FILE = QLatin1String("/tab/zone_sun.tab");
    const QString ZONE_INFO_DIR = QLatin1String("/usr/share/lib/zoneinfo");
#else
    const QString ZONE_TAB_FILE = QLatin1String("/zone.tab");
    const QString ZONE_INFO_DIR = QLatin1String("/usr/share/zoneinfo");
#endif

    mZoneTabCache = NoCache;

    // Find and open zone.tab - it's all easy except knowing where to look.
    // Try the LSB location first.
    QDir dir;
    QString zoneinfoDir = ZONE_INFO_DIR;
    // make a note if the dir exists; whether it contains zone.tab or not
    if (dir.exists(zoneinfoDir))
    {
        mZoneinfoDir = zoneinfoDir;
        f.setFileName(zoneinfoDir + ZONE_TAB_FILE);
        if (f.open(QIODevice::ReadOnly))
            return true;
        kDebug(1221) << "Can't open " << f.fileName();
    }

    zoneinfoDir = QLatin1String("/usr/lib/zoneinfo");
    if (dir.exists(zoneinfoDir))
    {
        mZoneinfoDir = zoneinfoDir;
        f.setFileName(zoneinfoDir + ZONE_TAB_FILE);
        if (f.open(QIODevice::ReadOnly))
            return true;
        kDebug(1221) << "Can't open " << f.fileName();
    }

    zoneinfoDir = ::getenv("TZDIR");
    if (!zoneinfoDir.isEmpty() && dir.exists(zoneinfoDir))
    {
        mZoneinfoDir = zoneinfoDir;
        f.setFileName(zoneinfoDir + ZONE_TAB_FILE);
        if (f.open(QIODevice::ReadOnly))
            return true;
        kDebug(1221) << "Can't open " << f.fileName();
    }

    zoneinfoDir = QLatin1String("/usr/share/lib/zoneinfo");
    if (dir.exists(zoneinfoDir + QLatin1String("/src")))
    {
        mZoneinfoDir = zoneinfoDir;
        // Solaris support. Synthesise something that looks like a zone.tab,
        // and cache it between sessions.
        //
        // grep -h ^Zone /usr/share/lib/zoneinfo/src/* | awk '{print "??\t+9999+99999\t" $2}'
        //
        // where the country code is set to "??" and the latitude/longitude
        // values are dummies.
        //
        QDir d(mZoneinfoDir + QLatin1String("/src"));
        d.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        QStringList fileList = d.entryList();

        mZoneTab = KStandardDirs::locateLocal("cache", QLatin1String("zone.tab"));
        f.setFileName(mZoneTab);
        if (!f.open(QIODevice::WriteOnly))
        {
            kError(1221) << "Could not create zone.tab cache" << endl;
            return false;
        }

        QFile zoneFile;
        QList<QByteArray> tokens;
        QByteArray line;
        line.reserve(1024);
        QTextStream tmpStream(&f);
        qint64 r;
        for (int i = 0, end = fileList.count();  i < end;  ++i)
        {
            zoneFile.setFileName(d.filePath(fileList[i].toLatin1()));
            if (!zoneFile.open(QIODevice::ReadOnly))
            {
                kDebug(1221) << "Could not open file '" << zoneFile.fileName().toLatin1() \
                         << "' for reading." << endl;
                continue;
            }
            while (!zoneFile.atEnd())
            {
                if ((r = zoneFile.readLine(line.data(), 1023)) > 0
                &&  line.startsWith("Zone"))
                {
                    line.replace('\t', ' ');    // change tabs to spaces
                    tokens = line.split(' ');
                    for (int j = 0, jend = tokens.count();  j < jend;  ++j)
                        if (tokens[j].endsWith(' '))
                            tokens[j].chop(1);
                    tmpStream << "??\t+9999+99999\t" << tokens[1] << "\n";
                }
            }
            zoneFile.close();
        }
        f.close();
        if (!f.open(QIODevice::ReadOnly))
        {
            kError(1221) << "Could not reopen zone.tab cache file for reading." << endl;
            return false;
        }
        mZoneTabCache = Solaris;
        return true;
    }
    return false;
}

// Parse zone.tab and for each time zone, create a KSystemTimeZone instance.
// Note that only data needed by this module is specified to KSystemTimeZone.
void KTimeZoned::readZoneTab(QFile &f)
{
    // Parse the already open real or fake zone.tab.
    QRegExp lineSeparator("[ \t]");
    if (!mSource)
        mSource = new KSystemTimeZoneSource;
    mZones.clear();
    QTextStream str(&f);
    while (!str.atEnd())
    {
        // Read the next line, but limit its length to guard against crashing
        // due to a corrupt very large zone.tab (see KDE bug 224868).
        QString line = str.readLine(MAX_ZONE_TAB_LINE_LENGTH);
        if (line.isEmpty() || line[0] == '#')
            continue;
        QStringList tokens = KStringHandler::perlSplit(lineSeparator, line, 4);
        int n = tokens.count();
        if (n < 3)
        {
            kError(1221) << "readZoneTab(): invalid record: " << line << endl;
            continue;
        }

        // Add entry to list.
        if (tokens[0] == "??")
            tokens[0] = "";
        else if (!tokens[0].isEmpty())
            mHaveCountryCodes = true;
        mZones.add(KSystemTimeZone(mSource, tokens[2], tokens[0]));
    }
    f.close();
}

// Find the local time zone, starting from scratch.
void KTimeZoned::findLocalZone()
{
    delete mDirWatch;
    mDirWatch = 0;
    mLocalZone.clear();
    mLocalIdFile.clear();
    mLocalIdFile2.clear();
    mLocalZoneDataFile.clear();

    // SOLUTION 1: DEFINITIVE.
    // First try the simplest solution of checking for well-formed TZ setting.
    const char *envtz = ::getenv("TZ");
    if (checkTZ(envtz))
    {
        mSavedTZ = envtz;
        if (!mLocalZone.isEmpty()) kDebug(1221)<<"TZ: "<<mLocalZone;
    }

    if (mLocalZone.isEmpty())
    {
        // SOLUTION 2: DEFINITIVE.
        // BSD & Linux support: local time zone id in /etc/timezone.
        checkTimezone();
    }
    if (mLocalZone.isEmpty() && !mZoneinfoDir.isEmpty())
    {
        // SOLUTION 3: DEFINITIVE.
        // Try to follow any /etc/localtime symlink to a zoneinfo file.
        // SOLUTION 4: DEFINITIVE.
        // Try to match /etc/localtime against the list of zoneinfo files.
        matchZoneFile(QLatin1String("/etc/localtime"));
    }
    if (mLocalZone.isEmpty())
    {
        // SOLUTION 5: DEFINITIVE.
        // Look for setting in /etc/rc.conf or /etc/rc.local.
        checkRcFile();
    }
    if (mLocalZone.isEmpty())
    {
        // SOLUTION 6: DEFINITIVE.
        // Solaris support using /etc/default/init.
        checkDefaultInit();
    }

    if (mLocalZone.isEmpty())
    {
        // The local time zone is not defined by a file.
        // Watch for creation of /etc/localtime in case it gets created later.
        // TODO: If under BSD it is possible for /etc/timezone to be missing but
        //       created later, we should also watch for its creation.
        mLocalIdFile = QLatin1String("/etc/localtime");
    }
    // Watch for changes in the file defining the local time zone so as to be
    // notified of any change in it.
    mDirWatch = new KDirWatch(this);
    mDirWatch->addFile(mLocalIdFile);
    if (!mLocalIdFile2.isEmpty())
        mDirWatch->addFile(mLocalIdFile2);
    if (!mLocalZoneDataFile.isEmpty())
        mDirWatch->addFile(mLocalZoneDataFile);
    connect(mDirWatch, SIGNAL(dirty(const QString&)), SLOT(localChanged(const QString&)));
    connect(mDirWatch, SIGNAL(deleted(const QString&)), SLOT(localChanged(const QString&)));
    connect(mDirWatch, SIGNAL(created(const QString&)), SLOT(localChanged(const QString&)));

    if (mLocalZone.isEmpty() && !mZoneinfoDir.isEmpty())
    {
        // SOLUTION 7: HEURISTIC.
        // None of the deterministic stuff above has worked: try a heuristic. We
        // try to find a pair of matching time zone abbreviations...that way, we'll
        // likely return a value in the user's own country.
        tzset();
        QByteArray tzname0(tzname[0]);   // store copies, because zone.parse() will change them
        QByteArray tzname1(tzname[1]);
        int bestOffset = INT_MAX;
        KSystemTimeZoneSource::startParseBlock();
        const KTimeZones::ZoneMap zmap = mZones.zones();
        for (KTimeZones::ZoneMap::ConstIterator it = zmap.constBegin(), end = zmap.constEnd();  it != end;  ++it)
        {
            KTimeZone zone = it.value();
            int candidateOffset = qAbs(zone.currentOffset(Qt::LocalTime));
            if (candidateOffset < bestOffset
            &&  zone.parse())
            {
                QList<QByteArray> abbrs = zone.abbreviations();
                if (abbrs.contains(tzname0)  &&  abbrs.contains(tzname1))
                {
                    // kDebug(1221) << "local=" << zone.name();
                    mLocalZone = zone.name();
                    bestOffset = candidateOffset;
                    if (!bestOffset)
                        break;
                }
            }
        }
        KSystemTimeZoneSource::endParseBlock();
        if (!mLocalZone.isEmpty())
        {
            mLocalMethod = TzName;
            kDebug(1221)<<"tzname: "<<mLocalZone;
        }
    }
    if (mLocalZone.isEmpty())
    {
        // SOLUTION 8: FAILSAFE.
        mLocalZone = KTimeZone::utc().name();
        mLocalMethod = Utc;
        if (!mLocalZone.isEmpty()) kDebug(1221)<<"Failsafe: "<<mLocalZone;
    }

    // Finally, if the local zone identity has changed, store
    // the new one in the config file.
    updateLocalZone();
}

// Called when KDirWatch detects a change in zone.tab
void KTimeZoned::zonetab_Changed(const QString& path)
{
    kDebug(1221) << "zone.tab changed";
    if (path != mZoneTab)
    {
        kError(1221) << "Wrong path (" << path << ") for zone.tab";
        return;
    }
    QDBusMessage message = QDBusMessage::createSignal("/Daemon", "org.kde.KTimeZoned", "zonetabChanged");
    QList<QVariant> args;
    args += mZoneTab;
    message.setArguments(args);
    QDBusConnection::sessionBus().send(message);

    // Reread zone.tab and recreate the collection of KTimeZone instances,
    // in case any zones have been created or deleted and one of them
    // subsequently becomes the local zone.
    QFile f;
    f.setFileName(mZoneTab);
    if (!f.open(QIODevice::ReadOnly))
        kError(1221) << "Could not open zone.tab (" << mZoneTab << ") to reread";
    else
        readZoneTab(f);
}

// Called when KDirWatch detects a change
void KTimeZoned::localChanged(const QString& path)
{
    if (path == mLocalZoneDataFile)
    {
        // Only need to update the definition of the local zone,
        // not its identity.
        QDBusMessage message = QDBusMessage::createSignal("/Daemon", "org.kde.KTimeZoned", "zoneDefinitionChanged");
        QList<QVariant> args;
        args += mLocalZone;
        message.setArguments(args);
        QDBusConnection::sessionBus().send(message);
        return;
    }
    QString oldDataFile = mLocalZoneDataFile;
    switch (mLocalMethod)
    {
        case EnvTzLink:
        case EnvTzFile:
        {
            const char *envtz = ::getenv("TZ");
            if (mSavedTZ != envtz)
            {
                // TZ has changed - start from scratch again
                findLocalZone();
                return;
            }
            // The contents of the file pointed to by TZ has changed.
        }
            // Fall through to LocaltimeLink
        case LocaltimeLink:
        case LocaltimeCopy:
        // The fallback methods below also set a watch for /etc/localtime in
        // case it gets created.
        case TzName:
        case Utc:
            matchZoneFile(mLocalIdFile);
            break;
        case Timezone:
            checkTimezone();
            break;
        case RcFile:
            checkRcFile();
            break;
        case DefaultInit:
            checkDefaultInit();
            break;
        default:
            return;
    }
    if (oldDataFile != mLocalZoneDataFile)
    {
        if (!oldDataFile.isEmpty())
            mDirWatch->removeFile(oldDataFile);
        if (!mLocalZoneDataFile.isEmpty())
            mDirWatch->addFile(mLocalZoneDataFile);
    }
    updateLocalZone();
}

bool KTimeZoned::checkTZ(const char *envZone)
{
    // SOLUTION 1: DEFINITIVE.
    // First try the simplest solution of checking for well-formed TZ setting.
    if (envZone)
    {
        if (envZone[0] == '\0')
        {
            mLocalMethod = EnvTz;
            mLocalZone = KTimeZone::utc().name();
            mLocalIdFile.clear();
            mLocalZoneDataFile.clear();
            return true;
        }
        if (envZone[0] == ':')
        {
            // TZ specifies a file name, either relative to zoneinfo/ or absolute.
            QString TZfile = QFile::decodeName(envZone + 1);
            if (TZfile.startsWith(mZoneinfoDir))
            {
                // It's an absolute file name in the zoneinfo directory.
                // Convert it to a file name relative to zoneinfo/.
                TZfile = TZfile.mid(mZoneinfoDir.length());
            }
            if (TZfile.startsWith(QLatin1Char('/')))
            {
                // It's an absolute file name.
                QString symlink;
                if (matchZoneFile(TZfile))
                {
                    mLocalMethod = static_cast<LocalMethod>(EnvTz | (mLocalMethod & TypeMask));
                    return true;
                }
            }
            else if (!TZfile.isEmpty())
            {
                // It's a file name relative to zoneinfo/
                mLocalZone = TZfile;
                if (!mLocalZone.isEmpty())
                {
                    mLocalMethod = EnvTz;
                    mLocalZoneDataFile = mZoneinfoDir + '/' + TZfile;
                    mLocalIdFile.clear();
                    return true;
                }
            }
        }
    }
    return false;
}

bool KTimeZoned::checkTimezone()
{
    // SOLUTION 2: DEFINITIVE.
    // BSD support.
    QFile f;
    f.setFileName(QLatin1String("/etc/timezone"));
    if (!f.open(QIODevice::ReadOnly))
        return false;
    // Read the first line of the file.
    QTextStream ts(&f);
    ts.setCodec("ISO-8859-1");
    QString zoneName;
    if (!ts.atEnd())
        zoneName = ts.readLine();
    f.close();
    if (zoneName.isEmpty())
        return false;
    if (!setLocalZone(zoneName))
        return false;
    mLocalMethod = Timezone;
    mLocalIdFile = f.fileName();
    kDebug(1221)<<"/etc/timezone: "<<mLocalZone;
    return true;
}

bool KTimeZoned::matchZoneFile(const QString &path)
{
    // SOLUTION 3: DEFINITIVE.
    // Try to follow any symlink to a zoneinfo file.
    // Get the path of the file which the symlink points to.
    QFile f;
    f.setFileName(path);
    QFileInfo fi(f);
    if (fi.isSymLink())
    {
        // The file is a symlink.
        QString zoneInfoFileName = fi.canonicalFilePath();
        QFileInfo fiz(zoneInfoFileName);
        if (fiz.exists() && fiz.isReadable())
        {
            if (zoneInfoFileName.startsWith(mZoneinfoDir))
            {
                // We've got the zoneinfo file path.
                // The time zone name is the part of the path after the zoneinfo directory.
                // Note that some systems (e.g. Gentoo) have zones under zoneinfo which
                // are not in zone.tab, so don't validate against mZones.
                mLocalZone = zoneInfoFileName.mid(mZoneinfoDir.length() + 1);
                // kDebug(1221) << "local=" << mLocalZone;
            }
            else
            {
                // It isn't a zoneinfo file or a copy thereof.
                // Use the absolute path as the time zone name.
                mLocalZone = f.fileName();
            }
            mLocalMethod = LocaltimeLink;
            mLocalIdFile = f.fileName();
            mLocalZoneDataFile = zoneInfoFileName;
            kDebug(1221)<<mLocalIdFile<<": "<<mLocalZone;
            return true;
        }
    }
    else if (f.open(QIODevice::ReadOnly))
    {
        // SOLUTION 4: DEFINITIVE.
        // Try to match the file against the list of zoneinfo files.

        // Compute the file's MD5 sum.
        KMD5 context("");
        context.reset();
        context.update(f);
        qlonglong referenceSize = f.size();
        QString referenceMd5Sum = context.hexDigest();
        MD5Map::ConstIterator it5, end5;
        KTimeZone local;
        QString zoneName;

        if (!mConfigLocalZone.isEmpty())
        {
            // We know the local zone from last time.
            // Check whether the file still matches it.
            KTimeZone tzone = mZones.zone(mConfigLocalZone);
            if (tzone.isValid())
            {
                local = compareChecksum(tzone, referenceMd5Sum, referenceSize);
                if (local.isValid())
                    zoneName = local.name();
            }
        }

        if (!local.isValid() && mHaveCountryCodes)
        {
            /* Look for time zones with the user's country code.
             * This has two advantages: 1) it shortens the search;
             * 2) it increases the chance of the correctly titled time zone
             * being found, since multiple time zones can have identical
             * definitions. For example, Europe/Guernsey is identical to
             * Europe/London, but the latter is more likely to be the right
             * zone name for a user with 'gb' country code.
             */
            QString country = KGlobal::locale()->country().toUpper();
            const KTimeZones::ZoneMap zmap = mZones.zones();
            for (KTimeZones::ZoneMap::ConstIterator zit = zmap.constBegin(), zend = zmap.constEnd();  zit != zend;  ++zit)
            {
                KTimeZone tzone = zit.value();
                if (tzone.countryCode() == country)
                {
                    local = compareChecksum(tzone, referenceMd5Sum, referenceSize);
                    if (local.isValid())
                    {
                        zoneName = local.name();
                        break;
                    }
                }
            }
        }

        if (!local.isValid())
        {
            // Look for a checksum match with the cached checksum values
            MD5Map oldChecksums = mMd5Sums;   // save a copy of the existing checksums
            for (it5 = mMd5Sums.constBegin(), end5 = mMd5Sums.constEnd();  it5 != end5;  ++it5)
            {
                if (it5.value() == referenceMd5Sum)
                {
                    // The cached checksum matches. Ensure that the file hasn't changed.
                    if (compareChecksum(it5, referenceMd5Sum, referenceSize))
                    {
                        zoneName = it5.key();
                        local = mZones.zone(zoneName);
                        if (local.isValid())
                            break;
                    }
                    oldChecksums.clear();    // the cache has been cleared
                    break;
                }
            }

            if (!local.isValid())
            {
                // The checksum didn't match any in the cache.
                // Continue building missing entries in the cache on the assumption that
                // we haven't previously looked at the zoneinfo file which matches.
                const KTimeZones::ZoneMap zmap = mZones.zones();
                for (KTimeZones::ZoneMap::ConstIterator zit = zmap.constBegin(), zend = zmap.constEnd();  zit != zend;  ++zit)
                {
                    KTimeZone zone = zit.value();
                    zoneName = zone.name();
                    if (!mMd5Sums.contains(zoneName))
                    {
                        QString candidateMd5Sum = calcChecksum(zoneName, referenceSize);
                        if (candidateMd5Sum == referenceMd5Sum)
                        {
                            // kDebug(1221) << "local=" << zone.name();
                            local = zone;
                            break;
                        }
                    }
                }
            }

            if (!local.isValid())
            {
                // Didn't find the file, so presumably a previously cached checksum must
                // have changed. Delete all the old checksums.
                MD5Map::ConstIterator mit;
                MD5Map::ConstIterator mend = oldChecksums.constEnd();
                for (mit = oldChecksums.constBegin();  mit != mend;  ++mit)
                    mMd5Sums.remove(mit.key());

                // And recalculate the old checksums
                for (mit = oldChecksums.constBegin(); mit != mend; ++mit)
                {
                    zoneName = mit.key();
                    QString candidateMd5Sum = calcChecksum(zoneName, referenceSize);
                    if (candidateMd5Sum == referenceMd5Sum)
                    {
                        // kDebug(1221) << "local=" << zoneName;
                        local = mZones.zone(zoneName);
                        break;
                    }
                }
            }
        }
        bool success = false;
        if (local.isValid())
        {
            // The file matches a zoneinfo file
            mLocalZone = zoneName;
            mLocalZoneDataFile = mZoneinfoDir + '/' + zoneName;
            success = true;
        }
        else
        {
            // The file doesn't match a zoneinfo file. If it's a TZfile, use it directly.
            // Read the file type identifier.
            char buff[4];
            f.reset();
            QDataStream str(&f);
            if (str.readRawData(buff, 4) == 4
            &&  buff[0] == 'T' && buff[1] == 'Z' && buff[2] == 'i' && buff[3] == 'f')
            {
                // Use its absolute path as the zone name.
                mLocalZone = f.fileName();
                mLocalZoneDataFile.clear();
                success = true;
            }
        }
        f.close();
        if (success)
        {
            mLocalMethod = LocaltimeCopy;
            mLocalIdFile = f.fileName();
            kDebug(1221)<<mLocalIdFile<<": "<<mLocalZone;
            return true;
        }
    }
    return false;
}

bool KTimeZoned::checkRcFile()
{
    // SOLUTION 5: DEFINITIVE.
    // Look for setting in /etc/rc.conf or /etc/rc.local,
    // with priority to /etc/rc.local.
    if (findKey(QLatin1String("/etc/rc.local"), "TIMEZONE"))
    {
        mLocalIdFile2.clear();
        kDebug(1221)<<"/etc/rc.local: "<<mLocalZone;
    }
    else
    {
        if (!findKey(QLatin1String("/etc/rc.conf"), "TIMEZONE"))
            return false;
        mLocalIdFile2 = mLocalIdFile;
        mLocalIdFile  = QLatin1String("/etc/rc.local");
        kDebug(1221)<<"/etc/rc.conf: "<<mLocalZone;
    }
    mLocalMethod = RcFile;
    return true;
}

bool KTimeZoned::checkDefaultInit()
{
    // SOLUTION 6: DEFINITIVE.
    // Solaris support using /etc/default/init.
    if (!findKey(QLatin1String("/etc/default/init"), "TZ"))
        return false;
    mLocalMethod = DefaultInit;
    kDebug(1221)<<"/etc/default/init: "<<mLocalZone;
    return true;
}

bool KTimeZoned::findKey(const QString &path, const QString &key)
{
    QFile f;
    f.setFileName(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QString line;
    QString zoneName;
    QRegExp keyexp('^' + key + "\\s*=\\s*");
    QTextStream ts(&f);
    ts.setCodec("ISO-8859-1");
    while (!ts.atEnd())
    {
        line = ts.readLine();
        if (keyexp.indexIn(line) == 0)
        {
            zoneName = line.mid(keyexp.matchedLength());
            break;
        }
    }
    f.close();
    if (zoneName.isEmpty())
        return false;
    if (!setLocalZone(zoneName))
        return false;
    kDebug(1221) << "Key:" << key << "->" << zoneName;
    mLocalIdFile = f.fileName();
    return true;
}

// Check whether the zone name is valid, either as a zone in zone.tab or
// as another file in the zoneinfo directory.
// If valid, set the local zone information.
bool KTimeZoned::setLocalZone(const QString &zoneName)
{
    KTimeZone local = mZones.zone(zoneName);
    if (!local.isValid())
    {
        // It isn't a recognised zone in zone.tab.
        // Note that some systems (e.g. Gentoo) have zones under zoneinfo which
        // are not in zone.tab, so check if it points to another zone file.
        if (mZoneinfoDir.isEmpty())
            return false;
        QString path = mZoneinfoDir + '/' + zoneName;
        QFile qf;
        qf.setFileName(path);
        QFileInfo fi(qf);
        if (fi.isSymLink())
            fi.setFile(fi.canonicalFilePath());
        if (!fi.exists() || !fi.isReadable())
            return false;
    }
    mLocalZone = zoneName;
    mLocalZoneDataFile = mZoneinfoDir.isEmpty() ? QString() : mZoneinfoDir + '/' + zoneName;
    return true;
}

// Check whether the checksum for a time zone matches a given saved checksum.
KTimeZone KTimeZoned::compareChecksum(const KTimeZone &zone, const QString &referenceMd5Sum, qlonglong size)
{
    MD5Map::ConstIterator it5 = mMd5Sums.constFind(zone.name());
    if (it5 == mMd5Sums.constEnd())
    {
        // No checksum has been computed yet for this zone file.
        // Compute it now.
        QString candidateMd5Sum = calcChecksum(zone.name(), size);
        if (candidateMd5Sum == referenceMd5Sum)
        {
            // kDebug(1221) << "local=" << zone.name();
            return zone;
        }
        return KTimeZone();
    }
    if (it5.value() == referenceMd5Sum)
    {
        // The cached checksum matches. Ensure that the file hasn't changed.
        if (compareChecksum(it5, referenceMd5Sum, size))
            return mZones.zone(it5.key());
    }
    return KTimeZone();
}

// Check whether a checksum matches a given saved checksum.
// Returns false if the file no longer matches and cache was cleared.
bool KTimeZoned::compareChecksum(MD5Map::ConstIterator it5, const QString &referenceMd5Sum, qlonglong size)
{
    // The cached checksum matches. Ensure that the file hasn't changed.
    QString zoneName = it5.key();
    QString candidateMd5Sum = calcChecksum(zoneName, size);
    if (candidateMd5Sum.isNull())
        mMd5Sums.remove(zoneName);    // no match - wrong file size
    else if (candidateMd5Sum == referenceMd5Sum)
        return true;

    // File(s) have changed, so clear the cache
    mMd5Sums.clear();
    mMd5Sums[zoneName] = candidateMd5Sum;    // reinsert the newly calculated checksum
    return false;
}

// Calculate the MD5 checksum for the given zone file, provided that its size matches.
// The calculated checksum is cached.
QString KTimeZoned::calcChecksum(const QString &zoneName, qlonglong size)
{
    QString path = mZoneinfoDir + '/' + zoneName;
    QFileInfo fi(path);
    if (static_cast<qlonglong>(fi.size()) == size)
    {
        // Only do the heavy lifting for file sizes which match.
        QFile f;
        f.setFileName(path);
        if (f.open(QIODevice::ReadOnly))
        {
            KMD5 context("");
            context.reset();
            context.update(f);
            QString candidateMd5Sum = context.hexDigest();
            f.close();
            mMd5Sums[zoneName] = candidateMd5Sum;    // cache the new checksum
            return candidateMd5Sum;
        }
    }
    return QString();
}
