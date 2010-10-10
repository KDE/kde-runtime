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

  m_labBinaryUnit = new QLabel(this);
  m_labBinaryUnit->setObjectName( QLatin1String( I18N_NOOP( "Units for byte sizes:" )) );
  m_labBinaryUnit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  lay->addWidget(m_labBinaryUnit, 2, 0);
  m_combBinaryUnit = new QComboBox(this);
  lay->addWidget(m_combBinaryUnit, 2, 1);
  connect( m_combBinaryUnit, SIGNAL( activated(int) ),
           SLOT( slotByteSizeUnitsChanged(int) ) );

  m_labBinaryUnitExample = new QLabel(this);
  lay->addWidget(m_labBinaryUnitExample, 3, 1);

  QString binaryUnitsWhatsThis = ki18n("<qt>This changes the units used by most KDE "
    "programs to display numbers counted in bytes. Traditionally \"kilobytes\" meant "
    "units of 1024, instead of the metric 1000, for most (but not all) byte sizes."
    "<ul><li>To reduce confusion you can use the recently standardized IEC units which "
    "are always in multiples of 1024.</li>"
    "<li>You can also select metric, which is always in units of 1000.</li>"
    "<li>Selecting JEDEC restores the older-style units used in KDE 3.5 and some other "
    "operating systems.</li></qt>").toString(m_locale);

  m_combBinaryUnit->setWhatsThis(binaryUnitsWhatsThis);
  m_labBinaryUnit->setWhatsThis(binaryUnitsWhatsThis);

  // Pre-fill the number of entries needed in each combo box for the translated
  // options.
  m_combPageSize->addItem(QString());
  m_combPageSize->addItem(QString());
  m_combMeasureSystem->addItem(QString());
  m_combMeasureSystem->addItem(QString());
  m_combBinaryUnit->addItem(QString());
  m_combBinaryUnit->addItem(QString());
  m_combBinaryUnit->addItem(QString());

  lay->setColumnStretch(1, 1);
  lay->setRowStretch(4, 1);

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

  i = entGrp.readEntry("BinaryUnitDialect", (int)KLocale::DefaultBinaryDialect);
  group.deleteEntry("BinaryUnitDialect", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->binaryUnitDialect())
    group.writeEntry("BinaryUnitDialect",
                       int(m_locale->binaryUnitDialect()), KConfig::Persistent|KConfig::Global);

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

  m_combBinaryUnit->setCurrentIndex(static_cast<int>(m_locale->binaryUnitDialect()));

  m_labBinaryUnitExample->setText(
    ki18nc("Example test for binary unit dialect", "Example: 2000 bytes equals %1")
      .subs(m_locale->formatByteSize(2000, 2))
      .toString());
}

void KLocaleConfigOther::slotTranslate()
{
  m_combMeasureSystem->setItemText( 0, ki18nc("The Metric System", "Metric").toString(m_locale) );
  m_combMeasureSystem->setItemText( 1, ki18nc("The Imperial System", "Imperial").toString(m_locale) );

  m_combPageSize->setItemText( 0, ki18n("A4").toString(m_locale) );
  m_combPageSize->setItemText( 1, ki18n("US Letter").toString(m_locale) );

  m_combBinaryUnit->setItemText( 0,
    ki18nc("Unit of binary measurement", "IEC Units (KiB, MiB, etc.)").toString(m_locale) );
  m_combBinaryUnit->setItemText( 1,
    ki18nc("Unit of binary measurement", "JEDEC Units (KB, MB, etc.)").toString(m_locale) );
  m_combBinaryUnit->setItemText( 2,
    ki18nc("Unit of binary measurement", "Metric Units (kB, MB, etc.)").toString(m_locale) );
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

void KLocaleConfigOther::slotByteSizeUnitsChanged(int i)
{
  m_locale->setBinaryUnitDialect(static_cast<KLocale::BinaryUnitDialect>(i));

  m_labBinaryUnitExample->setText(
    ki18nc("Example test for binary unit dialect", "Example: 2000 bytes equals %1")
      .subs(m_locale->formatByteSize(2000, 2))
      .toString());

  emit localeChanged();
}
