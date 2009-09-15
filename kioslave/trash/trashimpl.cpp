/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

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

#define QT_NO_CAST_FROM_ASCII

#include "trashimpl.h"
#include "discspaceutil.h"

#include <klocale.h>
#include <kde_file.h>
#include <kio/global.h>
#include <kio/renamedialog.h>
#include <kio/job.h>
#include <kio/chmodjob.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kdebug.h>
#include <kurl.h>
#include <kdirnotify.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kfileitem.h>
#include <kconfiggroup.h>
#include <kmountpoint.h>

#include <QApplication>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <kjobuidelegate.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>

#include <solid/device.h>
#include <solid/block.h>
#include <solid/storageaccess.h>

TrashImpl::TrashImpl() :
    QObject(),
    m_lastErrorCode( 0 ),
    m_initStatus( InitToBeDone ),
    m_homeDevice( 0 ),
    m_trashDirectoriesScanned( false ),
    // not using kio_trashrc since KIO uses that one already for kio_trash
    // so better have a separate one, for faster parsing by e.g. kmimetype.cpp
    m_config(QString::fromLatin1("trashrc"), KConfig::SimpleConfig)
{
    KDE_struct_stat buff;
    if ( KDE_lstat( QFile::encodeName( QDir::homePath() ), &buff ) == 0 ) {
        m_homeDevice = buff.st_dev;
    } else {
        kError() << "Should never happen: couldn't stat $HOME " << strerror( errno ) << endl;
    }
}

/**
 * Test if a directory exists, create otherwise
 * @param _name full path of the directory
 * @return errorcode, or 0 if the dir was created or existed already
 * Warning, don't use return value like a bool
 */
int TrashImpl::testDir( const QString &_name ) const
{
  DIR *dp = opendir( QFile::encodeName(_name) );
  if ( dp == NULL )
  {
    QString name = _name;
    if (name.endsWith(QLatin1Char('/')))
      name.truncate( name.length() - 1 );
    QByteArray path = QFile::encodeName(name);

    bool ok = KDE_mkdir( path, S_IRWXU ) == 0;
    if ( !ok && errno == EEXIST ) {
#if 0 // this would require to use SlaveBase's method to ask the question
        //int ret = KMessageBox::warningYesNo( 0, i18n("%1 is a file, but KDE needs it to be a directory. Move it to %2.orig and create directory?").arg(name).arg(name) );
        //if ( ret == KMessageBox::Yes ) {
#endif
            if ( KDE_rename( path, path + ".orig" ) == 0 ) {
                ok = KDE_mkdir( path, S_IRWXU ) == 0;
            } else { // foo.orig existed already. How likely is that?
                ok = false;
            }
            if ( !ok ) {
                return KIO::ERR_DIR_ALREADY_EXIST;
            }
#if 0
        //} else {
        //    return 0;
        //}
#endif
    }
    if ( !ok )
    {
        //KMessageBox::sorry( 0, i18n( "Could not create directory %1. Check for permissions." ).arg( name ) );
        kWarning() << "could not create" << name ;
        return KIO::ERR_COULD_NOT_MKDIR;
    } else {
        kDebug() << name << "created.";
    }
  }
  else // exists already
  {
    closedir( dp );
  }
  return 0; // success
}

bool TrashImpl::init()
{
    if ( m_initStatus == InitOK )
        return true;
    if ( m_initStatus == InitError )
        return false;

    // Check the trash directory and its info and files subdirs
    // see also kdesktop/init.cc for first time initialization
    m_initStatus = InitError;
    // $XDG_DATA_HOME/Trash, i.e. ~/.local/share/Trash by default.
    const QString xdgDataDir = KGlobal::dirs()->localxdgdatadir();
    if ( !KStandardDirs::makeDir( xdgDataDir, 0700 ) ) {
        kWarning() << "failed to create " << xdgDataDir ;
        return false;
    }

    const QString trashDir = xdgDataDir + QString::fromLatin1("Trash");
    int err;
    if ( ( err = testDir( trashDir ) ) ) {
        error( err, trashDir );
        return false;
    }
    if ( ( err = testDir( trashDir + QString::fromLatin1("/info") ) ) ) {
        error( err, trashDir + QString::fromLatin1("/info") );
        return false;
    }
    if ( ( err = testDir( trashDir + QString::fromLatin1("/files") ) ) ) {
        error( err, trashDir + QString::fromLatin1("/files") );
        return false;
    }
    m_trashDirectories.insert( 0, trashDir );
    m_initStatus = InitOK;
    kDebug() << "initialization OK, home trash dir: " << trashDir;
    return true;
}

