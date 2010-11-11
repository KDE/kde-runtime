/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef COMMON_H
#define COMMON_H

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>

#include <Soprano/Model>
#include <Soprano/Backend>
#include <Soprano/PluginManager>

#include "resourcestruct.h"

namespace Nepomuk {

    Soprano::Model * createNewModel( const QString & dir );

    void setAsDefaultModel( Soprano::Model * model );

    // File Generator
    class FileGenerator {
    public:
        FileGenerator();

        float m_FileProb;
        float m_DirProb;
        float m_CdupProb;
        float m_sideWaysProb;

        void create( const QString & dir, int num );
        QList<QUrl> urls();

    private:
        QList<QUrl> m_urls;

        QString m_baseFileName;
        QString m_baseDirName;

        void createFile( const QUrl & url );
        void createDir( const QUrl & dir );
    };


    // Metadata Generator
    class IndexMetadataGenerator {
    public:
        IndexMetadataGenerator();

        void createImageImage( const QUrl & url );
        void createMusicFile( const QUrl & url );
        void createVideoFile( const QUrl & url );
        void createTextFile( const QUrl & url );

        const ResourceHash& resourceHash() const;
    private:
        ResourceHash m_resourceHash;

        QHash<QString, ResourceStruct> m_artistHash;
        QHash<QString, ResourceStruct> m_albumHash;

        QStringList m_artists;
        QStringList m_albums;
    };


    /**
     * Only compares valuable statements. That does not include indexed statements.
     */
    QList<Soprano::Statement> getBackupStatements( Soprano::Model * model );

    class MetadataGenerator {
    public:
        MetadataGenerator( Soprano::Model * model );

        void rate( const QUrl & url, int rating = -1 );

    private:
        Nepomuk::ResourceManager * m_manager;
        Soprano::Model * m_model;
    };

}
#endif // COMMON_H
