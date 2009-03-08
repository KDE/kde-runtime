/***************************************************************************
                          sftpfileattr.cpp  -  description
                             -------------------
    begin                : Sat Jun 30 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@iastate.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "sftpfileattr.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <kio/global.h>
#include <kremoteencoding.h>
#include <kio/udsentry.h>

using namespace KIO;

sftpFileAttr::sftpFileAttr(){
    clear();
    mDirAttrs = false;
}

sftpFileAttr::sftpFileAttr(KRemoteEncoding* encoding){
    clear();
    mDirAttrs = false;
    mEncoding = encoding;
}


/** Constructor to initialize the file attributes on declaration. */
sftpFileAttr::sftpFileAttr(quint64 size, uid_t uid, gid_t gid,
                    mode_t permissions, time_t atime,
                    time_t mtime, quint32 extendedCount) {
    clear();
    mDirAttrs = false;
    mSize  = size;
    mUid   = uid;
    mGid   = gid;
    mAtime = atime;
    mMtime = mtime;
    mPermissions   = permissions;
    mExtendedCount = extendedCount;
}

sftpFileAttr::~sftpFileAttr(){
}

/** Returns a UDSEntry describing the file.
The UDSEntry is generated from the sftp file attributes. */
UDSEntry sftpFileAttr::entry() {
    UDSEntry entry;

    entry.insert(KIO::UDSEntry::UDS_NAME, mFilename );

    if( mFlags & SSH2_FILEXFER_ATTR_SIZE ) {
        entry.insert(KIO::UDSEntry::UDS_SIZE, mSize );
    }

    if( mFlags & SSH2_FILEXFER_ATTR_ACMODTIME ) {
        entry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, mAtime );

        entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, mMtime );
    }

    if( mFlags & SSH2_FILEXFER_ATTR_UIDGID ) {
        if( mUserName.isEmpty() || mGroupName.isEmpty() )
            getUserGroupNames();

        entry.insert(UDSEntry::UDS_USER, mUserName );

        entry.insert(UDSEntry::UDS_GROUP, mGroupName );
    }

    if( mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS ) {
        entry.insert(UDSEntry::UDS_ACCESS, mPermissions );

        mode_t type = fileType();

        // Set the type if we know what it is
        if( type != 0 ) {
            entry.insert(UDSEntry::UDS_FILE_TYPE, (mLinkType ? mLinkType:type) );
        }

        if( S_ISLNK(type) ) {
            entry.insert(UDSEntry::UDS_LINK_DEST, mLinkDestination );
        }
    }

    return entry;
}

/** Use to output the file attributes to a sftp packet */
QDataStream& operator<< (QDataStream& s, const sftpFileAttr& fa) {
    s << (quint32)fa.mFlags;

    if( fa.mFlags & SSH2_FILEXFER_ATTR_SIZE )
        { s << (quint64)fa.mSize; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_UIDGID )
        { s << (quint32)fa.mUid << (quint32)fa.mGid; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS )
        { s << (quint32)fa.mPermissions; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_ACMODTIME )
        { s << (quint32)fa.mAtime << (quint32)fa.mMtime; }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_EXTENDED ) {
        s << (quint32)fa.mExtendedCount;
        // XXX: Write extensions to data stream here
        // s.writeBytes(extendedtype).writeBytes(extendeddata);
    }
    return s;
}