void TrashImpl::migrateOldTrash()
{
    kDebug() ;

    KConfigGroup g( KGlobal::config(), "Paths" );
    const QString oldTrashDir = g.readPathEntry( "Trash", QString() );

    if ( oldTrashDir.isEmpty() )
        return;

    const QStringList entries = listDir( oldTrashDir );
    bool allOK = true;
    for ( QStringList::const_iterator entryIt = entries.begin(), entryEnd = entries.end();
          entryIt != entryEnd ; ++entryIt )
    {
        QString srcPath = *entryIt;
        if ( srcPath == QLatin1String(".") || srcPath == QLatin1String("..") || srcPath == QLatin1String(".directory") )
            continue;
        srcPath.prepend( oldTrashDir ); // make absolute
        int trashId;
        QString fileId;
        if ( !createInfo( srcPath, trashId, fileId ) ) {
            kWarning() << "Trash migration: failed to create info for " << srcPath ;
            allOK = false;
        } else {
            bool ok = moveToTrash( srcPath, trashId, fileId );
            if ( !ok ) {
                (void)deleteInfo( trashId, fileId );
                kWarning() << "Trash migration: failed to create info for " << srcPath ;
                allOK = false;
            } else {
                kDebug() << "Trash migration: moved " << srcPath;
            }
        }
    }
    if ( allOK ) {
        // We need to remove the old one, otherwise the desktop will have two trashcans...
        kDebug() << "Trash migration: all OK, removing old trash directory";
        synchronousDel( oldTrashDir, false, true );
    }
}

bool TrashImpl::createInfo( const QString& origPath, int& trashId, QString& fileId )
{
    kDebug() << origPath;
    // Check source
    const QByteArray origPath_c( QFile::encodeName( origPath ) );
    /*
         * off_t should be 64bit on Unix systems to have large file support
         * FIXME: on windows this gets disabled until trash gets integrated
         */
#ifndef Q_OS_WIN
    char off_t_should_be_64bit[sizeof(off_t) >= 8 ? 1:-1]; (void)off_t_should_be_64bit;
#endif
    KDE_struct_stat buff_src;
    if ( KDE_lstat( origPath_c.data(), &buff_src ) == -1 ) {
        if ( errno == EACCES )
           error( KIO::ERR_ACCESS_DENIED, origPath );
        else
           error( KIO::ERR_DOES_NOT_EXIST, origPath );
        return false;
    }

    // Choose destination trash
    trashId = findTrashDirectory( origPath );
    if ( trashId < 0 ) {
        kWarning() << "OUCH - internal error, TrashImpl::findTrashDirectory returned " << trashId ;
        return false; // ### error() needed?
    }
    kDebug() << "trashing to " << trashId;

    // Grab original filename
    KUrl url;
    url.setPath( origPath );
    const QString origFileName = url.fileName();

    // Make destination file in info/
    url.setPath( infoPath( trashId, origFileName ) ); // we first try with origFileName
    KUrl baseDirectory;
    baseDirectory.setPath( url.directory() );
    // Here we need to use O_EXCL to avoid race conditions with other kioslave processes
    int fd = 0;
    do {
        kDebug() << "trying to create " << url.path() ;
        fd = KDE_open( QFile::encodeName( url.path() ), O_WRONLY | O_CREAT | O_EXCL, 0600 );
        if ( fd < 0 ) {
            if ( errno == EEXIST ) {
                url.setFileName( KIO::RenameDialog::suggestName( baseDirectory, url.fileName() ) );
                // and try again on the next iteration
            } else {
                error( KIO::ERR_COULD_NOT_WRITE, url.path() );
                return false;
            }
        }
    } while ( fd < 0 );
    const QString infoPath = url.path();
    fileId = url.fileName();
    Q_ASSERT( fileId.endsWith( QLatin1String(".trashinfo") ) );
    fileId.truncate( fileId.length() - 10 ); // remove .trashinfo from fileId

    FILE* file = ::fdopen( fd, "w" );
    if ( !file ) { // can't see how this would happen
        error( KIO::ERR_COULD_NOT_WRITE, infoPath );
        return false;
    }

    // Contents of the info file. We could use KSimpleConfig, but that would
    // mean closing and reopening fd, i.e. opening a race condition...
    QByteArray info = "[Trash Info]\n";
    info += "Path=";
    // Escape filenames according to the way they are encoded on the filesystem
    // All this to basically get back to the raw 8-bit representation of the filename...
    if ( trashId == 0 ) // home trash: absolute path
        info += QUrl::toPercentEncoding( origPath, "/" );
    else
        info += QUrl::toPercentEncoding( makeRelativePath( topDirectoryPath( trashId ), origPath ), "/" );
    info += '\n';
    info += "DeletionDate=";
    info += QDateTime::currentDateTime().toString( Qt::ISODate ).toLatin1();
    info += '\n';
    size_t sz = info.size();

    size_t written = ::fwrite(info.data(), 1, sz, file);
    if ( written != sz ) {
        ::fclose( file );
        QFile::remove( infoPath );
        error( KIO::ERR_DISK_FULL, infoPath );
        return false;
    }

    ::fclose( file );

    kDebug() << "info file created in trashId=" << trashId << " : " << fileId;
    return true;
}

