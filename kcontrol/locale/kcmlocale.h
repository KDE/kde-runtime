/*
 * kcmlocale.h
 *
 * Copyright (c) 1998 Matthias Hoelzer <hoelzer@physik.uni-wuerzburg.de>
 * Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>
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

#ifndef KCMLOCALE_H
#define KCMLOCALE_H

#include <QWidget>
#include "ui_language.h"

class KControlLocale;

class KLocaleConfig : public QWidget, public Ui_Language
{
  Q_OBJECT

public:
  explicit KLocaleConfig( KControlLocale *_locale, QWidget *parent = 0);

  void save();

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
  void languageChanged();

private Q_SLOTS:
  void changeCountry();

  void slotCheckButtons();
  void slotAddLanguage(const QString & id);
  void slotRemoveLanguage();
  void slotLanguageUp();
  void slotLanguageDown();
private:
  enum Direction {Up,Down};
  void languageMove(Direction direcition);
  int selectedRow() const;


  // NOTE: we need to mantain our own language list instead of using KLocale's
  // because KLocale does not add a language if there is no translation
  // for the current application so it would not be possible to set
  // a language which has no systemsettings/kcontrol module translation
  QStringList m_languageList;
  KControlLocale *m_locale;
};

#endif
