/*
 * This file is part of Soprano Project.
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>
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

#include "tstring.h"

class TString::Private : public QSharedData
{
public:
    Private()
        : data( 0 ),
          wrap( false ) {
    }

    ~Private() {
        if ( !wrap ) {
            free( data );
        }
    }

    TCHAR* data;
    bool wrap;
};


TString::TString()
    : d( new Private() )
{
}


TString::TString( const TString& s )
{
    d = s.d;
}


TString::TString( const TCHAR* s, bool wrap )
    : d( new Private() )
{
    d->wrap = wrap;
    if ( wrap ) {
        d->data = const_cast<TCHAR*>( s );
    }
    else {
        operator=( s );
    }
}


TString::TString( const QString& s )
    : d( new Private() )
{
    operator=( s );
}


TString::~TString()
{
}


TString& TString::operator=( const TString& s )
{
    d = s.d;
    return *this;
}


TString& TString::operator=( const TCHAR* s )
{
#ifdef _UCS2
    size_t len = wcslen( s );
    d->data = ( TCHAR* )calloc( len+1, sizeof( TCHAR ) );
    if ( d->data ) {
        wcscpy( d->data, s );
    }
#else
    d->data = strdup( s );
#endif
    d->wrap = false;
    return *this;
}


TString& TString::operator=( const QString& s )
{
#ifdef _UCS2
    d->data = ( TCHAR* )calloc( s.length()+1, sizeof( TCHAR ) );
    s.toWCharArray( d->data );
#else
    d->data = strdup( s.toUtf8().data() );
#endif
    d->wrap = false;
    return *this;
}


bool TString::isEmpty() const
{
    return d->data == 0;
}


int TString::length() const
{
    return _tcslen( d->data );
}


const TCHAR* TString::data() const
{
    return d->data;
}


TString::operator QString() const
{
    return toQString();
}


QString TString::toQString() const
{
    if ( d->data ) {
#ifdef _UCS2
        return QString::fromWCharArray( d->data );
#else
        return QString::fromUtf8( d->data );
#endif
    }
    else {
        return QString();
    }
}


bool TString::operator==( const TString& other ) const
{
    return _tcscmp( d->data, other.d->data ) == 0;
}


bool TString::operator==( const QString& other ) const
{
    return toQString() == other;
}


bool TString::operator!=( const TString& other ) const
{
    return !operator==( other );
}


bool TString::operator!=( const QString& other ) const
{
    return toQString() != other;
}


TString TString::fromUtf8( const char* data )
{
    TString s;
#ifdef _UCS2
    s.d->data = ( TCHAR* )calloc( strlen( data )+1, sizeof( TCHAR ) ); // like this we are on the safe side
    QString::fromUtf8( data ).toWCharArray( s.d->data );
#else
    s.d->data = strdup( data );
#endif
    return s;
}
