/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef KURIIKWSFILTER_H
#define KURIIKWSFILTER_H

#include <kgenericfactory.h>

#include <kurifilter.h>

class KAutoWebSearch : public KUriFilterPlugin
{
    Q_OBJECT
public:
    explicit KAutoWebSearch(QObject *parent = 0, const QVariantList &args = QVariantList() );
    ~KAutoWebSearch();

    virtual bool filterUri( KUriFilterData& ) const;

public Q_SLOTS:
    void configure();
};

#endif
