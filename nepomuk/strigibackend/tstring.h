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

#ifndef _W_STRING_H_
#define _W_STRING_H_

#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>

#include <CLucene.h>

/**
 * TString is a simple string class which works on wchar_t
 * data. Its main purpose it to provide conversion from and to
 * QString.
 */
class TString
{
public:
    /**
     * Create an empty TString
     */
    TString();

    /**
     * Create a copy of another TString
     */
    TString( const TString& );

    /**
     * Create a new TString instance.
     * \param data The string data to be used.
     * \param wrap If false the data will be copied for the new
     * string instance. If true the new TString will act as a wrapper
     * around data.
     */
    TString( const TCHAR* data, bool wrap = false );

    /**
     * Create a new TString instance as a copy of a QString
     */
    TString( const QString& );

    ~TString();

    TString& operator=( const TString& );
    TString& operator=( const TCHAR* );
    TString& operator=( const QString& );

    bool isEmpty() const;
    int length() const;

    bool operator==( const TString& ) const;
    bool operator==( const QString& ) const;
    bool operator!=( const TString& ) const;
    bool operator!=( const QString& ) const;

    /**
     * The raw string data. It remains valid until the
     * string is modified or deleted.
     *
     * \return the raw string data or 0 if the string is empty.
     */
    const TCHAR* data() const;

    operator QString() const;

    QString toQString() const;

    static TString fromUtf8( const char* data );
	
private:
    class Private;
    QSharedDataPointer<Private> d;
};

#endif