QString TrashImpl::makeRelativePath( const QString& topdir, const QString& path )
{
    const QString realPath = KStandardDirs::realFilePath( path );
    // topdir ends with '/'
    if ( realPath.startsWith( topdir ) ) {
        const QString rel = realPath.mid( topdir.length() );
        Q_ASSERT( rel[0] != QLatin1Char('/') );
        return rel;
    } else { // shouldn't happen...
        kWarning() << "Couldn't make relative path for " << realPath << " (" << path << "), with topdir=" << topdir ;
        return realPath;
    }
}

void TrashImpl::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
        &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

QString TrashImpl::infoPath( int trashId, const QString& fileId ) const
{
    QString trashPath = trashDirectoryPath( trashId );
    trashPath += QString::fromLatin1("/info/");
    trashPath += fileId;
    trashPath += QString::fromLatin1(".trashinfo");
    return trashPath;
}

QString TrashImpl::filesPath( int trashId, const QString& fileId ) const
{
    QString trashPath = trashDirectoryPath( trashId );
    trashPath += QString::fromLatin1("/files/");
    trashPath += fileId;
    return trashPath;
}

bool TrashImpl::deleteInfo( int trashId, const QString& fileId )
{
    bool ok = QFile::remove( infoPath( trashId, fileId ) );
    if ( ok )
        fileRemoved();
    return ok;
}

bool TrashImpl::moveToTrash( const QString& origPath, int trashId, const QString& fileId )
{
    kDebug() ;
    if ( !adaptTrashSize( origPath, trashId ) )
        return false;

    const QString dest = filesPath( trashId, fileId );
    if ( !move( origPath, dest ) ) {
        // Maybe the move failed due to no permissions to delete source.
        // In that case, delete dest to keep things consistent, since KIO doesn't do it.
        if ( QFileInfo( dest ).isFile() )
            QFile::remove( dest );
        else
            synchronousDel( dest, false, true );
        return false;
    }
    fileAdded();
    return true;
}

bool TrashImpl::moveFromTrash( const QString& dest, int trashId, const QString& fileId, const QString& relativePath )
{
    QString src = filesPath( trashId, fileId );
    if ( !relativePath.isEmpty() ) {
        src += QLatin1Char('/');
        src += relativePath;
    }
    if ( !move( src, dest ) )
        return false;
    return true;
}

bool TrashImpl::move( const QString& src, const QString& dest )
{
    if ( directRename( src, dest ) ) {
        // This notification is done by KIO::moveAs when using the code below
        // But if we do a direct rename we need to do the notification ourselves
        org::kde::KDirNotify::emitFilesAdded( dest );
        return true;
    }
    if ( m_lastErrorCode != KIO::ERR_UNSUPPORTED_ACTION )
        return false;

    KUrl urlSrc, urlDest;
    urlSrc.setPath( src );
    urlDest.setPath( dest );
    kDebug() << urlSrc << " -> " << urlDest;
    KIO::CopyJob* job = KIO::moveAs( urlSrc, urlDest, KIO::HideProgressInfo );
    job->setUiDelegate(0);
    connect( job, SIGNAL( result(KJob*) ),
             this, SLOT( jobFinished(KJob*) ) );
    enterLoop();

    return m_lastErrorCode == 0;
}

void TrashImpl::jobFinished(KJob* job)
{
    kDebug() << " error=" << job->error();
    error( job->error(), job->errorText() );
    emit leaveModality();
}

bool TrashImpl::copyToTrash( const QString& origPath, int trashId, const QString& fileId )
{
    kDebug() ;
    if ( !adaptTrashSize( origPath, trashId ) )
        return false;

    const QString dest = filesPath( trashId, fileId );
    if ( !copy( origPath, dest ) )
        return false;
    fileAdded();
    return true;
}

bool TrashImpl::copyFromTrash( const QString& dest, int trashId, const QString& fileId, const QString& relativePath )
{
    QString src = filesPath( trashId, fileId );
    if ( !relativePath.isEmpty() ) {
        src += QLatin1Char('/');
        src += relativePath;
    }
    return copy( src, dest );
}

bool TrashImpl::copy( const QString& src, const QString& dest )
{
    // kio_file's copy() method is quite complex (in order to be fast), let's just call it...
    m_lastErrorCode = 0;
    KUrl urlSrc;
    urlSrc.setPath( src );
    KUrl urlDest;
    urlDest.setPath( dest );
    kDebug() << "copying " << src << " to " << dest;
    KIO::CopyJob* job = KIO::copyAs( urlSrc, urlDest, KIO::HideProgressInfo );
    job->setUiDelegate(0);
    connect( job, SIGNAL( result(KJob*) ),
             this, SLOT( jobFinished(KJob*) ) );
    enterLoop();

    return m_lastErrorCode == 0;
}

bool TrashImpl::directRename( const QString& src, const QString& dest )
{
    kDebug() << src << " -> " << dest;
    if ( KDE_rename( QFile::encodeName( src ), QFile::encodeName( dest ) ) != 0 ) {
        if (errno == EXDEV) {
            error( KIO::ERR_UNSUPPORTED_ACTION, QString::fromLatin1("rename") );
        } else {
            if (( errno == EACCES ) || (errno == EPERM)) {
                error( KIO::ERR_ACCESS_DENIED, dest );
            } else if (errno == EROFS) { // The file is on a read-only filesystem
                error( KIO::ERR_CANNOT_DELETE, src );
            } else {
                error( KIO::ERR_CANNOT_RENAME, src );
            }
        }
        return false;
    }
    return true;
}

