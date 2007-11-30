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
#include "nepomukserver-config.h"

#include <Soprano/Backend>
#include <Soprano/Global>
#include <Soprano/Model>

#ifdef HAVE_SOPRANO_INDEX
#include <Soprano/Index/IndexFilterModel>
#include <Soprano/Index/CLuceneIndex>
#endif

#include <kstandarddirs.h>
#include <kdebug.h>


Nepomuk::Repository::Repository()
    : m_model( 0 ),
      m_index( 0 ),
      m_indexModel( 0 )
{
}


Nepomuk::Repository::~Repository()
{
#ifdef HAVE_SOPRANO_INDEX
    delete m_indexModel;
    delete m_index;
#endif
    delete m_model;
}


Soprano::Model* Nepomuk::Repository::model() const
{
#ifdef HAVE_SOPRANO_INDEX
    return m_indexModel;
#else
    return m_model;
#endif
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
#ifdef HAVE_SOPRANO_INDEX
        Soprano::Index::CLuceneIndex* index = new Soprano::Index::CLuceneIndex();
        if ( index->open( path + "/index", true ) ) {
            kDebug(300002) << "(Nepomuk::Repository::open) Successfully created new index.";
            Repository* rep = new Repository();
            rep->m_model = model;
            rep->m_index = index;
            rep->m_indexModel = new Soprano::Index::IndexFilterModel( index, model );

            // FIXME: find a good value here
            rep->m_indexModel->setTransactionCacheSize( 500 );
            return rep;
        }
        else {
            kDebug(300002) << "(Nepomuk::Repository::open) Unable to open CLucene index for repo '" << name << "': " << index->lastError();
            delete index;
            delete model;
        }
#else
        Repository* rep = new Repository();
        rep->m_model = model;
        return rep;
#endif
    }
    else {
        kDebug(300002) << "(Nepomuk::Repository::open) Unable to create new model.";
    }

    return 0;
}
