/*
 * Modified version of StandardAnalyzer.h for Nepomuk mostly to optimize for filename indexing
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 */

/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#ifndef _NEPOMUK_CLUCENE_ANALYZER_H_
#define _NEPOMUK_CLUCENE_ANALYZER_H_

#include <CLucene.h>
#include <CLucene/clucene-config.h>
#include <CLucene/StdHeader.h>
#include <CLucene/util/VoidMap.h>
#include <CLucene/util/Reader.h>
#include <CLucene/analysis/AnalysisHeader.h>
#include <CLucene/analysis/Analyzers.h>


namespace Nepomuk {

	/** Represents a standard analyzer. */
	class CLuceneAnalyzer : public CL_NS(analysis)::Analyzer 
	{
	public:
		/** Builds an analyzer.*/
		CLuceneAnalyzer();

		/** Builds an analyzer with the given stop words. */
		CLuceneAnalyzer( const TCHAR** stopWords);

		~CLuceneAnalyzer();

		/**
         * Constructs a StandardTokenizer filtered by a 
         * StandardFilter, a LowerCaseFilter and a StopFilter.
         */
		CL_NS(analysis)::TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);

 	private:
		CL_NS(util)::CLSetList<const TCHAR*> stopSet;
	};
}

#endif
