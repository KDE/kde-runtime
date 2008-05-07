/*
 * Modified version of StandardAnalyzer.cpp for Nepomuk mostly to optimize for filename indexing
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * Based on StandardAnalyzer.cpp from the CLucene package.
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

#include "cluceneanalyzer.h"
#include "clucenetokenizer.h"

#include <CLucene/StdHeader.h>
#include <CLucene/util/VoidMap.h>
#include <CLucene/util/Reader.h>
#include <CLucene/analysis/AnalysisHeader.h>
#include <CLucene/analysis/Analyzers.h>
#include <CLucene/analysis/standard/StandardFilter.h>

CL_NS_USE(util)
CL_NS_USE(analysis)

namespace Nepomuk {

	CLuceneAnalyzer::CLuceneAnalyzer()
        : stopSet(false)
	{
        CL_NS(analysis)::StopFilter::fillStopTable( &stopSet,CL_NS(analysis)::StopAnalyzer::ENGLISH_STOP_WORDS);
	}

	CLuceneAnalyzer::CLuceneAnalyzer( const TCHAR** stopWords):
		stopSet(false)
	{
		CL_NS(analysis)::StopFilter::fillStopTable( &stopSet,stopWords );
	}

	CLuceneAnalyzer::~CLuceneAnalyzer()
    {
	}


	CL_NS(analysis)::TokenStream* CLuceneAnalyzer::tokenStream(const TCHAR* fieldName, Reader* reader)
	{
		CL_NS(analysis)::TokenStream* ret = _CLNEW CLuceneTokenizer(reader);
		ret = _CLNEW CL_NS2(analysis, standard)::StandardFilter(ret,true);
		ret = _CLNEW CL_NS(analysis)::LowerCaseFilter(ret,true);
		ret = _CLNEW CL_NS(analysis)::StopFilter(ret,true, &stopSet);
		return ret;
	}
}
