/*****************************************************************************
 *   Copyright (C) 2009 by Shaun Reich <shaun.reich@kdemail.net>             *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or           *
 *   modify it under the terms of the GNU General Public License as          *
 *   published by the Free Software Foundation; either version 2 of          *
 *   the License, or (at your option) any later version.                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#include <QDBusPendingCallWatcher>
#include "requestviewcallwatcher.h"
#include "jobview.h"

RequestViewCallWatcher::RequestViewCallWatcher(JobView* jobView, const QString& service,
        const QDBusPendingCall& call, QObject* parent)
        : QDBusPendingCallWatcher(call, parent),
        m_jobView(jobView),
        m_service(service)
{
    connect(this, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(slotFinished()));
}


#include "requestviewcallwatcher.moc"