#if 0
bool TrashImplKDE_mkdir( int trashId, const QString& fileId, int permissions )
{
    const QString path = filesPath( trashId, fileId );
    if ( KDE_mkdir( QFile::encodeName( path ), permissions ) != 0 ) {
        if ( errno == EACCES ) {
            error( KIO::ERR_ACCESS_DENIED, path );
            return false;
        } else if ( errno == ENOSPC ) {
            error( KIO::ERR_DISK_FULL, path );
            return false;
        } else {
            error( KIO::ERR_COULD_NOT_MKDIR, path );
            return false;
        }
    } else {
        if ( permissions != -1 )
            ::chmod( QFile::encodeName( path ), permissions );
    }
    return true;
}
#endif

bool TrashImpl::del( int trashId, const QString& fileId )
{
    QString info = infoPath(trashId, fileId);
    QString file = filesPath(trashId, fileId);

    QByteArray info_c = QFile::encodeName(info);

    KDE_struct_stat buff;
    if ( KDE_lstat( info_c.data(), &buff ) == -1 ) {
        if ( errno == EACCES )
            error( KIO::ERR_ACCESS_DENIED, file );
        else
            error( KIO::ERR_DOES_NOT_EXIST, file );
        return false;
    }

    if ( !synchronousDel( file, true, QFileInfo(file).isDir() ) )
        return false;

    QFile::remove( info );
    fileRemoved();
    return true;
}

bool TrashImpl::synchronousDel( const QString& path, bool setLastErrorCode, bool isDir )
{
    const int oldErrorCode = m_lastErrorCode;
    const QString oldErrorMsg = m_lastErrorMessage;
    KUrl url;
    url.setPath( path );
    // First ensure that all dirs have u+w permissions,
    // otherwise we won't be able to delete files in them (#130780).
    if ( isDir ) {
        kDebug() << "chmod'ing " << url;
        KFileItem fileItem( url, QString::fromLatin1("inode/directory"), KFileItem::Unknown );
        KFileItemList fileItemList;
        fileItemList.append( fileItem );
        KIO::ChmodJob* chmodJob = KIO::chmod( fileItemList, 0200, 0200, QString(), QString(), true /*recursive*/, KIO::HideProgressInfo );
        connect( chmodJob, SIGNAL( result(KJob *) ),
                 this, SLOT( jobFinished(KJob *) ) );
        enterLoop();
    }

    KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo );
    connect( job, SIGNAL( result(KJob*) ),
             this, SLOT( jobFinished(KJob*) ) );
    enterLoop();
    bool ok = m_lastErrorCode == 0;
    if ( !setLastErrorCode ) {
        m_lastErrorCode = oldErrorCode;
        m_lastErrorMessage = oldErrorMsg;
    }
    return ok;
}

bool TrashImpl::emptyTrash()
{
    kDebug() ;
    // The naive implementation "delete info and files in every trash directory"
    // breaks when deleted directories contain files owned by other users.
    // We need to ensure that the .trashinfo file is only removed when the
    // corresponding files could indeed be removed (#116371)

    // On the other hand, we certainly want to remove any file that has no associated
    // .trashinfo file for some reason (#167051)

    QSet<QString> unremoveableFiles;

    int myErrorCode = 0;
    QString myErrorMsg;
    const TrashedFileInfoList fileInfoList = list();

    TrashedFileInfoList::const_iterator it = fileInfoList.begin();
    const TrashedFileInfoList::const_iterator end = fileInfoList.end();
    for ( ; it != end ; ++it ) {
        const TrashedFileInfo& info = *it;
        const QString filesPath = info.physicalPath;
        if ( synchronousDel( filesPath, true, true ) ) {
            QFile::remove( infoPath( info.trashId, info.fileId ) );
        } else {
            // error code is set by synchronousDel, let's remember it
            // (so that successfully removing another file doesn't erase the error)
            myErrorCode = m_lastErrorCode;
            myErrorMsg = m_lastErrorMessage;
            // and remember not to remove this file
            unremoveableFiles.insert(filesPath);
            kDebug() << "Unremoveable:" << filesPath;
        }
    }

    // Now do the orphaned-files cleanup
    TrashDirMap::const_iterator trit = m_trashDirectories.constBegin();
    for (; trit != m_trashDirectories.constEnd() ; ++trit) {
        //const int trashId = trit.key();
        QString filesDir = trit.value();
        filesDir += QString::fromLatin1("/files");
        Q_FOREACH(const QString& fileName, listDir(filesDir)) {
            if (fileName == QString::fromLatin1(".") || fileName == QString::fromLatin1(".."))
                continue;
            const QString filePath = filesDir + QLatin1Char('/') + fileName;
            if (!unremoveableFiles.contains(filePath)) {
                kWarning() << "Removing orphaned file" << filePath;
                QFile::remove(filePath);
            }
        }
    }

    m_lastErrorCode = myErrorCode;
    m_lastErrorMessage = myErrorMsg;

    fileRemoved();

    return m_lastErrorCode == 0;
}

