/*
    localdomainurifilter.h

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LOCALDOMAINURIFILTER_H_
#define _LOCALDOMAINURIFILTER_H_

#include <time.h>

#include <kgenericfactory.h>
#include <kurifilter.h>
#include <QRegExp>

/*
 This filter takes care of hostnames in the local search domain.
 If you're in domain domain.org which has a host intranet.domain.org
 and the typed URI is just intranet, check if there's a host
 intranet.domain.org and if yes, it's a network URI.
*/

class LocalDomainUriFilter : public KUriFilterPlugin
{
  Q_OBJECT

  public:
    LocalDomainUriFilter( QObject* parent, const QVariantList& args );
    virtual bool filterUri( KUriFilterData &data ) const;

  public Q_SLOTS:
    void configure();

  private:
    bool isLocalDomainHost( QString& cmd ) const;
    mutable QString last_host;
    mutable bool last_result;
    mutable time_t last_time;
    QRegExp m_hostPortPattern;
};

#endif
