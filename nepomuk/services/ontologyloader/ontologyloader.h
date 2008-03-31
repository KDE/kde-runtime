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

#ifndef _NEPOMUK_SERVER_ONTOLOGY_LOADER_H_
#define _NEPOMUK_SERVER_ONTOLOGY_LOADER_H_

#include <Nepomuk/Service>
#include <QtCore/QUrl>

namespace Soprano {
    class Model;
}


namespace Nepomuk {
    class OntologyLoader : public Nepomuk::Service
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.OntologyManager" )

    public:
        OntologyLoader( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~OntologyLoader();

    public Q_SLOTS:
        /**
         * Update all installed ontologies and install dir watches
         * to monitor newly installed and changed ontologies.
         *
         * This should also be called for initialization
         */
        Q_SCRIPTABLE void updateAllOntologies();

        // FIXME: add methods (exported on DBus) like:
        // void importOntology( const QUrl& url... )

    private Q_SLOTS:
        // a little async updating
        void updateNextOntology();

    private:
        class Private;
        Private* const d;
    };
}

#endif
