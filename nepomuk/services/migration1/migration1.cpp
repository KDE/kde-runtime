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

#include "migration1.h"

#include <Soprano/Model>
#include <Soprano/Node>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/Vocabulary/NAO>

#include <kpluginfactory.h>
#include <kpluginloader.h>


Nepomuk::Migration1::Migration1( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    // we just do our data updating and afterwards we idle

    // 1. nao:label
    // ====================================
    QList<Soprano::Statement> oldLabelStatements
        = mainModel()->listStatements( Soprano::Node(),
                                       QUrl::fromEncoded( "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#label" ),
                                       Soprano::Node() ).allStatements();

    // update the data
    // (keep all metametadata like graphs and creation dates since the actual data does not change)
    foreach( Soprano::Statement s, oldLabelStatements ) {
        s.setPredicate( Soprano::Vocabulary::NAO::prefLabel() );
        mainModel()->addStatement( s );
    }

    mainModel()->removeStatements( oldLabelStatements );


    // 2. nao:rating
    // ====================================
    QList<Soprano::Statement> oldRatingStatements
        = mainModel()->listStatements( Soprano::Node(),
                                       QUrl::fromEncoded( "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#rating" ),
                                       Soprano::Node() ).allStatements();

    // update the data
    // (keep all metametadata like graphs and creation dates since the actual data does not change)
    foreach( Soprano::Statement s, oldRatingStatements ) { // krazy:exclude=foreach
        s.setPredicate( Soprano::Vocabulary::NAO::numericRating() );
        mainModel()->addStatement( s );
    }

    mainModel()->removeStatements( oldLabelStatements );
}


Nepomuk::Migration1::~Migration1()
{
}

NEPOMUK_EXPORT_SERVICE( Nepomuk::Migration1, "nepomukmigration1" )

#include "migration1.moc"