TrashImpl::TrashedFileInfoList TrashImpl::list()
{
    // Here we scan for trash directories unconditionally. This allows
    // noticing plugged-in [e.g. removeable] devices, or new mounts etc.
    scanTrashDirectories();

    TrashedFileInfoList lst;
    // For each known trash directory...
    TrashDirMap::const_iterator it = m_trashDirectories.constBegin();
    for ( ; it != m_trashDirectories.constEnd() ; ++it ) {
        const int trashId = it.key();
        QString infoPath = it.value();
        infoPath += QString::fromLatin1("/info");
        // Code taken from kio_file
        const QStringList entryNames = listDir( infoPath );
        //char path_buffer[PATH_MAX];
        //getcwd(path_buffer, PATH_MAX - 1);
        //if ( chdir( infoPathEnc ) )
        //    continue;
        for ( QStringList::const_iterator entryIt = entryNames.constBegin(), entryEnd = entryNames.constEnd();
              entryIt != entryEnd ; ++entryIt )
        {
            QString fileName = *entryIt;
            if ( fileName == QLatin1String(".") || fileName == QLatin1String("..") )
                continue;
            if ( !fileName.endsWith( QLatin1String(".trashinfo") ) ) {
                kWarning() << "Invalid info file found in " << infoPath << " : " << fileName ;
                continue;
            }
            fileName.truncate( fileName.length() - 10 );

            TrashedFileInfo info;
            if ( infoForFile( trashId, fileName, info ) )
                lst << info;
        }
    }
    return lst;
}

// Returns the entries in a given directory - including "." and ".."
QStringList TrashImpl::listDir( const QString& physicalPath )
{
    QDir dir( physicalPath );
    return dir.entryList( QDir::Dirs | QDir::Files | QDir::Hidden );
}

bool TrashImpl::infoForFile( int trashId, const QString& fileId, TrashedFileInfo& info )
{
    kDebug() << trashId << " " << fileId;
    info.trashId = trashId; // easy :)
    info.fileId = fileId; // equally easy
    info.physicalPath = filesPath( trashId, fileId );
    return readInfoFile( infoPath( trashId, fileId ), info, trashId );
}

bool TrashImpl::readInfoFile( const QString& infoPath, TrashedFileInfo& info, int trashId )
{
    KConfig cfg( infoPath, KConfig::SimpleConfig);
    if ( !cfg.hasGroup( "Trash Info" ) ) {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, infoPath );
        return false;
    }
    const KConfigGroup group = cfg.group( "Trash Info" );
    info.origPath = QUrl::fromPercentEncoding( group.readEntry( "Path" ).toLatin1() );
    if ( info.origPath.isEmpty() )
        return false; // path is mandatory...
    if ( trashId == 0 ) {
        Q_ASSERT( info.origPath[0] == QLatin1Char('/') );
    } else {
        const QString topdir = topDirectoryPath( trashId ); // includes trailing slash
        info.origPath.prepend( topdir );
    }
    const QString line = group.readEntry( "DeletionDate" );
    if ( !line.isEmpty() ) {
        info.deletionDate = QDateTime::fromString( line, Qt::ISODate );
    }
    return true;
}

QString TrashImpl::physicalPath( int trashId, const QString& fileId, const QString& relativePath )
{
    QString filePath = filesPath( trashId, fileId );
    if ( !relativePath.isEmpty() ) {
        filePath += QLatin1Char('/');
        filePath += relativePath;
    }
    return filePath;
}

void TrashImpl::error( int e, const QString& s )
{
    if ( e )
        kDebug() << e << " " << s;
    m_lastErrorCode = e;
    m_lastErrorMessage = s;
}

bool TrashImpl::isEmpty() const
{
    // For each known trash directory...
    if ( !m_trashDirectoriesScanned )
        scanTrashDirectories();
    TrashDirMap::const_iterator it = m_trashDirectories.constBegin();
    for ( ; it != m_trashDirectories.constEnd() ; ++it ) {
        QString infoPath = it.value();
        infoPath += QString::fromLatin1("/info");

        DIR *dp = opendir( QFile::encodeName( infoPath ) );
        if ( dp )
        {
            KDE_struct_dirent *ep;
            ep = KDE_readdir( dp );
            ep = KDE_readdir( dp ); // ignore '.' and '..' dirent
            ep = KDE_readdir( dp ); // look for third file
            closedir( dp );
            if ( ep != 0 ) {
                //kDebug() << ep->d_name << " in " << infoPath << " -> not empty";
                return false; // not empty
            }
        }
    }
    return true;
}

