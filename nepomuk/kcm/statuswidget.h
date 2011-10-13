/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _STATUS_WIDGET_H_
#define _STATUS_WIDGET_H_

#include <KDialog>
#include "ui_statuswidget.h"
#include "servicecontrol.h"
#include "fileindexerinterface.h"
#include <QtCore/QTimer>

class QShowEvent;
class QHideEvent;
class KJob;
class StrigiService;
class StatusWidget;

namespace Soprano {
namespace Util {
class AsyncQuery;
}
}

namespace Nepomuk {


class StatusWidget : public KDialog, public Ui::StatusWidget
{
    Q_OBJECT

public:
    StatusWidget( QWidget* parent = 0 );
    ~StatusWidget();

private Q_SLOTS:
    void slotUpdateStoreStatus();
    void slotFileCountFinished( Soprano::Util::AsyncQuery* query );
    void slotUpdateTimeout();
    void slotUpdateStatus();
    void slotSuspendResume();

private:
    void updateSuspendResumeButtonText(bool suspended);

    void showEvent( QShowEvent* event );
    void hideEvent( QHideEvent* event );

    bool m_connected;
    QTimer m_updateTimer;
    int m_updatingJobCnt;
    bool m_updateRequested;

    org::kde::nepomuk::FileIndexer* m_fileIndexerService;
    org::kde::nepomuk::ServiceControl* m_fileIndexerServiceControl;
};
}

#endif
