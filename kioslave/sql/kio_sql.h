/*
   This file is part of the KDB libraries
   Copyright (c) 2000 Praduroux Alessandro <pradu@thekompany.com>
 
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     
#ifndef KIO_SQL_H
#define KIO_SQL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kio/slavebase.h>
#include <kio/global.h>
#include <qstring.h>

namespace KDB {

class Connection;
class Plugin;
class Database;
class Table;
class Query;

};

class KDBProtocol : public KIO::SlaveBase {

public:
    KDBProtocol( const QCString &pool, const QCString &app);
    virtual ~KDBProtocol();

    void setHost(const QString& host, int ip, const QString& user, const QString& pass);

    void get( const KURL& url );

    void stat( const KURL& url );
    void listDir( const KURL& url );
    void del( const KURL& url, bool isfile);

private:

    void createUDSEntry(QString path, QString filename, KIO::UDSEntry &);
    void createUDSEntry(int fileType, QString filename, KIO::UDSEntry &);

    enum { NO_TYPE, CONNECTION, DATABASE, TABLE, QUERY };

    int fileType(QString filename);

    QString m_host;
    int m_port;
    QString m_user;
    QString m_pwd;
    QString m_key;

};

#endif

