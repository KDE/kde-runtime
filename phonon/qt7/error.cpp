/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "backendheader.h"

namespace Phonon
{
namespace QT7
{

Q_GLOBAL_STATIC(QString, gErrorString)
int gErrorType = NO_ERROR;

void gSetErrorString(const QString &errorString)
{
    *gErrorString() = errorString;   
    if (qgetenv("PHONON_MAC_DEBUG") == "1"){
        qDebug() << "Error:" << errorString;
    }
}

QString gGetErrorString()
{
    return *gErrorString();
}

void gSetErrorLocation(const QString &errorLocation)
{
    if (qgetenv("PHONON_MAC_DEBUG") == "1"){
        qDebug() << "Location:" << errorLocation;
    }
}

void gSetErrorType(int errorType)
{
    gErrorType = errorType;
}

int gGetErrorType()
{
    return gErrorType;
}

void gClearError()
{
    gErrorString()->clear();
    gErrorType = NO_ERROR;
}

}}
