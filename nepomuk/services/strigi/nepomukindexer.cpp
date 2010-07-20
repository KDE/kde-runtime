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

#include "nepomukindexer.h"
#include "nepomukindexwriter.h"
#include "nepomukindexfeeder.h"
#include "nie.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>
#include <Nepomuk/Variant>

#include <KUrl>
#include <KDebug>

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <strigi/strigiconfig.h>
#include <strigi/indexwriter.h>
#include <strigi/analysisresult.h>
#include <strigi/fileinputstream.h>
#include <strigi/analyzerconfiguration.h>


namespace {
    class StoppableConfiguration : public Strigi::AnalyzerConfiguration
    {
    public:
        StoppableConfiguration()
            : m_stop(false) {
#if defined(STRIGI_IS_VERSION)
#if STRIGI_IS_VERSION( 0, 6, 1 )
            setIndexArchiveContents( false );
#endif
#endif
        }

        bool indexMore() const {
            return !m_stop;
        }

        bool addMoreText() const {
            return !m_stop;
        }

        void setStop( bool s ) {
            m_stop = s;
        }

    private:
        bool m_stop;
    };
}


class Nepomuk::Indexer::Private
{
public:
    StoppableConfiguration m_analyzerConfig;
    IndexFeeder* m_indexFeeder;
    StrigiIndexWriter* m_indexWriter;
    Strigi::StreamAnalyzer* m_streamAnalyzer;
};


Nepomuk::Indexer::Indexer( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->m_indexFeeder = new IndexFeeder( this );
    d->m_indexWriter = new StrigiIndexWriter( d->m_indexFeeder );
    d->m_streamAnalyzer = new Strigi::StreamAnalyzer( d->m_analyzerConfig );
    d->m_streamAnalyzer->setIndexWriter( *d->m_indexWriter );
}


Nepomuk::Indexer::~Indexer()
{
    delete d->m_streamAnalyzer;
    delete d->m_indexWriter;
    delete d->m_indexFeeder;
    delete d;
}


void Nepomuk::Indexer::indexFile( const KUrl& url )
{
    indexFile( QFileInfo( url.toLocalFile() ) );
}


void Nepomuk::Indexer::indexFile( const QFileInfo& info )
{
    d->m_analyzerConfig.setStop( false );

    KUrl url( info.filePath() );

    // strigi asserts if the file path has a trailing slash
    QString filePath = url.toLocalFile( KUrl::RemoveTrailingSlash );
    QString dir = url.directory( KUrl::IgnoreTrailingSlash );

    Strigi::AnalysisResult analysisresult( QFile::encodeName( filePath ).data(),
                                           info.lastModified().toTime_t(),
                                           *d->m_indexWriter,
                                           *d->m_streamAnalyzer,
                                           QFile::encodeName( dir ).data() );
    if ( info.isFile() && !info.isSymLink() ) {
#ifdef STRIGI_HAS_FILEINPUTSTREAM_OPEN
        Strigi::InputStream* stream = Strigi::FileInputStream::open( QFile::encodeName( info.filePath() ) );
        analysisresult.index( stream );
        delete stream;
#else
        Strigi::FileInputStream stream( QFile::encodeName( info.filePath() ) );
        analysisresult.index( &stream );
#endif
    }
    else {
        analysisresult.index(0);
    }
}


namespace {
    class QDataStreamStrigiBufferedStream : public Strigi::BufferedStream<char>
    {
    public:
        QDataStreamStrigiBufferedStream( QDataStream& stream )
            : m_stream( stream ) {
        }

        int32_t fillBuffer( char* start, int32_t space ) {
            int r = m_stream.readRawData( start, space );
            if ( r == 0 ) {
                // Strigi's API is so weird!
                return -1;
            }
            else if ( r < 0 ) {
                // Again: weird API. m_status is a protected member of StreamBaseBase (yes, 2x Base)
                m_status = Strigi::Error;
                return -1;
            }
            else {
                return r;
            }
        }

    private:
        QDataStream& m_stream;
    };
}


void Nepomuk::Indexer::indexResource( const KUrl& uri, const QDateTime& modificationTime, QDataStream& data )
{
    d->m_analyzerConfig.setStop( false );

    Resource dirRes( uri );
    if ( !dirRes.exists() ||
         dirRes.property( Nepomuk::Vocabulary::NIE::lastModified() ).toDateTime() != modificationTime ) {
        Strigi::AnalysisResult analysisresult( uri.toEncoded().data(),
                                               modificationTime.toTime_t(),
                                               *d->m_indexWriter,
                                               *d->m_streamAnalyzer );
        QDataStreamStrigiBufferedStream stream( data );
        analysisresult.index( &stream );
    }
    else {
        kDebug() << uri << "up to date";
    }
}


void Nepomuk::Indexer::removeIndexedData( const KUrl& url )
{
    Nepomuk::IndexFeeder::removeIndexedDataForUrl( url );
}


void Nepomuk::Indexer::removeIndexedData( const Nepomuk::Resource& res )
{
    Nepomuk::IndexFeeder::removeIndexedDataForResourceUri( res.resourceUri() );
}


void Nepomuk::Indexer::stop()
{
    d->m_analyzerConfig.setStop( true );
    d->m_indexFeeder->stop();
}

#include "nepomukindexer.moc"