/** Use to read a file attribute from a sftp packet */
QDataStream& operator>> (QDataStream& s, sftpFileAttr& fa) {

    // XXX Add some error checking in here in case
    //     we get a bad sftp packet.

    fa.clear();

    if( fa.mDirAttrs ) {
        QByteArray fn;
        s >> fn;
        fn.truncate( fn.size() );

        fa.mFilename = fa.mEncoding->decode( fn );

        s >> fa.mLongname;
        fa.mLongname.truncate( fa.mLongname.size() );
        //kDebug() << ">>: ftpfileattr long filename (" << fa.mLongname.size() << ")= " << fa.mLongname;
    }

    s >> fa.mFlags;  // get flags

    if( fa.mFlags & SSH2_FILEXFER_ATTR_SIZE ) {
        quint64 fileSize;
        s >> fileSize;
        fa.setFileSize(fileSize);
    }

    quint32 x;

    if( fa.mFlags & SSH2_FILEXFER_ATTR_UIDGID ) {
        s >> x; fa.setUid(x);
        s >> x; fa.setGid(x);
    }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS ) {
        s >> x; fa.setPermissions(x);
    }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_ACMODTIME ) {
        s >> x; fa.setAtime(x);
        s >> x; fa.setMtime(x);
    }

    if( fa.mFlags & SSH2_FILEXFER_ATTR_EXTENDED ) {
        s >> x; fa.setExtendedCount(x);
        // XXX: Read in extensions from data stream here
        // s.readBytes(extendedtype).readBytes(extendeddata);
    }

    fa.getUserGroupNames();
    return s;
}
/** Parse longname for the owner and group names. */
void sftpFileAttr::getUserGroupNames(){
    // Get the name of the owner and group of the file from longname.
    QString user, group;
    if( mLongname.isEmpty() ) {
        // do not have the user name so use the user id instead
        user.setNum(mUid);
        group.setNum(mGid);
    }
    else {
        int field = 0;
        int i = 0;
        int l = mLongname.length();

        QString longName = mEncoding->decode( mLongname );

        kDebug(7120) << "Decoded:  " << longName;

        // Find the beginning of the third field which contains the user name.
        while( field != 2 ) {
            if( longName[i].isSpace() ) {
                field++; i++;
                while( i < l && longName[i].isSpace() ) { i++; }
            }
            else { i++; }
        }
        // i is the index of the first character of the third field.
        while( i < l && !longName[i].isSpace() ) {
            user.append(longName[i]);
            i++;
        }

        // i is the first character of the space between fields 3 and 4
        // user contains the owner's user name
        while( i < l && longName[i].isSpace() ) {
            i++;
        }

        // i is the first character of the fourth field
        while( i < l && !longName[i].isSpace() ) {
            group.append(longName[i]);
            i++;
        }
        // group contains the name of the group.
    }

    mUserName = user;
    mGroupName = group;
}

/** No descriptions */
QDebug operator<<(QDebug s, sftpFileAttr& a) {
    s << "Filename: " << a.mFilename
      << ", Uid: " << a.mUid
      << ", Gid: " << a.mGid
      << ", Username: " << a.mUserName
      << ", GroupName: " << a.mGroupName
      << ", Permissions: " << a.mPermissions
      << ", size: " << a.mSize
      << ", atime: " << a.mAtime
      << ", mtime: " << a.mMtime
      << ", extended cnt: " << a.mExtendedCount;

    if (S_ISLNK(a.mLinkType)) {
      s << ", Link Type: " << a.mLinkType;
      s << ", Link Destination: " << a.mLinkDestination;
    }

    return s;
}

/** Clear all attributes and flags. */
void sftpFileAttr::clear(){
    clearAtime();
    clearMtime();
    clearGid();
    clearUid();
    clearFileSize();
    clearPermissions();
    clearExtensions();
    mFilename.clear();
    mGroupName =  QString();
    mUserName.clear();
    mLinkDestination.clear();
    mFlags = 0;
    mLongname = "\0";
    mLinkType = 0;
}

/** Return the size of the sftp attribute. */
quint32 sftpFileAttr::size() const{
    quint32 size = 4; // for the attr flag
    if( mFlags & SSH2_FILEXFER_ATTR_SIZE )
        size += 8;

    if( mFlags & SSH2_FILEXFER_ATTR_UIDGID )
        size += 8;

    if( mFlags & SSH2_FILEXFER_ATTR_PERMISSIONS )
        size += 4;

    if( mFlags & SSH2_FILEXFER_ATTR_ACMODTIME )
        size += 8;

    if( mFlags & SSH2_FILEXFER_ATTR_EXTENDED ) {
        size += 4;
        // add size of extensions
    }
    return size;
}

/** Returns the file type as determined from the file permissions */
mode_t sftpFileAttr::fileType() const{
    mode_t type = 0;

    if( S_ISLNK(mPermissions) )
      type |= S_IFLNK;

    if( S_ISREG(mPermissions) )
      type |= S_IFREG;
    else if( S_ISDIR(mPermissions) )
      type |= S_IFDIR;
    else if( S_ISCHR(mPermissions) )
      type |= S_IFCHR;
    else if( S_ISBLK(mPermissions) )
      type |= S_IFBLK;
    else if( S_ISFIFO(mPermissions) )
      type |= S_IFIFO;
    else if( S_ISSOCK(mPermissions) )
      type |= S_IFSOCK;

    return type;
}

void sftpFileAttr::setEncoding( KRemoteEncoding* encoding )
{
    mEncoding = encoding;
}
// vim:ts=4:sw=4
