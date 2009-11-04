/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
 */

#ifndef ATTICA_POSTJOB_H
#define ATTICA_POSTJOB_H

#include <QtNetwork/QNetworkRequest>

#include "atticaclient_export.h"
#include "atticabasejob.h"

// workaround to get initialization working with gcc < 4.4
typedef QMap<QString, QString> StringMap;

namespace Attica {
    class Provider;

class ATTICA_EXPORT PostJob : public BaseJob
{
    Q_OBJECT

protected:
    PostJob(const QSharedPointer<Internals>& internals, const QNetworkRequest& request, QIODevice* data);
    PostJob(const QSharedPointer<Internals>& internals, const QNetworkRequest& request, const StringMap& parameters = StringMap());
    PostJob(const QSharedPointer<Internals>& internals, const QNetworkRequest& request, const QByteArray& byteArray);

private:
    virtual QNetworkReply* executeRequest();
    virtual void parse(const QString&);

    QIODevice* m_ioDevice;
    QByteArray m_byteArray;
    
    QString m_responseData;
    const QNetworkRequest m_request;
  
    QString m_status;
    QString m_statusMessage;

    friend class Attica::Provider;
};

}


#endif
