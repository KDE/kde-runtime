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

#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NIE>
#include <Soprano/Vocabulary/RDF>

#include <KDebug>

TestWidget::TestWidget(QWidget* parent, Qt::WindowFlags f)
: QWidget( parent )
{
    m_resNum = 0;
    m_model = new Nepomuk::IdentifierModel( this );
    QTreeView * view = new QTreeView( this );
    m_delegate = new MergeConflictDelegate( view, this );

    view->setModel( m_model );
    view->setItemDelegate( m_delegate );
    
    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( view );

    QPushButton * button = new QPushButton( "Test", this );
    layout->addWidget( button );

    connect( button, SIGNAL(clicked(bool)), this, SLOT(slotOnButtonClick()) );
}

TestWidget::~TestWidget()
{
}

void TestWidget::notIdentified(const QString& resUri, const QString& nieUrl)
{
    Soprano::Statement st( Soprano::Node( QUrl(resUri) ),
                           Soprano::Node( Nepomuk::Vocabulary::NIE::url() ),
                           Soprano::Node( QUrl(nieUrl) ) );
    QList<Soprano::Statement> stList;
    stList.append( st );
    if( nieUrl.endsWith('/') ) {
        stList << Soprano::Statement( QUrl(resUri), Soprano::Vocabulary::RDF::type(), Nepomuk::Vocabulary::NFO::Folder() );
    }
    else {
        stList << Soprano::Statement( QUrl(resUri), Soprano::Vocabulary::RDF::type(), Nepomuk::Vocabulary::NFO::FileDataObject() );
    }
    
    m_model->notIdentified( stList );
}

void TestWidget::notIdentified(const QString& nieUrl)
{
    notIdentified( QString("nepomuk:/res/") + QString::number( m_resNum ), nieUrl );
    m_resNum++;
}


void TestWidget::slotOnButtonClick()
{
    shouldCrash();
//     notIdentified( "nepomuk:/res/1", "/home/vishesh/" );
//     notIdentified( "nepomuk:/res/2", "/home/vishesh/file1" );
//     notIdentified( "nepomuk:/res/3", "/home/vishesh/folder/" );
//     notIdentified( "nepomuk:/res/4", "/home/vishesh/file2" );
//     notIdentified( "nepomuk:/res/5", "/home/vishesh/folder/fol-file1" );
//     notIdentified( "nepomuk:/res/6", "/home/vishesh/folder/fol-file2" );
// 
//     notIdentified( "nepomuk:/res/-1", "/home/user/" );
//     notIdentified( "nepomuk:/res/-2", "/home/user/file1" );
//     notIdentified( "nepomuk:/res/-3", "/home/user/folder/" );
//     notIdentified( "nepomuk:/res/-4", "/home/user/file2" );
//     notIdentified( "nepomuk:/res/-5", "/home/user/folder/fol-file1" );
//     notIdentified( "nepomuk:/res/-6", "/home/user/folder/fol-file2" );
// 
//     m_model->identified(0, "nepomuk:/res/-5", "nepomuk:/res/-5-identified" );
}

void TestWidget::shouldCrash()
{
    for( int i=0; i<20; i++ ) {
        QString num = QString::number( i );
        notIdentified( "nepomuk:/res/" + num, "/home/vishesh/wd/" + num );
    }


    notIdentified( "nepomuk:/res/1", "/home/vishesh/ewd/1" );
    m_model->identified( QUrl("nepomuk:/res/0"), QUrl("nepomuk:/ident/res/0") );
    //for( int i=0; i<5; i++ ) {
    //    QString num = QString::number( i );
    //    m_model->identified( "nepomuk:/res/" + num, "nepomuk:/ident/res/" + num );
    //}
}

