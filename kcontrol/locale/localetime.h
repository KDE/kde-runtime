/*
 * localetime.h
 *
 * Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>
 * Copyright (c) 2008 John Layt <john@layt.net>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LOCALETIME_H
#define LOCALETIME_H

#include "ui_localetime.h"

#include <QWidget>

#include <QMap>

class QCheckBox;
class QComboBox;

class KLocale;

class StringPair;

class KLocaleConfigTime : public QWidget, Ui::localetime
{
  Q_OBJECT

public:
  explicit KLocaleConfigTime( KLocale *_locale, QWidget *parent=0);
  virtual ~KLocaleConfigTime( );

  void save();

protected:
  void showEvent( QShowEvent *e );

public Q_SLOTS:
  /**
   * Loads all settings from the current locale into the current widget.
   */
  void slotLocaleChanged();
  /**
   * Retranslate all objects owned by this object using the current locale.
   */
  void slotTranslate();

Q_SIGNALS:
  void localeChanged();

private Q_SLOTS:
  // Time & dates
  void slotTimeFmtChanged(const QString &t);
  void slotDateFmtChanged(const QString &t);
  void slotDateFmtShortChanged(const QString &t);
  void slotWeekStartDayChanged(int firstDay);
  void slotWorkingWeekStartDayChanged(int startDay);
  void slotWorkingWeekEndDayChanged(int endDay);
  void slotWeekDayOfPrayChanged(int weekDay);
  void slotDateMonthNamePossChanged();
  void slotCalendarSystemChanged(int calendarSystem);

private:
  void updateWeekDayNames();

  QList<StringPair> timeMap() const;
  QList<StringPair> dateMap() const;

  QString storeToUser(const QList<StringPair> & map,
		      const QString & storeFormat) const;
  QString userToStore(const QList<StringPair> & map,
		      const QString & userFormat) const;
  StringPair buildStringPair(const QChar &storeName, const QString &userName) const;

  KLocale *m_locale;

};

#endif // LOCALETIME_H
