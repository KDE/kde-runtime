/*
  toplevel.h - A KControl Application

  Copyright (C) 2007 Bernhard Loos <nhuh.put@web.de>

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

#ifndef __KCONTROLLOCALE_H__
#define __KCONTROLLOCALE_H__

#include <klocale.h>
#include <ksharedconfig.h>

class KCalendarSystem;

class KControlLocale : public KLocale
{
public:
    explicit KControlLocale(const QString &catalog, KSharedConfig::Ptr config_);

    void setLanguage(const QString &lang);
    void setLanguage(const QStringList &langlist);
    bool setCountry(const QString &country);
    const KCalendarSystem *calendar() const;
    void setCalendar(const QString & calendarType);

private:
    KSharedConfig::Ptr config;
    KCalendarSystem *m_calendar;
};

#endif
