/*
   Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
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

#ifndef STRIGI_NEPOMUK_INDEX_READER_H
#define STRIGI_NEPOMUK_INDEX_READER_H

#include <strigi/indexreader.h>

namespace Soprano {
    class Model;
}

namespace Strigi {

    class Query;

    /// The IndexReader is not used in Nepomuk and thus, this is only
    /// a dummy implementation
    class NepomukIndexReader : public Strigi::IndexReader
    {
    public:
        NepomukIndexReader( Soprano::Model* );
        ~NepomukIndexReader();

        void getChildren( const std::string& parent,
                          std::map<std::string, time_t>& children );
        int64_t indexSize();
        time_t mTime( const std::string& uri );
        int32_t countDocuments();
        int32_t countWords();
        std::vector<std::string> fieldNames();
        int32_t countHits( const Query& query );
        std::vector<IndexedDocument> query( const Query&, int off, int max );
        void getHits( const Strigi::Query& query,
                      const std::vector<std::string>& fields,
                      const std::vector<Strigi::Variant::Type>& types,
                      std::vector<std::vector<Strigi::Variant> >& result,
                      int off, int max );
        std::vector<std::pair<std::string,uint32_t> > histogram( const std::string& query,
                                                                 const std::string& fieldname,
                                                                 const std::string& labeltype );
        int32_t countKeywords( const std::string& keywordprefix,
                               const std::vector<std::string>& fieldnames);
        std::vector<std::string> keywords( const std::string& keywordmatch,
                                           const std::vector<std::string>& fieldnames,
                                           uint32_t max, uint32_t offset );

    private:
        class Private;
        Private* d;
    };
}

#endif
