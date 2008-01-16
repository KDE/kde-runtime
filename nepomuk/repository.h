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

#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include <QtCore/QString>
#include <QtCore/QMap>

#include <soprano/version.h>

#ifndef SOPRANO_IS_VERSION
#define SOPRANO_IS_VERSION(a,b,c) false
#endif

#if SOPRANO_IS_VERSION(2,0,90)
#include <Soprano/Util/SignalCacheModel>
#else
#include <Soprano/FilterModel>
#endif


namespace Soprano {
    class Model;
    namespace Index {
        class IndexFilterModel;
        class CLuceneIndex;
    }
}

namespace Nepomuk {

    class Repository : public
#if SOPRANO_IS_VERSION(2,0,90)
        Soprano::Util::SignalCacheModel
#else
        Soprano::FilterModel
#endif
    {
        Q_OBJECT

    public:
        ~Repository();

        QString name() const { return m_name; }

        static Repository* open( const QString& path, const QString& name );

    private:
        Repository();

        QString m_name;
        Soprano::Model* m_model;
        Soprano::Index::CLuceneIndex* m_index;
        Soprano::Index::IndexFilterModel* m_indexModel;
    };

    typedef QMap<QString, Repository*> RepositoryMap;
}

#endif
