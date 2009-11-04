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
#ifndef ATTICA_ATTICABASEJOB_H
#define ATTICA_ATTICABASEJOB_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QNetworkAccessManager>

#include "atticaclient_export.h"
#include "metadata.h"

class QNetworkReply;
class QIODevice;

namespace Attica {
    class Internals;

class ATTICA_EXPORT BaseJob : public QObject
{
    Q_OBJECT

public:
    virtual ~BaseJob();

    Metadata metadata() const;
    void setMetadata(const Metadata& data) const;
    
public Q_SLOTS:
    void start();

Q_SIGNALS:
    void finished(Attica::BaseJob* job);

protected Q_SLOTS:
    void dataFinished();

protected:
    BaseJob(const QSharedPointer<Internals>& internals);

    virtual QNetworkReply* executeRequest() = 0;
    virtual void parse(const QString& xml) = 0;
    Internals* internals();
    void setError(int errorCode);
    void setErrorString(const QString& errorString);

private Q_SLOTS:
    void doWork();

private:
    BaseJob(const BaseJob& other);
    BaseJob& operator=(const BaseJob& other);

    class Private;
    Private* d;
};

}

#endif
