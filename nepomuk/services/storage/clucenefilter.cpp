/*
 * Modified version of StandardFilter.cpp for Nepomuk mostly to optimize for filename indexing
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * Based on StandardFilter.cpp from the CLucene package.
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
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

#include <CLucene/StdHeader.h>
#include "clucenefilter.h"
#include "clucenetokenizerconstants.h"

CL_NS_USE(analysis)
CL_NS_USE(util)

namespace Nepomuk {

    CLuceneFilter::CLuceneFilter(TokenStream* in, bool deleteTokenStream)
        : TokenFilter(in, deleteTokenStream)
    {
    }

    CLuceneFilter::~CLuceneFilter(){
    }

    bool CLuceneFilter::next(Token* t) {
        if (!input->next(t))
            return false;

        TCHAR* text = t->_termText;
        const int32_t textLength = t->termTextLength();
        const TCHAR* type = t->type();

        if ( type == tokenImage[APOSTROPHE] && //we can compare the type directy since the type should always come from the tokenImage
             ( textLength >= 2 && _tcsicmp(text+textLength-2, _T("'s"))==0  ) )
        {
            // remove 's
            text[textLength-2]=0;
            t->resetTermTextLen();

            return true;

        } else if ( type == tokenImage[ACRONYM] ) {		  // remove dots
            int32_t j = 0;
            for ( int32_t i=0;i<textLength;i++ ){
                if ( text[i] != '.' )
                    text[j++]=text[i];
            }
            text[j]=0;
            return true;

        } else {
            return true;
        }
    }
}
