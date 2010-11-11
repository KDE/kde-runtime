/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
        IdentifierWidget( QWidget* parent = 0 );

    private slots:
        void t();

        void ignoreSubTree();
        void identify( const QUrl & uri );
        void ignore( const QUrl & uri );
        
    private slots:
        void notIdentified( int id, const QString & string );
        
    private :
        IdentifierModel * m_model;

        /*QTreeView * m_treeView;
        
        QPushButton * m_button;
        
        QPushButton * m_ignore;
        QPushButton * m_ignoreSubTree;
        QPushButton * m_identify;*/
        
        typedef OrgKdeNepomukServicesNepomukbackupsyncIdentifierInterface Identifier;
        Identifier * m_identifier;
    };

}

#endif // IDENTIFIERWIDGET_H
