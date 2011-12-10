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

#ifndef QNETWORKREPLY_P_H
#define QNETWORKREPLY_P_H

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

#include "qnetworkrequest.h"
#include "qnetworkrequest_p.h"
#include "qnetworkreply.h"
#include "QtCore/qpointer.h"
#include "private/qiodevice_p.h"

QT_BEGIN_NAMESPACE

class QNetworkReplyPrivate: public QIODevicePrivate, public QNetworkHeadersPrivate
{
public:
    QNetworkReplyPrivate();
    QNetworkRequest request;
    QUrl url;
    QPointer<QNetworkAccessManager> manager;
    qint64 readBufferMaxSize;
    QNetworkAccessManager::Operation operation;
    QNetworkReply::NetworkError errorCode;

    static inline void setManager(QNetworkReply *reply, QNetworkAccessManager *manager)
    { reply->d_func()->manager = manager; }

    virtual bool isFinished() const { return false; }

    Q_DECLARE_PUBLIC(QNetworkReply)
};

QT_END_NAMESPACE

#endif
