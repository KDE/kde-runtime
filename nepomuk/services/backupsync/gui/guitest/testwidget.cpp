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


#include "testwidget.h"

#include <QtGui/QBoxLayout>

#include <KDebug>

TestWidget::TestWidget(QWidget* parent, Qt::WindowFlags f)
: QWidget( parent )
{
    m_model = new Nepomuk::IdentifierModel( this );
    QTreeView * view = new QTreeView( this );
    view->setModel( m_model );

    m_delegate = new MergeConflictDelegate( view, this );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( view );

    QPushButton * button = new QPushButton( "Test", this );
    layout->addWidget( button );

    connect( button, SIGNAL(clicked(bool)), this, SLOT(slotOnButtonClick()) );
}

TestWidget::~TestWidget()
{
}


void TestWidget::slotOnButtonClick()
{

    m_model->debug_notIdentified( "nepomuk:/res/1", "/home/vishesh/" );
    m_model->debug_notIdentified( "nepomuk:/res/2", "/home/vishesh/file1" );
    m_model->debug_notIdentified( "nepomuk:/res/3", "/home/vishesh/folder/" );
    m_model->debug_notIdentified( "nepomuk:/res/4", "/home/vishesh/file2" );
    m_model->debug_notIdentified( "nepomuk:/res/5", "/home/vishesh/folder/fol-file1" );
    m_model->debug_notIdentified( "nepomuk:/res/6", "/home/vishesh/folder/fol-file2" );
}

