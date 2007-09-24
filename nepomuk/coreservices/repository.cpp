/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "repository.h"

#include <soprano/soprano.h>
#include <soprano/cluceneindex.h>
#include <soprano/indexfiltermodel.h>

#include <kstandarddirs.h>
#include <kdebug.h>


Nepomuk::CoreServices::Repository::Repository()
{
}


Nepomuk::CoreServices::Repository::~Repository()
{
    delete m_indexModel;
    delete m_index;
    delete m_model;
}


Soprano::Model* Nepomuk::CoreServices::Repository::model() const
{
    return m_indexModel;
}


Soprano::Index::CLuceneIndex* Nepomuk::CoreServices::Repository::index() const
{
    return m_index;
}


Nepomuk::CoreServices::Repository* Nepomuk::CoreServices::Repository::open( const QString& path, const QString& name )
{
    kDebug() << "(Nepomuk::Repository::open) opening repository '" << name << "' at '" << path << "'";
    KStandardDirs::makeDir( path );
    QList<Soprano::BackendSetting> settings;
    settings.append( Soprano::BackendSetting( Soprano::BackendOptionStorageDir, path ) );
    Soprano::Model* model = Soprano::createModel( settings );
    if ( model ) {
        Soprano::Index::CLuceneIndex* index = new Soprano::Index::CLuceneIndex();
        if ( index->open( path + "/index", true ) ) {
            Repository* rep = new Repository();
            rep->m_model = model;
            rep->m_index = index;
            rep->m_indexModel = new Soprano::Index::IndexFilterModel( index, model );
            return rep;
        }
        else {
            kDebug() << "(Nepomuk::Repository::open) Unable to open CLucene index for repo '" << name << "': " << index->lastError();
            delete index;
            delete model;
        }
    }

    return 0;
}