void TrashImpl::fileAdded()
{
    m_config.reparseConfiguration();
    KConfigGroup group = m_config.group( "Status" );
    if ( group.readEntry( "Empty", true) == true ) {
        group.writeEntry( "Empty", false );
        m_config.sync();
    }
    // The apps showing the trash (e.g. kdesktop) will be notified
    // of this change when KDirNotify::FilesAdded("trash:/") is emitted,
    // which will be done by the job soon after this.
}

void TrashImpl::fileRemoved()
{
    if ( isEmpty() ) {
        KConfigGroup group = m_config.group( "Status" );
        group.writeEntry( "Empty", true );
        m_config.sync();
    }
    // The apps showing the trash (e.g. kdesktop) will be notified
    // of this change when KDirNotify::FilesRemoved(...) is emitted,
    // which will be done by the job soon after this.
}

static int idForDevice(const Solid::Device& device)
{
    const Solid::Block* block = device.as<Solid::Block>();
    kDebug() << "major=" << block->deviceMajor() << " minor=" << block->deviceMinor();
    return block->deviceMajor()*1000 + block->deviceMinor();
}

int TrashImpl::findTrashDirectory( const QString& origPath )
{
    kDebug() << origPath;
    // First check if same device as $HOME, then we use the home trash right away.
    KDE_struct_stat buff;
    if ( KDE_lstat( QFile::encodeName( origPath ), &buff ) == 0
         && buff.st_dev == m_homeDevice )
        return 0;

    KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByPath( origPath );
    if (!mp) {
        kDebug() << "KMountPoint found no mount point for" << origPath;
        return 0;
    }
    QString mountPoint = mp->mountPoint();
    const QString trashDir = trashForMountPoint( mountPoint, true );
    kDebug() << "mountPoint=" << mountPoint << " trashDir=" << trashDir;
    if ( trashDir.isEmpty() )
        return 0; // no trash available on partition
    int id = idForTrashDirectory( trashDir );
    if ( id > -1 ) {
        kDebug() << " known with id " << id;
        return id;
    }
    // new trash dir found, register it
    // but we need stability in the trash IDs, so that restoring or asking
    // for properties works even kio_trash gets killed because idle.
#if 0
    kDebug() << "found " << trashDir;
    m_trashDirectories.insert( ++m_lastId, trashDir );
    if ( !mountPoint.endsWith( '/' ) )
        mountPoint += '/';
    m_topDirectories.insert( m_lastId, mountPoint );
    return m_lastId;
#endif

    const QString query = QString::fromLatin1("[StorageAccess.accessible == true AND StorageAccess.filePath == '")+mountPoint+QString::fromLatin1("']");
    //kDebug() << "doing solid query:" << query;
    const QList<Solid::Device> lst = Solid::Device::listFromQuery(query);
    //kDebug() << "got" << lst.count() << "devices";
    if ( lst.isEmpty() ) // not a device. Maybe some tmpfs mount for instance.
        return 0; // use the home trash instead
    // Pretend we got exactly one...
    const Solid::Device device = lst[0];

    // new trash dir found, register it
    id = idForDevice( device );
    m_trashDirectories.insert( id, trashDir );
    kDebug() << "found" << trashDir << "gave it id" << id;
    if (!mountPoint.endsWith(QLatin1Char('/')))
        mountPoint += QLatin1Char('/');
    m_topDirectories.insert( id, mountPoint );

    return idForTrashDirectory( trashDir );
}

void TrashImpl::scanTrashDirectories() const
{
    const QList<Solid::Device> lst = Solid::Device::listFromQuery(QString::fromLatin1("StorageAccess.accessible == true"));
    for ( QList<Solid::Device>::ConstIterator it = lst.begin() ; it != lst.end() ; ++it ) {
        QString topdir = (*it).as<Solid::StorageAccess>()->filePath();
        QString trashDir = trashForMountPoint( topdir, false );
        if ( !trashDir.isEmpty() ) {
            // OK, trashDir is a valid trash directory. Ensure it's registered.
            int trashId = idForTrashDirectory( trashDir );
            if ( trashId == -1 ) {
                // new trash dir found, register it
                trashId = idForDevice( *it );
                m_trashDirectories.insert( trashId, trashDir );
                kDebug() << "found " << trashDir << " gave it id " << trashId;
                if (!topdir.endsWith(QLatin1Char('/')))
                    topdir += QLatin1Char('/');
                m_topDirectories.insert( trashId, topdir );
            }
        }
    }
    m_trashDirectoriesScanned = true;
}

TrashImpl::TrashDirMap TrashImpl::trashDirectories() const
{
    if ( !m_trashDirectoriesScanned )
        scanTrashDirectories();
    return m_trashDirectories;
}

TrashImpl::TrashDirMap TrashImpl::topDirectories() const
{
    if ( !m_trashDirectoriesScanned )
        scanTrashDirectories();
    return m_topDirectories;
}

