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

#include "desktopontologyupdatejob.h"

#include <KDebug>
#include <KConfigGroup>
#include <KDesktopFile>

#include <Soprano/StatementIterator>
#include <Soprano/PluginManager>
#include <Soprano/Parser>


Nepomuk::DesktopOntologyUpdateJob::DesktopOntologyUpdateJob( Soprano::Model* mainModel, QObject* parent )
    : OntologyUpdateJob( mainModel, parent )
{
}


Nepomuk::DesktopOntologyUpdateJob::~DesktopOntologyUpdateJob()
{
}


void Nepomuk::DesktopOntologyUpdateJob::setOntologyDesktopFile( const QString& df )
{
    m_desktopFilePath = df;
}


Soprano::StatementIterator Nepomuk::DesktopOntologyUpdateJob::data()
{
    kDebug() << "Updating ontology" << m_desktopFilePath;

    KDesktopFile df( m_desktopFilePath );

    // Parse the ontology data
    // ------------------------------------
    QString mimeType = df.desktopGroup().readEntry( "MimeType", QString() );
    const Soprano::Parser* parser
        = Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::mimeTypeToSerialization( mimeType ),
                                                                              mimeType );
    if ( !parser ) {
        kDebug() << "No parser to handle" << df.readName() << "(" << mimeType << ")";
        return 0;
    }

    kDebug() << "Parsing" << df.readPath();

    return parser->parseFile( df.readPath(),
                              df.readUrl(),
                              Soprano::mimeTypeToSerialization( mimeType ),
                              mimeType );
}

#include "desktopontologyupdatejob.moc"
