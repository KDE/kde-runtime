/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

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


#ifndef IDENTIFIERWIDGET_H
#define IDENTIFIERWIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QPushButton>

#include "mergeconflictdelegate.h"
#include "ui_mergeconflictwidget.h"

class OrgKdeNepomukServicesNepomukbackupsyncIdentifierInterface;

namespace Nepomuk {

    class IdentifierModel;

    class IdentifierWidget : public QWidget, public Ui::MergeConflictWidget
    {
        Q_OBJECT
    public :
        IdentifierWidget( int id, QWidget* parent = 0 );

    private slots:
        void identify( const QUrl & uri );
        void ignore( const QUrl & uri );
        
    private slots:
        void notIdentified( int id, const QString & string );
        void identified( int id, const QString & oldUri, const QString & newUri );
        
    private :
        IdentifierModel * m_model;
        int m_id;

        typedef OrgKdeNepomukServicesNepomukbackupsyncIdentifierInterface Identifier;
        Identifier * m_identifier;
    };

}

#endif // IDENTIFIERWIDGET_H
