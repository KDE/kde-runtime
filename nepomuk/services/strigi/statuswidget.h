/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include <QtCore/QTimer>

class QShowEvent;
class QHideEvent;
class KJob;

namespace Soprano {
    class Model;
}

namespace Nepomuk {

    class IndexScheduler;

    class StatusWidget : public KDialog, public Ui::StatusWidget
    {
        Q_OBJECT

    public:
        StatusWidget( Soprano::Model* model, IndexScheduler* scheduler, QWidget* parent = 0 );
        ~StatusWidget();

    private Q_SLOTS:
        void slotConfigure();
        void slotUpdateStrigiStatus();
        void slotUpdateStoreStatus();
        void slotStoreSizeCalculated( KJob* job );
        void slotFileCountFinished();
        void slotUpdateTimeout();

    private:
        void showEvent( QShowEvent* event );
        void hideEvent( QHideEvent* event );

        Soprano::Model* m_model;
        IndexScheduler* m_indexScheduler;

        bool m_connected;
        QTimer m_updateTimer;
        int m_updatingJobCnt;
        bool m_updateRequested;
    };
}

#endif
