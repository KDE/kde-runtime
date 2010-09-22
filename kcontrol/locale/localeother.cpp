/*
 * localeother.cpp
 *
 * Copyright (c) 2001-2003 Hans Petter Bieker <bieker@kde.org>
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

#include "localeother.h"

#include <QComboBox>
#include <QLabel>
#include <QPrinter>
#include <QGridLayout>

#include <KLocale>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include "localeother.moc"


KLocaleConfigOther::KLocaleConfigOther(KLocale *locale,
                                       QWidget *parent)
  : QWidget(parent),
    m_locale(locale)
{
  // Other
  QGridLayout *lay = new QGridLayout(this );

  m_labPageSize = new QLabel(this);
  m_labPageSize->setObjectName( QLatin1String(I18N_NOOP("Paper format:" )) );
  m_labPageSize->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  lay->addWidget(m_labPageSize, 0, 0);
  m_combPageSize = new QComboBox(this);
  lay->addWidget(m_combPageSize, 0, 1);
  connect( m_combPageSize, SIGNAL( activated(int) ),
           SLOT( slotPageSizeChanged(int) ) );

  m_labMeasureSystem = new QLabel(this);
  m_labMeasureSystem->setObjectName( QLatin1String( I18N_NOOP("Measure system:" )) );
  m_labMeasureSystem->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  lay->addWidget(m_labMeasureSystem, 1, 0);
  m_combMeasureSystem = new QComboBox(this);
  lay->addWidget(m_combMeasureSystem, 1, 1);
  connect( m_combMeasureSystem, SIGNAL( activated(int) ),
           SLOT( slotMeasureSystemChanged(int) ) );

  m_combPageSize->addItem(QString());
  m_combPageSize->addItem(QString());
  m_combMeasureSystem->addItem(QString());
  m_combMeasureSystem->addItem(QString());

  lay->setColumnStretch(1, 1);
  lay->setRowStretch(2, 1);

  adjustSize();
}

KLocaleConfigOther::~KLocaleConfigOther()
{
}

void KLocaleConfigOther::save()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group(config, "Locale");
  KConfig ent(KStandardDirs::locate("locale",
                           QString::fromLatin1("l10n/%1/entry.desktop")
                           .arg(m_locale->country())));
  KConfigGroup entGrp = ent.group("KCM Locale");

  // ### HPB: Add code here
  int i;
  i = entGrp.readEntry("PageSize", (int)QPrinter::A4);
  group.deleteEntry("PageSize", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->pageSize())
    group.writeEntry("PageSize",
                       m_locale->pageSize(), KConfig::Persistent|KConfig::Global);

  i = entGrp.readEntry("MeasureSystem", (int)KLocale::Metric);
  group.deleteEntry("MeasureSystem", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->measureSystem())
    group.writeEntry("MeasureSystem",
                       int(m_locale->measureSystem()), KConfig::Persistent|KConfig::Global);

  group.sync();
}

void KLocaleConfigOther::slotLocaleChanged()
{
  m_combMeasureSystem->setCurrentIndex(m_locale->measureSystem());

  int pageSize = m_locale->pageSize();

  int i = 0; // default to A4
  if ( pageSize == (int)QPrinter::Letter )
    i = 1;
  m_combPageSize->setCurrentIndex(i);
}

void KLocaleConfigOther::slotTranslate()
{
  m_combMeasureSystem->setItemText( 0, ki18nc("The Metric System", "Metric").toString(m_locale) );
  m_combMeasureSystem->setItemText( 1, ki18nc("The Imperial System", "Imperial").toString(m_locale) );

  m_combPageSize->setItemText( 0, ki18n("A4").toString(m_locale) );
  m_combPageSize->setItemText( 1, ki18n("US Letter").toString(m_locale) );
}

void KLocaleConfigOther::slotPageSizeChanged(int i)
{
  QPrinter::PageSize pageSize = QPrinter::A4;

  if ( i == 1 )
    pageSize = QPrinter::Letter;

  m_locale->setPageSize((int)pageSize);
  emit localeChanged();
}

void KLocaleConfigOther::slotMeasureSystemChanged(int i)
{
  m_locale->setMeasureSystem((KLocale::MeasureSystem)i);
  emit localeChanged();
}
