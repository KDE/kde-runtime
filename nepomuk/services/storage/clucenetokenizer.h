/*
 * Modified version of StandardTokenizer.h for Nepomuk mostly to optimize for filename indexing
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * Based on StandardTokenizer.h from the CLucene package.
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

#ifndef _NEPOMUK_CLUCENE_TOKENIZER_H_
#define _NEPOMUK_CLUCENE_TOKENIZER_H_

#include <CLucene/clucene-config.h>
#include <CLucene/analysis/AnalysisHeader.h>
#include <CLucene/analysis/Analyzers.h>
#include <CLucene/util/StringBuffer.h>
#include <CLucene/util/FastCharStream.h>
#include <CLucene/util/Reader.h>

#include "clucenetokenizerconstants.h"

namespace Nepomuk {

    /** A grammar-based tokenizer constructed with JavaCC.
     *
     * <p> This should be a good tokenizer for most European-language documents:
     *
     * <ul>
     *   <li>Splits words at punctuation characters, removing punctuation. However, a 
     *     dot that's not followed by whitespace is considered part of a token.
     *   <li>Splits words at hyphens, unless there's a number in the token, in which case
     *     the whole token is interpreted as a product number and is not split.
     *   <li>Recognizes email addresses and internet hostnames as one token.
     * </ul>
     *
     * <p>Many applications have specific tokenizer needs.  If this tokenizer does
     * not suit your application, please consider copying this source code
     * directory to your project and maintaining your own grammar-based tokenizer.
     */
    class CLuceneTokenizer: public CL_NS(analysis)::Tokenizer
    {
    public:
        CL_NS(util)::FastCharStream* rd;

        // Constructs a tokenizer for this Reader.
        CLuceneTokenizer(CL_NS(util)::Reader* reader);

        ~CLuceneTokenizer();

        /** Returns the next token in the stream, or false at end-of-stream.
         * The returned token's type is set to an element of
         * CLuceneTokenizerConstants::tokenImage. */
        bool next(CL_NS(analysis)::Token* token);

        // Reads for number like "1"/"1234.567", or IP address like "192.168.1.2".
        bool ReadNumber(const TCHAR* previousNumber, const TCHAR prev, CL_NS(analysis)::Token* t);

        bool ReadAlphaNum(const TCHAR prev, CL_NS(analysis)::Token* t);

        // Reads for apostrophe-containing word.
        bool ReadApostrophe(CL_NS(util)::StringBuffer* str, CL_NS(analysis)::Token* t);

        // Reads for something@... it may be a COMPANY name or a EMAIL address
        bool ReadAt(CL_NS(util)::StringBuffer* str, CL_NS(analysis)::Token* t);

        // Reads for COMPANY name like AT&T.
        bool ReadCompany(CL_NS(util)::StringBuffer* str, CL_NS(analysis)::Token* t);
    
        // Reads CJK characters
        bool ReadCJK(const TCHAR prev, CL_NS(analysis)::Token* t);

    private:
        int32_t rdPos;
        int32_t tokenStart;

        // Advance by one character, incrementing rdPos and returning the character.
        int readChar();
        // Retreat by one character, decrementing rdPos.
        void unReadChar();

        // createToken centralizes token creation for auditing purposes.
        //Token* createToken(CL_NS(util)::StringBuffer* sb, TokenTypes tokenCode);
        inline bool setToken(CL_NS(analysis)::Token* t, CL_NS(util)::StringBuffer* sb, TokenTypes tokenCode);

        bool ReadDotted(CL_NS(util)::StringBuffer* str, TokenTypes forcedType, CL_NS(analysis)::Token* t);
    };
}

#endif
