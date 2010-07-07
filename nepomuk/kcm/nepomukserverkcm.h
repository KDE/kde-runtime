/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SERVER_KCM_H_
#define _NEPOMUK_SERVER_KCM_H_

#include <KCModule>
#include "ui_nepomukconfigwidget.h"
#include "nepomukserverinterface.h"
#include "strigiserviceinterface.h"

#include <Nepomuk/Query/Query>

class QRadioButton;
class QAbstractButton;

namespace Nepomuk {

    class IndexFolderSelectionDialog;
    class StatusWidget;

    class ServerConfigModule : public KCModule, private Ui::NepomukConfigWidget
    {
        Q_OBJECT

    public:
        ServerConfigModule( QWidget* parent, const QVariantList& args );
        ~ServerConfigModule();

    public Q_SLOTS:
        void load();
        void save();
        void defaults();

    private Q_SLOTS:
        void updateNepomukServerStatus();
        void updateStrigiStatus();
        void recreateInterfaces();
        void slotCustomQueryButtonClicked();
        void slotEditIndexFolders();
        void slotCustomQueryToggled( bool );
        void slotStatusDetailsClicked();

    private:
        QRadioButton* buttonForQuery( const Query::Query& query ) const;
        Nepomuk::Query::Query queryForButton( QAbstractButton* button ) const;

        bool m_nepomukAvailable;

        org::kde::NepomukServer* m_serverInterface;
        org::kde::nepomuk::Strigi* m_strigiInterface;

        IndexFolderSelectionDialog* m_indexFolderSelectionDialog;
        StatusWidget* m_statusDialog;

        bool m_failedToInitialize;

        QString m_customQuery;
    };
}

#endif
