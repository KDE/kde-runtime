/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "activity.h"

#include <QDateTime>


using namespace Attica;

class Activity::Private : public QSharedData {
    public:
        QString m_id;
        Person m_associatedPerson;
        QDateTime m_timestamp;
        QString m_message;
        QUrl m_link;
};

Activity::Activity() : d(new Private)
{
}

Activity::Activity(const Attica::Activity& other)
    : d(other.d)
{
}

Activity& Activity::operator=(const Attica::Activity & other)
{
    d = other.d;
    return *this;
}

Activity::~Activity()
{
}


void Activity::setId( const QString &id )
{
    d->m_id = id;
}

QString Activity::id() const
{
    return d->m_id;
}

void Activity::setAssociatedPerson(const Person& associatedPerson)
{
    d->m_associatedPerson = associatedPerson;
}

Person Activity::associatedPerson() const
{
    return d->m_associatedPerson;
}

void Activity::setTimestamp( const QDateTime &date )
{
  d->m_timestamp = date;
}

QDateTime Activity::timestamp() const
{
  return d->m_timestamp;
}

void Activity::setMessage( const QString &c )
{
  d->m_message = c;
}

QString Activity::message() const
{
  return d->m_message;
}

void Activity::setLink( const QUrl &v )
{
  d->m_link = v;
}

QUrl Activity::link() const
{
  return d->m_link;
}


bool Activity::isValid() const {
  return !(d->m_id.isEmpty());
}
