/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFILENETWORKREPLY_P_H
#define QFILENETWORKREPLY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"
#include <QFile>
#include <QAbstractFileEngine>

QT_BEGIN_NAMESPACE

 
class QFileNetworkReplyPrivate;
class QFileNetworkReply: public QNetworkReply
{
    Q_OBJECT
public:
    QFileNetworkReply(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op);
    ~QFileNetworkReply();
    virtual void abort();

    // reimplemented from QNetworkReply
    virtual void close();
    virtual qint64 bytesAvailable() const;
    virtual bool isSequential () const;
    qint64 size() const;

    virtual qint64 readData(char *data, qint64 maxlen);
    
    void setUrl(const QUrl &url){QNetworkReply::setUrl(url);}

    Q_DECLARE_PRIVATE(QFileNetworkReply)
};

class QFileNetworkReplyPrivate: public QNetworkReplyPrivate
{
public:
    QFileNetworkReplyPrivate();
    ~QFileNetworkReplyPrivate();

    QAbstractFileEngine *fileEngine;
    qint64 fileSize;
    qint64 filePos;

    virtual bool isFinished() const;

    Q_DECLARE_PUBLIC(QFileNetworkReply)
};

QT_END_NAMESPACE

#endif // QFILENETWORKREPLY_P_H
