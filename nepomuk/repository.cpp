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


Nepomuk::Repository::Repository()
{
}


Nepomuk::Repository::~Repository()
{
    delete m_indexModel;
    delete m_index;
    delete m_model;
}


Soprano::Model* Nepomuk::Repository::model() const
{
    return m_indexModel;
}


Soprano::Index::CLuceneIndex* Nepomuk::Repository::index() const
{
    return m_index;
}


Nepomuk::Repository* Nepomuk::Repository::open( const QString& path, const QString& name )
{
    kDebug(300002) << "(Nepomuk::Repository::open) opening repository '" << name << "' at '" << path << "'";
    KStandardDirs::makeDir( path );
    QList<Soprano::BackendSetting> settings;
    settings.append( Soprano::BackendSetting( Soprano::BackendOptionStorageDir, path ) );
    Soprano::Model* model = Soprano::createModel( settings );
    if ( model ) {
        kDebug(300002) << "(Nepomuk::Repository::open) Successfully created new model.";
        Soprano::Index::CLuceneIndex* index = new Soprano::Index::CLuceneIndex();
        if ( index->open( path + "/index", true ) ) {
            kDebug(300002) << "(Nepomuk::Repository::open) Successfully created new index.";
            Repository* rep = new Repository();
            rep->m_model = model;
            rep->m_index = index;
            rep->m_indexModel = new Soprano::Index::IndexFilterModel( index, model );
//            rep->m_indexModel->setTransactionCacheSize( 50 );
            return rep;
        }
        else {
            kDebug(300002) << "(Nepomuk::Repository::open) Unable to open CLucene index for repo '" << name << "': " << index->lastError();
            delete index;
            delete model;
        }
    }
    else {
        kDebug(300002) << "(Nepomuk::Repository::open) Unable to create new model.";
    }

    return 0;
}
