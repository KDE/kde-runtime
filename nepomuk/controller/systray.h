/* This file is part of the KDE Project
   Copyright (c) 2008-2011 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_STRIGI_SYSTRAY_H_
#define _NEPOMUK_STRIGI_SYSTRAY_H_

#include <KStatusNotifierItem>
#include "strigiserviceinterface.h"
#include "servicecontrol.h"

class KToggleAction;

namespace Nepomuk {

    class FileIndexer;
    class StatusWidget;

    class SystemTray : public KStatusNotifierItem
    {
        Q_OBJECT

    public:
        SystemTray( QObject* parent );
        ~SystemTray();

    private Q_SLOTS:
        void slotUpdateStrigiStatus();
        void slotConfigure();
        void slotSuspend( bool suspended );

        void slotActivateRequested();

    private:
        KToggleAction* m_suspendResumeAction;

        org::kde::nepomuk::Strigi* m_service;
        org::kde::nepomuk::ServiceControl* m_serviceControl;
        bool m_suspendedManually;

        StatusWidget* m_statusWidget;

        // used to prevent endless status updates
        QString m_prevStatus;
    };
}

#endif
