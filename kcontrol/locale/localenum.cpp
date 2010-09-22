/*
 * localenum.cpp
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

#include "localenum.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>

#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KIntNumInput>

#include "toplevel.h"
#include "localenum.moc"

KLocaleConfigNumber::KLocaleConfigNumber(KLocale *locale,
					 QWidget *parent)
  : QWidget(parent),
    m_locale(locale)
{
  QGridLayout *lay = new QGridLayout(this );

  m_labDecSym = new QLabel(this);
  lay->addWidget(m_labDecSym, 0, 0);
  m_labDecSym->setObjectName( QLatin1String( I18N_NOOP("&Decimal symbol:" )) );
  m_labDecSym->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_edDecSym = new QLineEdit(this);
  lay->addWidget(m_edDecSym, 0, 1);
  connect( m_edDecSym, SIGNAL( textChanged(const QString &) ),
	   this, SLOT( slotDecSymChanged(const QString &) ) );
  m_labDecSym->setBuddy(m_edDecSym);

  m_labThoSep = new QLabel(this);
  lay->addWidget(m_labThoSep, 1, 0);
  m_labThoSep->setObjectName( QLatin1String( I18N_NOOP("Tho&usands separator:" )) );
  m_labThoSep->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_edThoSep = new QLineEdit(this);
  lay->addWidget(m_edThoSep, 1, 1);
  connect( m_edThoSep, SIGNAL( textChanged(const QString &) ),
	   this, SLOT( slotThoSepChanged(const QString &) ) );
  m_labThoSep->setBuddy(m_edThoSep);

  m_labMonPosSign = new QLabel(this);
  lay->addWidget(m_labMonPosSign, 2, 0);
  m_labMonPosSign->setObjectName( QLatin1String( I18N_NOOP("Positive si&gn:" )) );
  m_labMonPosSign->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_edMonPosSign = new QLineEdit(this);
  lay->addWidget(m_edMonPosSign, 2, 1);
  connect( m_edMonPosSign, SIGNAL( textChanged(const QString &) ),
	   this, SLOT( slotMonPosSignChanged(const QString &) ) );
  m_labMonPosSign->setBuddy(m_edMonPosSign);

  m_labMonNegSign = new QLabel(this);
  lay->addWidget(m_labMonNegSign, 3, 0);
  m_labMonNegSign->setObjectName( QLatin1String( I18N_NOOP("&Negative sign:" )) );
  m_labMonNegSign->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_edMonNegSign = new QLineEdit(this);
  lay->addWidget(m_edMonNegSign, 3, 1);
  connect( m_edMonNegSign, SIGNAL( textChanged(const QString &) ),
	   this, SLOT( slotMonNegSignChanged(const QString &) ) );
  m_labMonNegSign->setBuddy(m_edMonNegSign);

  m_labelDecimalPlaces = new QLabel(this);
  lay->addWidget(m_labelDecimalPlaces, 4, 0);
  m_labelDecimalPlaces->setObjectName( QLatin1String( I18N_NOOP("Decimal &places:" )) );
  m_labelDecimalPlaces->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_intDecimalPlaces = new KIntNumInput(this);
  m_intDecimalPlaces->setRange(0, 10, 1);
  lay->addWidget(m_intDecimalPlaces, 4, 1);
  connect( m_intDecimalPlaces, SIGNAL( valueChanged(int) ),
           this,               SLOT( slotDecimalPlacesChanged(int) ) );
  m_labelDecimalPlaces->setBuddy(m_intDecimalPlaces);

  m_labDigSet = new QLabel(this);
  lay->addWidget(m_labDigSet, 5, 0);
  m_labDigSet->setObjectName( QLatin1String( I18N_NOOP("Di&git set:" )) );
  m_labDigSet->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_cmbDigSet = new QComboBox(this);
  lay->addWidget(m_cmbDigSet, 5, 1);
  connect( m_cmbDigSet, SIGNAL( activated(int) ),
	   this, SLOT( slotDigSetChanged(int) ) );
  m_labDigSet->setBuddy(m_cmbDigSet);


  lay->setColumnStretch(1, 1);
  lay->setRowStretch(6, 1);

  connect(this, SIGNAL(localeChanged()),
	  SLOT(slotLocaleChanged()));
}

KLocaleConfigNumber::~KLocaleConfigNumber()
{
}

void KLocaleConfigNumber::save()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group(config, "Locale");
  KConfig ent(KStandardDirs::locate("locale",
			   QString::fromLatin1("l10n/%1/entry.desktop")
			   .arg(m_locale->country())));
  ent.setLocale(m_locale->language());
  KConfigGroup entGrp = ent.group("KCM Locale");

  QString str;
  int i;

  str = entGrp.readEntry("DecimalSymbol",
		      QString::fromLatin1("."));
  group.deleteEntry("DecimalSymbol", KConfig::Persistent | KConfig::Global);
  if (str != m_locale->decimalSymbol())
    group.writeEntry("DecimalSymbol",
		       m_locale->decimalSymbol(), KConfig::Persistent|KConfig::Global);

  str = entGrp.readEntry("ThousandsSeparator",
		      QString::fromLatin1(","));
  group.deleteEntry("ThousandsSeparator", KConfig::Persistent | KConfig::Global);
  str.remove(QString::fromLatin1("$0"));
  if (str != m_locale->thousandsSeparator())
    group.writeEntry("ThousandsSeparator",
		       QString::fromLatin1("$0%1$0")
		       .arg(m_locale->thousandsSeparator()), KConfig::Persistent|KConfig::Global);

  str = entGrp.readEntry("PositiveSign");
  group.deleteEntry("PositiveSign", KConfig::Persistent | KConfig::Global);
  if (str != m_locale->positiveSign())
    group.writeEntry("PositiveSign", m_locale->positiveSign(), KConfig::Persistent|KConfig::Global);

  str = entGrp.readEntry("NegativeSign", QString::fromLatin1("-"));
  group.deleteEntry("NegativeSign", KConfig::Persistent | KConfig::Global);
  if (str != m_locale->negativeSign())
    group.writeEntry("NegativeSign", m_locale->negativeSign(), KConfig::Persistent|KConfig::Global);

  int decimalPlaces = entGrp.readEntry("DecimalPlaces", 2);
  group.deleteEntry("DecimalPlaces", KConfig::Persistent | KConfig::Global);
  if (decimalPlaces != m_locale->decimalPlaces()) {
    group.writeEntry("DecimalPlaces", m_locale->decimalPlaces(), KConfig::Persistent|KConfig::Global);
  }

  i = entGrp.readEntry("DigitSet", (int)KLocale::ArabicDigits);
  group.deleteEntry("DigitSet", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->digitSet())
    group.writeEntry("DigitSet", (int)m_locale->digitSet(), KConfig::Persistent|KConfig::Global);
}

void KLocaleConfigNumber::slotLocaleChanged()
{
  // #### load all settings here
  m_edDecSym->setText( m_locale->decimalSymbol() );
  m_edThoSep->setText( m_locale->thousandsSeparator() );
  m_edMonPosSign->setText( m_locale->positiveSign() );
  m_edMonNegSign->setText( m_locale->negativeSign() );
  m_intDecimalPlaces->setValue( m_locale->decimalPlaces() );
  m_cmbDigSet->setCurrentIndex( m_locale->digitSet() );
}

void KLocaleConfigNumber::slotDecSymChanged(const QString &t)
{
  m_locale->setDecimalSymbol(t);
  emit localeChanged();
}

void KLocaleConfigNumber::slotThoSepChanged(const QString &t)
{
  m_locale->setThousandsSeparator(t);
  emit localeChanged();
}

void KLocaleConfigNumber::slotMonPosSignChanged(const QString &t)
{
  m_locale->setPositiveSign(t);
  emit localeChanged();
}

void KLocaleConfigNumber::slotMonNegSignChanged(const QString &t)
{
  m_locale->setNegativeSign(t);
  emit localeChanged();
}

void KLocaleConfigNumber::slotDecimalPlacesChanged(int value)
{
    m_locale->setDecimalPlaces(value);
    emit localeChanged();
}

void KLocaleConfigNumber::slotDigSetChanged(int i)
{
  m_locale->setDigitSet((KLocale::DigitSet) i);
  emit localeChanged();
}

void KLocaleConfigNumber::slotTranslate()
{
  QString str;

  QList<KLocale::DigitSet> digitSets = m_locale->allDigitSetsList();
  qSort(digitSets);
  m_cmbDigSet->clear();
  foreach (const KLocale::DigitSet &ds, digitSets)
  {
    m_cmbDigSet->addItem(m_locale->digitSetToName(ds, true));
  }
  m_cmbDigSet->setCurrentIndex(m_locale->digitSet());

  str = ki18n( "<p>Here you can define the decimal separator used "
	       "to display numbers (i.e. a dot or a comma in "
	       "most countries).</p><p>"
	       "Note that the decimal separator used to "
	       "display monetary values has to be set "
	       "separately (see the 'Money' tab).</p>" ).toString( m_locale );
  m_labDecSym->setWhatsThis( str );
  m_edDecSym->setWhatsThis( str );

  str = ki18n( "<p>Here you can define the thousands separator "
	       "used to display numbers.</p><p>"
	       "Note that the thousands separator used to "
	       "display monetary values has to be set "
	       "separately (see the 'Money' tab).</p>" ).toString( m_locale );
  m_labThoSep->setWhatsThis( str );
  m_edThoSep->setWhatsThis( str );

  str = ki18n( "Here you can specify text used to prefix "
	       "positive numbers. Most people leave this "
	       "blank." ).toString( m_locale );
  m_labMonPosSign->setWhatsThis( str );
  m_edMonPosSign->setWhatsThis( str );

  str = ki18n( "Here you can specify text used to prefix "
	       "negative numbers. This should not be empty, so "
	       "you can distinguish positive and negative "
	       "numbers. It is normally set to minus (-)." ).toString( m_locale );
  m_labMonNegSign->setWhatsThis( str );
  m_edMonNegSign->setWhatsThis( str );

  str = ki18n( "Here you can set the number of decimal places "
               "displayed for numeric values, i.e. the number "
               "of digits <em>after</em> the decimal separator. " ).toString( m_locale );
  m_labelDecimalPlaces->setWhatsThis( str );
  m_intDecimalPlaces->setWhatsThis( str );

  str = ki18n( "<p>Here you can define the set of digits "
	       "used to display numbers. "
	       "If digits other than Arabic are selected, "
	       "they will appear only if used in the language "
	       "of the application or the piece of text "
	       "where the number is shown.</p>"
	       "<p>Note that the set of digits used to "
	       "display monetary values has to be set "
	       "separately (see the 'Money' tab).</p>" ).toString( m_locale );
  m_labDigSet->setWhatsThis( str );
  m_cmbDigSet->setWhatsThis( str );

}
