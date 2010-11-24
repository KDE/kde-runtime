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


#include "identifierwidget.h"
#include "identifiermodel.h"
#include "identifiermodeltree.h"
#include "identifierinterface.h"
#include "../service/dbusoperators.h"

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <KDebug>
#include <KFileDialog>

#include <Soprano/Parser>
#include <Soprano/PluginManager>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>


Nepomuk::IdentifierWidget::IdentifierWidget(int id, QWidget* parent): QWidget(parent), m_id(id)
{
    registerMetaTypes();
    setupUi(this);
    
    m_model = new IdentifierModel( this );

    MergeConflictDelegate * delegate = new MergeConflictDelegate( m_viewConflicts, this );
    m_viewConflicts->setModel( m_model );
    m_viewConflicts->setItemDelegate( delegate );

    m_identifier = new Identifier( QLatin1String("org.kde.nepomuk.services.nepomukbackupsync"),
                                   QLatin1String("/identifier"),
                                   QDBusConnection::sessionBus(), this );
    
    if( m_identifier->isValid() ) {
        connect( m_identifier, SIGNAL(notIdentified(int,QString)),
                 this, SLOT(notIdentified(int,QString)) );
        connect( m_identifier, SIGNAL(identified(int,QString,QString)),
                 this, SLOT(identified(int,QString,QString)) );
    }

    connect( delegate, SIGNAL(requestResourceResolve(QUrl)),
             this, SLOT(identify(QUrl)) );
    connect( delegate, SIGNAL(requestResourceDiscard(QUrl)),
             this, SLOT(ignore(QUrl)) );
}

void Nepomuk::IdentifierWidget::ignore(const QUrl& uri)
{
    QModelIndex index = m_viewConflicts->currentIndex();
    if( !index.isValid() )
        return;

    IdentifierModelTreeItem * item = static_cast<IdentifierModelTreeItem*>( index.internalPointer() );
    item->setDiscarded( true );
    
    m_identifier->ignore( m_id, item->resourceUri().toString(), true);
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
        m_identifier->identify( m_id, treeItem->resourceUri().toString(), newUrl.url() );
}

void Nepomuk::IdentifierWidget::notIdentified(int id, const QString& string)
{
    if( id != m_id )
        return;
    
    kDebug() << string;
    const Soprano::Parser* parser = Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::SerializationNQuads );

    QList<Soprano::Statement> stList = parser->parseString( string, QUrl(), Soprano::SerializationNQuads ).allStatements();

    m_model->notIdentified( stList );
}

void Nepomuk::IdentifierWidget::identified(int id, const QString& oldUri, const QString& newUri)
{
    if( id != m_id )
        return;

    m_model->identified( QUrl(oldUri), QUrl(newUri) );
}

#include "identifierwidget.moc"
