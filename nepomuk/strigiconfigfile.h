/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _NEPOMUK_STRIGI_CONFIG_H_
#define _NEPOMUK_STRIGI_CONFIG_H_

#include <QtCore/QObject>
#include <QtCore/QStringList>

class QDomElement;
class QTextStream;

namespace Nepomuk {
    /**
     * Parser and writer class for Strigidaemon config
     * files.
     *
     * The default Strigi config file can be found at
     * ~/.strigi/daemon.conf.
     */
    class StrigiConfigFile : public QObject
    {
        Q_OBJECT

    public:
        StrigiConfigFile();
        StrigiConfigFile( const QString& filename );
        ~StrigiConfigFile();

        void setFilename( const QString& filename );

        bool load();
        bool save();

        /**
         * A Strigi repository. Normally there is only one.
         */
        class Repository {
        public:
            QString name() const { return m_name; }
            QString type() const { return m_type; }
            QString indexDir() const { return m_indexDir; }
            bool writeable() const { return m_writeable; }
            QString urlBase() const { return m_urlBase; }
            QStringList indexedDirectories() const { return m_indexedDirectories; }
            int pollingInterval() const { return m_pollingInterval; }

            bool isValid() const { return !m_type.isEmpty(); }

            void setType( const QString& type ) { m_type = type; }
            void setName( const QString& name ) { m_name = name; }
            void setIndexDir( const QString& dir ) { m_indexDir = dir; }
            void setWriteable( bool writeable ) { m_writeable = writeable; }
            void setUrlBase( const QString& urlBase ) { m_urlBase = urlBase; }
            void addIndexedDirectory( const QString& dir ) { m_indexedDirectories << dir; }
            void setIndexedDirectories( const QStringList& dirs ) { m_indexedDirectories = dirs; }
            void setPollingInterval( int pollingInterval ) { m_pollingInterval = pollingInterval; }
            
        private:
            QString m_name;
            QString m_type;
            QString m_indexDir;
            bool m_writeable;
            QString m_urlBase;
            QStringList m_indexedDirectories;
            int m_pollingInterval;
        };

        bool useDBus() const;
        QStringList excludeFilters() const;
        QStringList includeFilters() const;
        QList<Repository> repositories() const;

        /**
         * In most cases (or even always) there is only
         * one repository. This method will return the 
         * first repository in the list or create a default
         * one and insert it if the list is empty.
         *
         * This will also create default filters if none are
         * specified AND no repository is configured.
         */
        Repository& defaultRepository();
        const Repository& defaultRepository() const;

        /**
         * ~/.strigi/daemon.conf
         */
        static QString defaultStrigiConfigFilePath();

    public Q_SLOTS:
        void reset();

        void setUseDBus( bool b );
        void setExcludeFilters( const QStringList& filters );
        void addExcludeFilter( const QString& filter );
        void setIncludeFilters( const QStringList& filters );
        void addInludeFilter( const QString& filter );
        void setRepositories( const QList<Repository>& repos );
        void addRepository( const Repository& repo );

    private:
        bool readConfig( const QDomElement& );
        Repository readRepositoryConfig( const QDomElement& );
        bool readFilterConfig( const QDomElement& filterElement );

        QString m_filename;

        bool m_useDBus;
        QStringList m_excludeFilters;
        QStringList m_includeFilters;
        QList<Repository> m_repositories;
    };
}

QTextStream& operator<<( QTextStream& s, const Nepomuk::StrigiConfigFile& scf );

#endif
