/*
  Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef _STRIGI_NEPOMUK_INDEX_WRITER_H_
#define _STRIGI_NEPOMUK_INDEX_WRITER_H_

#include <strigi/indexwriter.h>
#include <strigi/analysisresult.h>
#include <strigi/analyzerconfiguration.h>

namespace Soprano {
    class Model;
}

class KUrl;
class QUrl;

namespace Nepomuk {
    class StrigiIndexWriter : public Strigi::IndexWriter
    {
    public:
        StrigiIndexWriter( Soprano::Model* );
        ~StrigiIndexWriter();

        void commit();

        /**
         * Delete the entries with the given paths from the index.
         *
         * @param entries the paths of the files that should be deleted
         **/
        void deleteEntries( const std::vector<std::string>& entries );

        /**
         * Delete all indexed documents from the index.
         **/
        void deleteAllEntries();

        void initWriterData( const Strigi::FieldRegister& );
        void releaseWriterData( const Strigi::FieldRegister& );

        void startAnalysis( const Strigi::AnalysisResult* );
        void addText( const Strigi::AnalysisResult*, const char* text, int32_t length );
        void addValue( const Strigi::AnalysisResult*, const Strigi::RegisteredField* field,
                       const std::string& value );
        void addValue( const Strigi::AnalysisResult*, const Strigi::RegisteredField* field,
                       const unsigned char* data, uint32_t size );
        void addValue( const Strigi::AnalysisResult*, const Strigi::RegisteredField* field,
                       int32_t value );
        void addValue( const Strigi::AnalysisResult*, const Strigi::RegisteredField* field,
                       uint32_t value );
        void addValue( const Strigi::AnalysisResult*, const Strigi::RegisteredField* field,
                       double value );
        void addTriplet( const std::string& subject,
                         const std::string& predicate, const std::string& object );
        void addValue( const Strigi::AnalysisResult*, const Strigi::RegisteredField* field,
                       const std::string& name, const std::string& value );
        void finishAnalysis( const Strigi::AnalysisResult* );

    private:
        /**
         * \param uri The resource URI. Can be empty if \p url is not.
         * \param url The file URL. Can be empty if \p uri is not.
         */
        void removeIndexedData( const KUrl& uri, const KUrl& url );

        QUrl determineFolderResourceUri( const KUrl& fileUrl );

        class Private;
        Private* d;
    };
}

uint qHash( const std::string& s );

#endif
