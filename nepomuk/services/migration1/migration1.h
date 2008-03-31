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

#ifndef _NEPOMUK_MIGRATION_SERVICE_LEVEL_1_H_
#define _NEPOMUK_MIGRATION_SERVICE_LEVEL_1_H_

#include <Nepomuk/Service>

namespace Nepomuk {
    /**
     * This simple run-once service migrates old metadata data by
     * updating it to the new ontology concepts.
     *
     * In detail this means:
     * nao:label -> nao:prefLabel
     * nao:rating -> nao:numericRating
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class Migration1 : public Nepomuk::Service
    {
        Q_OBJECT

    public:
        Migration1( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~Migration1();
    };
}

#endif
