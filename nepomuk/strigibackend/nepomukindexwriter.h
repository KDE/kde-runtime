/*
  Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

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

namespace Strigi {
    class NepomukIndexWriter : public Strigi::IndexWriter
    {
    public:
        NepomukIndexWriter( Soprano::Model* );
        ~NepomukIndexWriter();

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

        void startAnalysis( const AnalysisResult* );
        void addText( const AnalysisResult*, const char* text, int32_t length );
        void addValue( const AnalysisResult*, const RegisteredField* field,
                       const std::string& value );
        void addValue( const AnalysisResult*, const RegisteredField* field,
                       const unsigned char* data, uint32_t size );
        void addValue( const AnalysisResult*, const RegisteredField* field,
                       int32_t value );
        void addValue( const AnalysisResult*, const RegisteredField* field,
                       uint32_t value );
        void addValue( const AnalysisResult*, const RegisteredField* field,
                       double value );
        void addTriplet( const std::string& subject,
                         const std::string& predicate, const std::string& object );
        void addValue( const AnalysisResult*, const RegisteredField* field,
                       const std::string& name, const std::string& value );
        void finishAnalysis( const AnalysisResult* );

    private:
        /**
         * \param isResourceUri if true \p url is a resource URI, otherwise it
         * is a local file URL
         */
        void removeIndexedData( const KUrl& url, bool isResourceUri = false );

        QUrl determineFolderResourceUri( const KUrl& fileUrl );

        class Private;
        Private* d;
    };
}

uint qHash( const std::string& s );

#endif
