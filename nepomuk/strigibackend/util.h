/*
   $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $

   This file is part of the Strigi project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _STRIGI_SOPRNOA_UTIL_H_
#define _STRIGI_SOPRNOA_UTIL_H_

#include <string>
#include "tstring.h"

class QUrl;
namespace Soprano {
    class Model;
    class Node;
}

namespace Strigi {

    class Variant;

    namespace Soprano {
	namespace Util {
	    QUrl fieldUri( const std::string& s );
	    QUrl fileUrl( const std::string& filename );
	    std::string fieldName( const QUrl& uri );
	    TString convertSearchField( const std::string& field );
	    QUrl uniqueUri( const QString& ns, ::Soprano::Model* model );
	    Strigi::Variant nodeToVariant( const ::Soprano::Node& node );
	}
    }
}

#endif
