/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_ONTOLOGY_UPDATE_JOB_H_
#define _NEPOMUK_ONTOLOGY_UPDATE_JOB_H_

#include <KJob>

class QUrl;
namespace Soprano {
    class Model;
    class StatementIterator;
}

class QDateTime;

namespace Nepomuk {
    class OntologyUpdateJob : public KJob
    {
        Q_OBJECT

    public:
        OntologyUpdateJob( Soprano::Model* mainModel, QObject* parent );
        ~OntologyUpdateJob();

        /**
         * Start the job.
         * Connect to result() to see if updating
         * the ontology was successful.
         */
        void start();

        /**
         * Set the ontology's base URI, aka the namespace.
         * If not set it will be determined from the input
         * data.
         */
        void setBaseUri( const QUrl& uri );

        Soprano::Model* model() const;

        static QDateTime ontoModificationDate( Soprano::Model* model, const QUrl& uri );
    
    protected:
        /**
         * Provides the actual ontology data as statements.
         */
        virtual Soprano::StatementIterator data() = 0;

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_slotFinished() )
    };
}

#endif
