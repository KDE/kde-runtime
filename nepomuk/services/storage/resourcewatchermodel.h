/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef RESOURCEWATCHERMODEL_H
#define RESOURCEWATCHERMODEL_H

#include <QtCore/QMultiHash>
#include <QtCore/QSet>

#include <Soprano/FilterModel>
#include <QtDBus/QDBusObjectPath>

namespace Nepomuk {

    class ResourceWatcherConnection;
    
    class ResourceWatcherModel : public Soprano::FilterModel
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.ResourceWatcher" )
        
    public:
        virtual Soprano::Error::ErrorCode addStatement(const Soprano::Statement& statement);
        virtual Soprano::Error::ErrorCode removeStatement(const Soprano::Statement& statement);
        virtual Soprano::Error::ErrorCode removeAllStatements(const Soprano::Statement& statement);

        ResourceWatcherModel( Soprano::Model* parent );

    public slots:
        Q_SCRIPTABLE QDBusObjectPath watch( const QList<QString> & resources,
                                            const QList<QString> & properties,
                                            const QList<QString> & types );
        Q_SCRIPTABLE void stopWatcher( const QString& objectName );

    private:
        QMultiHash<QUrl, ResourceWatcherConnection*> m_subHash;
        QMultiHash<QUrl, ResourceWatcherConnection*> m_propHash;
        QMultiHash<QUrl, ResourceWatcherConnection*> m_typeHash;
        QSet<QUrl> m_metaProperties; 
        
        friend class ResourceWatcherConnection;
    };

}

#endif // RESOURCEWATCHERMODEL_H
