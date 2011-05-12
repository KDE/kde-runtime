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

#ifndef CLASSANDPROPERTYTREE_H
#define CLASSANDPROPERTYTREE_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QMutex>
#include <QtCore/QList>

namespace Soprano {
class Model;
class Node;
class Statement;
}


namespace Nepomuk {
class SimpleResource;
class ClassAndPropertyTree : public QObject
{
    Q_OBJECT

public:
    ClassAndPropertyTree(QObject *parent = 0);
    ~ClassAndPropertyTree();

    QSet<QUrl> allParents(const QUrl& uri) const;
    bool isChildOf(const QUrl& type, const QUrl& superClass) const;

    /**
     * Returns true if any one of the uris in \p types is a child of \p superClass
     */
    bool isChildOf(const QList<QUrl> & types, const QUrl& superClass) const;
    int maxCardinality(const QUrl& type) const;
    bool isUserVisible(const QUrl& type) const;

    QUrl propertyDomain(const QUrl& uri) const;
    QUrl propertyRange(const QUrl& uri) const;

    QList<QUrl> visibleTypes() const;

    /// will try very hard to convert a variant into a node. Supports literal XML types and QUrl
    Soprano::Node variantToNode(const QVariant& value, const QUrl& property) const;
    QSet<Soprano::Node> variantListToNodeSet(const QVariantList& vl, const QUrl& property) const;

    QList<Soprano::Statement> simpleResourceToStatementList(const Nepomuk::SimpleResource& res) const;

public Q_SLOTS:
    void rebuildTree(Soprano::Model* model);

private:
    class ClassOrProperty;

    const ClassOrProperty* findClassOrProperty(const QUrl& uri) const;
    int updateUserVisibility(ClassOrProperty *cop, QSet<QUrl> &visitedNodes);
    QSet<QUrl> getAllParents(ClassOrProperty *cop, QSet<QUrl> &visitedNodes);

    QHash<QUrl, ClassOrProperty*> m_tree;

    mutable QMutex m_mutex;
};
}

#endif