/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef CRAPPYINFERENCER2_H
#define CRAPPYINFERENCER2_H

#include <Soprano/FilterModel>

/**
 * The second version of the crappy inferencer it even crappier than the first.
 * It only does rdf:type inferencing, nothing else. In the long run this is not
 * enough but for now it is the only kind of inference we use in Nepomuk.
 * Thus, it is enough and makes for much faster queries than the first
 * crappy inferencer.
 *
 * \author Sebastian Trueg <trueg@kde.org>
 */
class CrappyInferencer2 : public Soprano::FilterModel
{
    Q_OBJECT

public:
    CrappyInferencer2(Soprano::Model* parent = 0);
    ~CrappyInferencer2();

    /**
     * \reimpl to update internal structures
     */
    void setParentModel( Soprano::Model* model );

    Soprano::Error::ErrorCode addStatement( const Soprano::Statement& statement );
    Soprano::Error::ErrorCode removeStatement( const Soprano::Statement& statement );
    Soprano::Error::ErrorCode removeAllStatements( const Soprano::Statement& statement );

    using Soprano::FilterModel::addStatement;
    using Soprano::FilterModel::removeStatement;
    using Soprano::FilterModel::removeAllStatements;

    /**
     * All inferred triples are stored in the one crappy inference graph.
     * Thus, removing inference is as simple as removing this one graph.
     */
    QUrl crappyInferenceContext() const;

Q_SIGNALS:
    /**
     * Emitted once a call to updateAllResources() has finished.
     */
    void allResourcesUpdated();

public Q_SLOTS:
    /**
     * Update the internal map of super types that is used to determine which
     * type statements need to be added and removed. It makes sense to call this
     * when the ontologies in the model change.
     */
    void updateInferenceIndex();

    /**
     * Update all types of all resources in the model. This method will add supertype
     * statements but NOT remove any superfluous statements. To achieve the latter call
     * removeContext(crappyInferenceContext()) before calling this method.
     */
    void updateAllResources();

private:
    void addInferenceStatements( const QUrl& resource, const QUrl& type );
    void addInferenceStatements( const QUrl& resource, const QSet<QUrl>& types );
    void removeInferenceStatements( const QUrl& resource, const QUrl& type );
    void removeInferenceStatements( const QUrl& resource, const QList<QUrl>& types );
    void removeInferenceStatements( const QUrl& resource, const QList<Soprano::Node>& types );

    void setVisibility(const QUrl& resource, bool visible);

    class UpdateAllResourcesThread;
    class Private;
    Private* const d;
};

#endif // CRAPPYINFERENCER2_H
