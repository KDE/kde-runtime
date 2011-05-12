/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#include "datamanagementcommand.h"
#include "datamanagementmodel.h"

#include <Soprano/Error/Error>
#include <Soprano/Error/ErrorCode>

#include <QtDBus/QDBusConnection>

#include <QtCore/QStringList>

#include <KUrl>


namespace {
QDBusError::ErrorType convertSopranoErrorCode(int code)
{
    switch(code) {
    case Soprano::Error::ErrorInvalidArgument:
        return QDBusError::InvalidArgs;
    default:
        return QDBusError::Failed;
    }
}
}


Nepomuk::DataManagementCommand::DataManagementCommand(DataManagementModel* model, const QDBusMessage& msg)
    : QRunnable(),
      m_model(model),
      m_msg(msg)
{
}

Nepomuk::DataManagementCommand::~DataManagementCommand()
{
}

void Nepomuk::DataManagementCommand::run()
{
    QVariant result = runCommand();
    Soprano::Error::Error error = model()->lastError();
    if(error) {
        // send error reply
        QDBusConnection::sessionBus().send(m_msg.createErrorReply(convertSopranoErrorCode(error.code()), error.message()));
    }
    else {
        // encode result (ie. convert QUrl to QString)
        if(result.isValid()) {
            if(result.type() == QVariant::Url) {
                result = encodeUrl(result.toUrl());
            }
            QDBusConnection::sessionBus().send(m_msg.createReply(result));
        }
        else {
            QDBusConnection::sessionBus().send(m_msg.createReply());
        }
    }
}


// static
QUrl Nepomuk::decodeUrl(const QString& urlsString)
{
    // we use the power of KUrl to automatically convert file paths to file:/ URLs
    return KUrl(urlsString);
}

// static
QList<QUrl> Nepomuk::decodeUrls(const QStringList& urlStrings)
{
    QList<QUrl> urls;
    Q_FOREACH(const QString& urlString, urlStrings) {
        urls << decodeUrl(urlString);
    }
    return urls;
}

// static
QString Nepomuk::encodeUrl(const QUrl& url)
{
    return QString::fromAscii(url.toEncoded());
}