QString TrashImpl::trashForMountPoint( const QString& topdir, bool createIfNeeded ) const
{
    // (1) Administrator-created $topdir/.Trash directory

    const QString rootTrashDir = topdir + QString::fromLatin1("/.Trash");
    const QByteArray rootTrashDir_c = QFile::encodeName( rootTrashDir );
    // Can't use QFileInfo here since we need to test for the sticky bit
    uid_t uid = getuid();
    KDE_struct_stat buff;
    const unsigned int requiredBits = S_ISVTX; // Sticky bit required
    if ( KDE_lstat( rootTrashDir_c, &buff ) == 0 ) {
        if ( (S_ISDIR(buff.st_mode)) // must be a dir
             && (!S_ISLNK(buff.st_mode)) // not a symlink
             && ((buff.st_mode & requiredBits) == requiredBits)
             && (::access(rootTrashDir_c, W_OK) == 0) // must be user-writable
            ) {
            const QString trashDir = rootTrashDir + QLatin1Char('/') + QString::number( uid );
            const QByteArray trashDir_c = QFile::encodeName( trashDir );
            if ( KDE_lstat( trashDir_c, &buff ) == 0 ) {
                if ( (buff.st_uid == uid) // must be owned by user
                     && (S_ISDIR(buff.st_mode)) // must be a dir
                     && (!S_ISLNK(buff.st_mode)) // not a symlink
                     && (buff.st_mode & 0777) == 0700 ) { // rwx for user
                    return trashDir;
                }
                kDebug() << "Directory " << trashDir << " exists but didn't pass the security checks, can't use it";
            }
            else if ( createIfNeeded && initTrashDirectory( trashDir_c ) ) {
                return trashDir;
            }
        } else {
            kDebug() << "Root trash dir " << rootTrashDir << " exists but didn't pass the security checks, can't use it";
        }
    }

    // (2) $topdir/.Trash-$uid
    const QString trashDir = topdir + QString::fromLatin1("/.Trash-") + QString::number( uid );
    const QByteArray trashDir_c = QFile::encodeName( trashDir );
    if ( KDE_lstat( trashDir_c, &buff ) == 0 )
    {
        if ( (buff.st_uid == uid) // must be owned by user
             && (S_ISDIR(buff.st_mode)) // must be a dir
             && (!S_ISLNK(buff.st_mode)) // not a symlink
             && ((buff.st_mode & 0777) == 0700) ) { // rwx for user, ------ for group and others

            if ( checkTrashSubdirs( trashDir_c ) )
                return trashDir;
        }
        kDebug() << "Directory " << trashDir << " exists but didn't pass the security checks, can't use it";
        // Exists, but not useable
        return QString();
    }
    if ( createIfNeeded && initTrashDirectory( trashDir_c ) ) {
        return trashDir;
    }
    return QString();
}

int TrashImpl::idForTrashDirectory( const QString& trashDir ) const
{
    // If this is too slow we can always use a reverse map...
    TrashDirMap::ConstIterator it = m_trashDirectories.constBegin();
    for ( ; it != m_trashDirectories.constEnd() ; ++it ) {
        if ( it.value() == trashDir ) {
            return it.key();
        }
    }
    return -1;
}

bool TrashImpl::initTrashDirectory( const QByteArray& trashDir_c ) const
{
    kDebug() << trashDir_c;
    if ( KDE_mkdir( trashDir_c, 0700 ) != 0 )
        return false;
    kDebug() ;
    // This trash dir will be useable only if the directory is owned by user.
    // In theory this is the case, but not on e.g. USB keys...
    uid_t uid = getuid();
    KDE_struct_stat buff;
    if ( KDE_lstat( trashDir_c, &buff ) != 0 )
        return false; // huh?
    if ( (buff.st_uid == uid) // must be owned by user
         && ((buff.st_mode & 0777) == 0700) ) { // rwx for user, --- for group and others

        return checkTrashSubdirs( trashDir_c );

    } else {
        kDebug() << trashDir_c << " just created, by it doesn't have the right permissions, must be a FAT partition. Removing it again.";
        // Not good, e.g. USB key. Delete again.
        // I'm paranoid, it would be better to find a solution that allows
        // to trash directly onto the USB key, but I don't see how that would
        // pass the security checks. It would also make the USB key appears as
        // empty when it's in fact full...
        ::rmdir( trashDir_c );
        return false;
    }
    return true;
}

bool TrashImpl::checkTrashSubdirs( const QByteArray& trashDir_c ) const
{
    // testDir currently works with a QString - ## optimize
    QString trashDir = QFile::decodeName( trashDir_c );
    const QString info = trashDir + QString::fromLatin1("/info");
    if ( testDir( info ) != 0 )
        return false;
    const QString files = trashDir + QString::fromLatin1("/files");
    if ( testDir( files ) != 0 )
        return false;
    return true;
}

QString TrashImpl::trashDirectoryPath( int trashId ) const
{
    // Never scanned for trash dirs? (This can happen after killing kio_trash
    // and reusing a directory listing from the earlier instance.)
    if ( !m_trashDirectoriesScanned )
        scanTrashDirectories();
    Q_ASSERT( m_trashDirectories.contains( trashId ) );
    return m_trashDirectories[trashId];
}

