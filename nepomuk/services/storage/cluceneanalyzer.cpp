/*
 * Modified version of StandardAnalyzer.cpp for Nepomuk mostly to optimize for filename indexing
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 */

/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
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
