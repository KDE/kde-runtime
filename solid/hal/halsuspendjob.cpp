/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "halsuspendjob.h"

#include <QtDBus/QDBusMessage>
#include <QTimer>

HalSuspendJob::HalSuspendJob(QDBusInterface &powermanagement,
                              Solid::Control::PowerManager::SuspendMethod method,
                              Solid::Control::PowerManager::SuspendMethods supported)
    : KJob(), m_halPowerManagement(powermanagement)
{
    if (supported  & method)
    {
        switch(method)
        {
        case Solid::Control::PowerManager::ToRam:
            m_dbusMethod = "Suspend";
            m_dbusParam = 0;
            break;
        case Solid::Control::PowerManager::ToDisk:
            m_dbusMethod = "Hibernate";
            m_dbusParam = -1;
            break;
        default:
            break;
        }
    }
}

HalSuspendJob::~HalSuspendJob()
{

}

void HalSuspendJob::start()
{
    QTimer::singleShot(0, this, SLOT(doStart()));
}

void HalSuspendJob::kill(bool /*quietly */)
{

}

void HalSuspendJob::doStart()
{
    if (m_dbusMethod.isEmpty())
    {
        setError(1);
        setErrorText("Unsupported suspend method");
        emitResult();
        return;
    }

    QList<QVariant> args;

    if (m_dbusParam>=0)
    {
        args << m_dbusParam;
    }

    if (!m_halPowerManagement.callWithCallback(m_dbusMethod, args,
                                                this, SLOT(resumeDone(const QDBusMessage &))))
    {
        setError(1);
        setErrorText(m_halPowerManagement.lastError().name()+": "
                     +m_halPowerManagement.lastError().message());
        emitResult();
    }
}


void HalSuspendJob::resumeDone(const QDBusMessage &reply)
{
    if (reply.type() == QDBusMessage::ErrorMessage)
    {
        // We ignore the NoReply error, since we can timeout anyway
        // if the computer is suspended for too long.
        if (reply.errorName() != "org.freedesktop.DBus.Error.NoReply")
        {
            setError(1);
            setErrorText(reply.errorName()+": "+reply.arguments().at(0).toString());
        }
    }

    emitResult();
}

#include "halsuspendjob.moc"