QString TrashImpl::topDirectoryPath( int trashId ) const
{
    if ( !m_trashDirectoriesScanned )
        scanTrashDirectories();
    assert( trashId != 0 );
    Q_ASSERT( m_topDirectories.contains( trashId ) );
    return m_topDirectories[trashId];
}

// Helper method. Creates a URL with the format trash:/trashid-fileid or
// trash:/trashid-fileid/relativePath/To/File for a file inside a trashed directory.
KUrl TrashImpl::makeURL( int trashId, const QString& fileId, const QString& relativePath )
{
    KUrl url;
    url.setProtocol(QString::fromLatin1("trash"));
    QString path = QString::fromLatin1("/");
    path += QString::number( trashId );
    path += QLatin1Char('-');
    path += fileId;
    if ( !relativePath.isEmpty() ) {
        path += QLatin1Char('/');
        path += relativePath;
    }
    url.setPath( path );
    return url;
}

// Helper method. Parses a trash URL with the URL scheme defined in makeURL.
// The trash:/ URL itself isn't parsed here, must be caught by the caller before hand.
bool TrashImpl::parseURL( const KUrl& url, int& trashId, QString& fileId, QString& relativePath )
{
    if (url.protocol() != QLatin1String("trash"))
        return false;
    const QString path = url.path();
    int start = 0;
    if ( path[0] == QLatin1Char('/') ) // always true I hope
        start = 1;
    int slashPos = path.indexOf(QLatin1Char('-'), 0); // don't match leading slash
    if ( slashPos <= 0 )
        return false;
    bool ok = false;
    trashId = path.mid( start, slashPos - start ).toInt( &ok );
    Q_ASSERT( ok );
    if ( !ok )
        return false;
    start = slashPos + 1;
    slashPos = path.indexOf(QLatin1Char('/'), start);
    if ( slashPos <= 0 ) {
        fileId = path.mid( start );
        relativePath.clear();
        return true;
    }
    fileId = path.mid( start, slashPos - start );
    relativePath = path.mid( slashPos + 1 );
    return true;
}

bool TrashImpl::adaptTrashSize( const QString& origPath, int trashId )
{
    KConfig config(QString::fromLatin1("ktrashrc"));

    const QString trashPath = trashDirectoryPath( trashId );
    KConfigGroup group = config.group( trashPath );

    bool useTimeLimit = group.readEntry( "UseTimeLimit", false );
    bool useSizeLimit = group.readEntry( "UseSizeLimit", true );
    double percent = group.readEntry( "Percent", 10.0 );
    int actionType = group.readEntry( "LimitReachedAction", 0 );

    if ( useTimeLimit ) { // delete all files in trash older than X days
        const int maxDays = group.readEntry( "Days", 7 );
        const QDateTime currentDate = QDateTime::currentDateTime();

        const TrashedFileInfoList trashedFiles = list();
        for ( int i = 0; i < trashedFiles.count(); ++i ) {
            struct TrashedFileInfo info = trashedFiles.at( i );
            if ( info.trashId != trashId )
                continue;

            if ( info.deletionDate.daysTo( currentDate ) > maxDays )
              del( info.trashId, info.fileId );
        }

        return true;
    }

    if ( useSizeLimit ) { // check if size limit exceeded

        // calculate size of the files to be put into the trash
        qulonglong additionalSize = DiscSpaceUtil::sizeOfPath( origPath );

        DiscSpaceUtil util(trashPath + QString::fromLatin1("/files/"));
        if ( util.usage( additionalSize ) >= percent ) {
            if ( actionType == 0 ) { // warn the user only
                m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
                m_lastErrorMessage = i18n( "The trash has reached its maximum size!\nCleanup the trash manually." );
                return false;
            } else {
                // before we start to remove any files from the trash,
                // check whether the new file will fit into the trash
                // at all...

                qulonglong partitionSize = util.size();

                if ( (((double)additionalSize/(double)partitionSize)*100) >= percent ) {
                    m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
                    m_lastErrorMessage = i18n( "The file is too large to be trashed." );
                    return false;
                }

                // the size of the to be trashed file is ok, so lets start removing
                // some other files from the trash

                QDir dir(trashPath + QString::fromLatin1("/files"));
                QFileInfoList infos;
                if ( actionType == 1 )  // delete oldest files first
                    infos = dir.entryInfoList( QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed );
                else if ( actionType == 2 ) // delete biggest files first
                    infos = dir.entryInfoList( QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Size );
                else
                    qWarning( "Should never happen!" );

                bool deleteFurther = true;
                for ( int i = 0; (i < infos.count()) && deleteFurther; ++i ) {
                    const QFileInfo info = infos.at( i );

                    del( trashId, info.fileName() ); // delete trashed file

                    if ( util.usage( additionalSize ) < percent ) // check whether we have enough space now
                         deleteFurther = false;
                }
            }
        }
    }

    return true;
}

#include "trashimpl.moc"
