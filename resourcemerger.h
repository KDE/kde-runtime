/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

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


#ifndef NEPOMUK_DATAMANAGEMENT_RESOURCEMERGER_H
#define NEPOMUK_DATAMANAGEMENT_RESOURCEMERGER_H

#include <QtCore/QHash>
#include <QtCore/QVariant>

#include <nepomuk/resourcemerger.h>

namespace Soprano {
    class Node;
    class Statement;
}

namespace Nepomuk {
    class DataManagementModel;

    class ResourceMerger : public Sync::ResourceMerger
    {
    public:
        ResourceMerger( Nepomuk::DataManagementModel * model, const QString & app,
                        const QHash<QUrl, QVariant> & additionalMetadata );
        virtual ~ResourceMerger();

    protected:
        virtual KUrl createGraph();
        virtual void resolveDuplicate(const Soprano::Statement& newSt);
        virtual QUrl createResourceUri();
        virtual QUrl createGraphUri();
        virtual KUrl resolveUnidentifiedResource(const KUrl& uri);

    private:
        QHash<QString, QUrl> m_graphHash;
        QHash<QUrl, QVariant> m_additionalMetadata;

        QString m_app;
        QUrl m_graph;

        Nepomuk::DataManagementModel * m_model;

        QUrl mergeGraphs( const QUrl & oldGraph );
    };

}

#endif // NEPOMUK_DATAMANAGEMENT_RESOURCEMERGER_H
