/*
 * localenum.h
 *
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


#ifndef LOCALENUM_H
#define LOCALENUM_H

#include <QWidget>

class QLineEdit;
class QComboBox;

class KLocale;

class KLocaleConfigNumber : public QWidget
{
  Q_OBJECT

public:
  explicit KLocaleConfigNumber( KLocale *_locale, QWidget *parent=0);
  virtual ~KLocaleConfigNumber( );

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

private Q_SLOTS:
  // Numbers
  void slotMonPosSignChanged(const QString &t);
  void slotMonNegSignChanged(const QString &t);
  void slotDecSymChanged(const QString &t);
  void slotThoSepChanged(const QString &t);
  void slotDigSetChanged(int);

private:
  KLocale *m_locale;

  // Numbers
  QLabel *m_labDecSym;
  QLineEdit *m_edDecSym;
  QLabel *m_labThoSep;
  QLineEdit *m_edThoSep;
  QLabel *m_labMonPosSign;
  QLineEdit *m_edMonPosSign;
  QLabel *m_labMonNegSign;
  QLineEdit *m_edMonNegSign;
  QLabel *m_labDigSet;
  QComboBox *m_cmbDigSet;
};

#endif // LOCALENUM_H
