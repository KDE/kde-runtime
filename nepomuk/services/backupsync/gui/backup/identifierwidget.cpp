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


#include "identifierwidget.h"
#include "identifiermodel.h"
#include "identifiermodeltree.h"
#include "identifierinterface.h"
#include "../../service/dbusoperators.h"

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <KDebug>
#include <KFileDialog>

#include <Soprano/Parser>
#include <Soprano/PluginManager>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>


Nepomuk::IdentifierWidget::IdentifierWidget(QWidget* parent): QWidget(parent)
{
    registerMetaTypes();
    setupUi(this);
    
    m_model = new IdentifierModel( this );

    MergeConflictDelegate * delegate = new MergeConflictDelegate( m_viewConflicts, this );
    m_viewConflicts->setModel( m_model );
    m_viewConflicts->setItemDelegate( delegate );
    /*
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( label );
    layout->addWidget( m_treeView );

    setLayout( layout );*/

    //m_button = new QPushButton("test", this);
    //connect( m_button, SIGNAL(clicked(bool)), this , SLOT(t()) );
    //layout->addWidget( m_button );

    //View Options
    //m_treeView->setSelectionMode( QAbstractItemView::SingleSelection );
    //m_treeView->setSelectionBehavior( QAbstractItemView::SelectRows );
    //m_treeView->setHeaderHidden( true );
    //m_treeView->setExpandsOnDoubleClick( false );
/*
    m_ignore = new QPushButton( "Ignore", this );
    m_ignoreSubTree = new QPushButton( "Ignore Sub Tree", this );
    m_identify = new QPushButton( "Identify", this );

    QHBoxLayout * hBox = new QHBoxLayout();
    hBox->addWidget( m_ignore );
    hBox->addWidget( m_ignoreSubTree );
    hBox->addWidget( m_identify );
    layout->addLayout( hBox );

    connect( m_ignore, SIGNAL(clicked(bool)), this, SLOT(ignore()) );
    connect( m_ignoreSubTree, SIGNAL(clicked(bool)), this, SLOT(ignoreSubTree()) );
    connect( m_identify, SIGNAL(clicked(bool)), this,SLOT(identify()) );*/

    m_identifier = new Identifier( QLatin1String("org.kde.nepomuk.services.nepomukbackupsync"),
                                   QLatin1String("/identifier"),
                                   QDBusConnection::sessionBus(), this );
    
    if( m_identifier->isValid() ) {
        kDebug() << "Valid : Connecting identified signals";
        Q_ASSERT(connect( m_identifier, SIGNAL(notIdentified(int,QString)),
                          this, SLOT(notIdentified(int,QString)) ));
        connect( m_identifier, SIGNAL(identified(int,QString,QString)),
                 m_model, SLOT(identified(int,QString,QString)) );
    }

    connect( delegate, SIGNAL(requestResourceResolve(QUrl)),
             this, SLOT(identify(QUrl)) );
    connect( delegate, SIGNAL(requestResourceDiscard(QUrl)),
             this, SLOT(ignore(QUrl)) );
}

void Nepomuk::IdentifierWidget::t()
{
    //kDebug() << m_model->rowCount();
    //m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/file0");
    //kDebug() << m_model->rowCount();

    //kDebug() <<"Inserting another";
    m_model->debug_notIdentified(0, "res2", "/home/user/", true);
    
    m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/file2");
    m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/file1");
    m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/file3");
    m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/file4");
    m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/ZAB", true);
    m_model->debug_notIdentified(0, "res1", "/home/user/nepomuk/ZAB/f1");
    //m_treeView->expandAll();
}

void Nepomuk::IdentifierWidget::ignore(const QUrl& uri)
{
    QModelIndex index = m_viewConflicts->currentIndex();
    if( !index.isValid() )
        return;

    IdentifierModelTreeItem * treeItem = static_cast<IdentifierModelTreeItem*>( index.internalPointer() );

    m_identifier->ignore( treeItem->id(), treeItem->resourceUri().toString(), false);
}

void Nepomuk::IdentifierWidget::ignoreSubTree()
{
//     QModelIndex index = m_treeView->currentIndex();
//     if( !index.isValid() )
//         return;
//     
//     IdentifierModelTreeItem * treeItem = static_cast<IdentifierModelTreeItem*>( index.internalPointer() );
// 
//     m_identifier->ignore( treeItem->id(), treeItem->resourceUri().toString(), true);
}

void Nepomuk::IdentifierWidget::identify(const QUrl& uri)
{
    QModelIndex index = m_viewConflicts->currentIndex();
    if( !index.isValid() )
        return;

    IdentifierModelTreeItem * treeItem = static_cast<IdentifierModelTreeItem*>( index.internalPointer() );

    KUrl newUrl;
    if( treeItem->isFolder() )
        newUrl = KFileDialog::getExistingDirectoryUrl();
    else
        newUrl = KFileDialog::getOpenUrl();

   if( !newUrl.isEmpty() )
        m_identifier->identify( treeItem->id(), treeItem->resourceUri().toString(), newUrl.url() );
}

void Nepomuk::IdentifierWidget::notIdentified(int id, const QString& string)
{
    kDebug() << string;
    const Soprano::Parser* parser = Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::SerializationNQuads );

    QList<Soprano::Statement> stList = parser->parseString( string, QUrl(), Soprano::SerializationNQuads ).allStatements();

    m_model->notIdentified( id, stList );
}

#include "identifierwidget.moc"
