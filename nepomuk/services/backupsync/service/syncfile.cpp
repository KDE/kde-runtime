/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "syncfile.h"
#include "changelog.h"
#include "identificationset.h"

#include <QtCore/QFile>
#include <QtCore/QMutableListIterator>

#include <KTar>
#include <KTempDir>
#include <KDebug>
#include <Nepomuk/ResourceManager>


class Nepomuk::SyncFile::Private {
public :
    ChangeLog m_changeLog;
    IdentificationSet m_identificationSet;
};


Nepomuk::SyncFile::SyncFile()
    : d( new Nepomuk::SyncFile::Private() )
{
}


Nepomuk::SyncFile::SyncFile(const Nepomuk::SyncFile& rhs)
    : d( new Nepomuk::SyncFile::Private() )
{
    this->operator=( rhs );
}


Nepomuk::SyncFile::~SyncFile()
{
    delete d;
}


Nepomuk::SyncFile& Nepomuk::SyncFile::operator=(const Nepomuk::SyncFile& rhs)
{
    // trueg: why not use QSharedData if the data can be copied this easily anyway?
    // however, in that case the non-const methods below returning refs will call d->detach
    (*d) = (*rhs.d);
    return *this;
}


Nepomuk::SyncFile::SyncFile(const QUrl& url)
: d( new Nepomuk::SyncFile::Private() )
{
    load( url );
}


Nepomuk::SyncFile::SyncFile(const Nepomuk::ChangeLog& log, Soprano::Model* model)
: d( new Nepomuk::SyncFile::Private() )
{
    d->m_changeLog = log;
    d->m_identificationSet = IdentificationSet::fromChangeLog( log, model );
}


Nepomuk::SyncFile::SyncFile(const Nepomuk::ChangeLog& log, const Nepomuk::IdentificationSet& ident)
: d( new Nepomuk::SyncFile::Private() )
{
    d->m_changeLog = log;
    d->m_identificationSet = ident;
}


Nepomuk::SyncFile::SyncFile(const QList<Soprano::Statement>& stList, Soprano::Model* model)
: d( new Nepomuk::SyncFile::Private() )
{
    d->m_changeLog = ChangeLog::fromList( stList );
    d->m_identificationSet = IdentificationSet::fromChangeLog( d->m_changeLog, model );
}


bool Nepomuk::SyncFile::load(const QUrl& changeLogUrl, const QUrl& identFileUrl)
{
    d->m_identificationSet = IdentificationSet::fromUrl( identFileUrl );
    d->m_changeLog = ChangeLog::fromUrl( changeLogUrl );
    return true;
}


bool Nepomuk::SyncFile::load(const QUrl& syncFile)
{
    KTar tarFile( syncFile.toString(), QString::fromLatin1("application/x-gzip") );
    if( !tarFile.open( QIODevice::ReadOnly ) ) {
        kWarning() << "File could not be opened : " << syncFile.path();
        return false;
    }

    // trueg: no need to check (dir != 0)?
    const KArchiveDirectory * dir = tarFile.directory();
    Q_ASSERT(dir);
    KTempDir tempDir;
    dir->copyTo( tempDir.name() );

    QUrl logFileUrl( tempDir.name() + "changelog" );
    QUrl identFileUrl( tempDir.name() + "identificationset" );

    return load( logFileUrl, identFileUrl );
}


bool Nepomuk::SyncFile::save( const QUrl& outFile )
{
    KTempDir tempDir;

    QUrl logFileUrl( tempDir.name() + "changelog" );
    d->m_changeLog.save( logFileUrl );

    QUrl identFileUrl( tempDir.name() + "identificationset" );
    d->m_identificationSet.save( identFileUrl );

    KTar tarFile( outFile.toString(), QString::fromLatin1("application/x-gzip") );
    if( !tarFile.open( QIODevice::WriteOnly ) ) {
        kWarning() << "File could not be opened : " << outFile.path();
        return false;
    }

    tarFile.addLocalFile( logFileUrl.path(), "changelog" );
    tarFile.addLocalFile( identFileUrl.path(), "identificationset" );

    return true;
}


Nepomuk::ChangeLog& Nepomuk::SyncFile::changeLog()
{
    return d->m_changeLog;
}


Nepomuk::IdentificationSet& Nepomuk::SyncFile::identificationSet()
{
    return d->m_identificationSet;
}

const Nepomuk::ChangeLog& Nepomuk::SyncFile::changeLog() const
{
    return d->m_changeLog;
}

const Nepomuk::IdentificationSet& Nepomuk::SyncFile::identificationSet() const
{
    return d->m_identificationSet;
}
