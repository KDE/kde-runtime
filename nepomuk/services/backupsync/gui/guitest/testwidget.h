/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QtGui/QTreeView>
#include <QtGui/QPushButton>

#include "../identifiermodel.h"
#include "../mergeconflictdelegate.h"

class TestWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TestWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~TestWidget();
    
private:
    Nepomuk::IdentifierModel * m_model;
    MergeConflictDelegate * m_delegate;

    void notIdentified( const QString& resUri, const QString& nieUrl );
private slots:
    void slotOnButtonClick();
};

#endif // TESTWIDGET_H
