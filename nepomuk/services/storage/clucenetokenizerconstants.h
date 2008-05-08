/*
 * Modified version of StandardTokenizerConstants.h for Nepomuk mostly to optimize for filename indexing
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * Based on StandardTokenizerConstants.h from the CLucene package.
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

#ifndef _NEPOMUK_CLUCENE_TOKENIZER_CONSTANTS_H_
#define _NEPOMUK_CLUCENE_TOKENIZER_CONSTANTS_H_

namespace Nepomuk {
    enum TokenTypes {
        _EOF,
        UNKNOWN,
        ALPHANUM,
        APOSTROPHE,
        ACRONYM,
        COMPANY,
        EMAIL,
        HOST,
        NUM,
        CJK
    };
    extern const TCHAR** tokenImage;
}

#endif
