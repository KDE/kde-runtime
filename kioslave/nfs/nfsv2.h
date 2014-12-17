/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>,
        2014 Mathias Tillman <master.homer@gmail.com>

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

#ifndef KIO_NFSV2_H
#define KIO_NFSV2_H

#include "kio_nfs.h"

#define PORTMAP  //this seems to be required to compile on Solaris
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

class NFSProtocolV2 : public NFSProtocol
{
public:
    NFSProtocolV2(NFSSlave* slave);
    ~NFSProtocolV2();

    bool isCompatible(bool& connectionError);
    bool isConnected() const;

    void openConnection();
    void closeConnection();

    void setHost(const QString& host);

    void put(const KUrl& url, int _mode, KIO::JobFlags _flags);
    void get(const KUrl& url);
    void listDir(const KUrl& url);
    void symlink(const QString& target, const KUrl& dest, KIO::JobFlags);
    void stat(const KUrl& url);
    void mkdir(const KUrl& url, int permissions);
    void del(const KUrl& url, bool isfile);
    void chmod(const KUrl& url, int permissions);
    void rename(const KUrl& src, const KUrl& dest, KIO::JobFlags flags);

protected:
    void copySame(const KUrl& src, const KUrl& dest, int mode, KIO::JobFlags flags);
    void copyFrom(const KUrl& src, const KUrl& dest, int mode, KIO::JobFlags flags);
    void copyTo(const KUrl& src, const KUrl& dest, int mode, KIO::JobFlags flags);

    NFSFileHandle lookupFileHandle(const QString& path);

private:
    bool create(const QString& path, int mode, int& rpcStatus, diropres& result);

    bool getAttr(const QString& path, int& rpcStatus, attrstat& result);

    bool lookupHandle(const QString& path, int& rpcStatus, diropres& result);

    bool readLink(const QString& path, int& rpcStatus, readlinkres& result, char* dataBuffer);

    // Calls @remove, but with dummy rpcStatus and result arguments
    bool remove(const QString& path);
    bool remove(const QString& path, int& rpcStatus, nfsstat& result);

    // Calls @rename, but with dummy rpcStatus and result arguments
    bool rename(const QString& src, const QString& dest);
    bool rename(const QString& src, const QString& dest, int& rpcStatus, nfsstat& result);

    bool setAttr(const QString& path, const sattr& attributes, int& rpcStatus, nfsstat& result);

    bool symLink(const QString& target, const QString& dest, int& rpcStatus, nfsstat& result);

    // UDS helper functions
    void completeUDSEntry(KIO::UDSEntry& entry, const fattr& attributes);
    void completeBadLinkUDSEntry(KIO::UDSEntry& entry, const fattr& attributes);

    NFSSlave* m_slave;

    QString m_currentHost;

    CLIENT* m_mountClient;
    int m_mountSock;
    CLIENT* m_nfsClient;
    int m_nfsSock;

    timeval clnt_timeout;

    QHash<long, QString> m_usercache;
    QHash<long, QString> m_groupcache;
};

#endif
