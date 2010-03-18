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

#include "nepomukindexreader.h"

using namespace Soprano;
using namespace std;


class Strigi::NepomukIndexReader::Private
{
public:
    Soprano::Model* repository;
};


Strigi::NepomukIndexReader::NepomukIndexReader( Soprano::Model* model )
    : Strigi::IndexReader()
{
    d = new Private;
    d->repository = model;
}


Strigi::NepomukIndexReader::~NepomukIndexReader()
{
    delete d;
}


// not implemented
void Strigi::NepomukIndexReader::getChildren( const std::string&,
                                              std::map<std::string, time_t>& )
{
    // unused in Nepomuk
}


// not implemented
int64_t Strigi::NepomukIndexReader::indexSize()
{
    return -1;
}


// not implemented
time_t Strigi::NepomukIndexReader::mTime( const std::string& )
{
    return -1;
}



// not implemented
int32_t Strigi::NepomukIndexReader::countDocuments()
{
    return 0;
}


// not implemented
int32_t Strigi::NepomukIndexReader::countWords()
{
    return -1;
}


// not implemented
std::vector<std::string> Strigi::NepomukIndexReader::fieldNames()
{
    return std::vector<std::string>();
}


// not implemented
int32_t Strigi::NepomukIndexReader::countHits( const Query& )
{
    return -1;
}


// not implemented
void Strigi::NepomukIndexReader::getHits( const Strigi::Query&,
                                          const std::vector<std::string>&,
                                          const std::vector<Strigi::Variant::Type>&,
                                          std::vector<std::vector<Strigi::Variant> >&,
                                          int,
                                          int )
{
}


// not implemented
std::vector<Strigi::IndexedDocument> Strigi::NepomukIndexReader::query( const Query&, int, int )
{
    return vector<IndexedDocument>();
}


// not implemented
std::vector<std::pair<std::string,uint32_t> > Strigi::NepomukIndexReader::histogram( const std::string&,
                                                                                     const std::string&,
                                                                                     const std::string& )
{
    return std::vector<std::pair<std::string,uint32_t> >();
}


// not implemented
int32_t Strigi::NepomukIndexReader::countKeywords( const std::string&,
                                                   const std::vector<std::string>& )
{
    // the clucene indexer also returns 2. I suspect this means: "not implemented" ;)
    return 2;
}


// not implemented
std::vector<std::string> Strigi::NepomukIndexReader::keywords( const std::string&,
                                                               const std::vector<std::string>&,
                                                               uint32_t,
                                                               uint32_t )
{
    return std::vector<std::string>();
}
