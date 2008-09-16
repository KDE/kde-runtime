/*
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "resultiterator.h"

class Nepomuk::Search::ResultIterator::Private
{
public:

};


Nepomuk::Search::ResultIterator::ResultIterator()
    : d( new Private() )
{
}


Nepomuk::Search::ResultIterator::~ResultIterator()
{
    delete d;
}


bool Nepomuk::Search::ResultIterator::next()
{

}


Nepomuk::Search::Result Nepomuk::Search::ResultIterator::current() const
{

}
