/*
  toplevel.h - A KControl Application

  written 1998 by Matthias Hoelzer

  Copyright (c) 1998 Matthias Hoelzer <hoelzer@physik.uni-wuerzburg.de>
  Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>

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

#ifndef __TOPLEVEL_H__
#define __TOPLEVEL_H__

#include <kcmodule.h>
#include <klocale.h>
#include <kconfig.h>

class QGroupBox;
class QTabWidget;

class KControlLocale;
class KLocaleConfig;
class KLocaleConfigMoney;
class KLocaleConfigNumber;
class KLocaleConfigTime;
class KLocaleConfigOther;
class KLocaleSample;

class KLocaleApplication : public KCModule
{
  Q_OBJECT

public:
  KLocaleApplication(QWidget *parent, const QVariantList &);
  virtual ~KLocaleApplication();

  virtual void load();
  virtual void save();
  virtual void defaults();
  virtual QString quickHelp() const;

Q_SIGNALS:
  void languageChanged();
  void localeChanged();

public Q_SLOTS:
  /**
   * Retranslates the current widget.
   */
  void slotTranslate();
  void slotChanged();

private:
  KControlLocale *m_locale;

  QTabWidget          *m_tab;
  KLocaleConfig       *m_localemain;
  KLocaleConfigNumber *m_localenum;
  KLocaleConfigMoney  *m_localemon;
  KLocaleConfigTime   *m_localetime;
  KLocaleConfigOther  *m_localeother;

  QGroupBox           *m_gbox;
  KLocaleSample       *m_sample;

  KSharedConfigPtr m_globalConfig;
  KSharedConfigPtr m_nullConfig;
};


#endif