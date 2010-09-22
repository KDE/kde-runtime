/*
 * localemon.cpp
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

#include "localemon.h"

#include <kdebug.h>
#include <QCheckBox>
#include <QComboBox>

#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KIntNumInput>
#include "kcurrencycode.h"

#include "toplevel.h"
#include "localemon.moc"

KLocaleConfigMoney::KLocaleConfigMoney(KLocale *locale,
                                       QWidget *parent)
  : QWidget(parent),
    m_locale(locale)
{
  setupUi(this);

  // Money
  m_labelCurrencyCode->setObjectName( QLatin1String( I18N_NOOP("Currency:" )) );
  m_labelCurrencySymbol->setObjectName( QLatin1String( I18N_NOOP("Currency symbol:" )) );
  m_labMonDecSym->setObjectName( QLatin1String( I18N_NOOP("Decimal symbol:" )) );
  m_labMonThoSep->setObjectName( QLatin1String( I18N_NOOP("Thousands separator:" )) );
  m_labelMonetaryDecimalPlaces->setObjectName( QLatin1String( I18N_NOOP("Decimal places:" )) );
  m_positiveGB->setObjectName( QLatin1String( I18N_NOOP("Positive" )) );
  m_chMonPosPreCurSym->setObjectName( QLatin1String(I18N_NOOP("Prefix currency symbol" )));
  m_labMonPosMonSignPos->setObjectName( QLatin1String( I18N_NOOP("Sign position:" )) );
  m_negativeGB->setObjectName( QLatin1String( I18N_NOOP("Negative" )) );
  m_chMonNegPreCurSym->setObjectName( QLatin1String(I18N_NOOP("Prefix currency symbol" )));
  m_labMonNegMonSignPos->setObjectName( QLatin1String( I18N_NOOP("Sign position:" )) );
  m_labMonDigSet->setObjectName( QLatin1String( I18N_NOOP("Digit set:" )) );

  connect( m_comboCurrencyCode, SIGNAL( activated(int) ),
           SLOT( slotCurrencyCodeChanged(int) ) );

  connect( m_comboCurrencySymbol, SIGNAL( activated(int) ),
           SLOT( slotCurrencySymbolChanged(int) ) );

  connect( m_edMonDecSym, SIGNAL( textChanged(const QString &) ),
           SLOT( slotMonDecSymChanged(const QString &) ) );

  connect( m_edMonThoSep, SIGNAL( textChanged(const QString &) ),
           SLOT( slotMonThoSepChanged(const QString &) ) );

  connect( m_intMonetaryDecimalPlaces, SIGNAL( valueChanged(int) ),
           SLOT( slotMonetaryDecimalPlacesChanged(int) ) );

  connect( m_chMonPosPreCurSym, SIGNAL( clicked() ),
           SLOT( slotMonPosPreCurSymChanged() ) );

  connect( m_cmbMonPosMonSignPos, SIGNAL( activated(int) ),
           SLOT( slotMonPosMonSignPosChanged(int) ) );

  connect( m_chMonNegPreCurSym, SIGNAL( clicked() ),
           SLOT( slotMonNegPreCurSymChanged() ) );

  connect( m_cmbMonNegMonSignPos, SIGNAL( activated(int) ),
           SLOT( slotMonNegMonSignPosChanged(int) ) );

  connect( m_cmbMonDigSet, SIGNAL( activated(int) ),
           SLOT( slotMonDigSetChanged(int) ) );
}

KLocaleConfigMoney::~KLocaleConfigMoney()
{
}

void KLocaleConfigMoney::save()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group(config, "Locale");
  KConfig ent(KStandardDirs::locate("locale",
                           QString::fromLatin1("l10n/%1/entry.desktop")
                           .arg(m_locale->country())));
  KConfigGroup entGrp = ent.group("KCM Locale");

  QString str;
  int i;
  bool b;

  QString currencyCode = entGrp.readEntry("CurrencyCode", m_locale->defaultCurrencyCode());
  group.deleteEntry("CurrencyCode", KConfig::Persistent | KConfig::Global);
  if (currencyCode != m_locale->currencyCode()) {
    group.writeEntry("CurrencyCode", m_locale->currencyCode(), KConfig::Persistent|KConfig::Global);
  }

  QString currencySymbol = entGrp.readEntry("CurrencySymbol", m_locale->currency()->defaultSymbol());
  group.deleteEntry("CurrencySymbol", KConfig::Persistent | KConfig::Global);
  if (currencySymbol != m_locale->currencySymbol()) {
    group.writeEntry("CurrencySymbol", m_locale->currencySymbol(), KConfig::Persistent|KConfig::Global);
  }

  str = entGrp.readEntry("MonetaryDecimalSymbol", QString::fromLatin1("."));
  group.deleteEntry("MonetaryDecimalSymbol", KConfig::Persistent | KConfig::Global);
  if (str != m_locale->monetaryDecimalSymbol())
    group.writeEntry("MonetaryDecimalSymbol",
                       m_locale->monetaryDecimalSymbol(), KConfig::Persistent|KConfig::Global);

  str = entGrp.readEntry("MonetaryThousandsSeparator", QString::fromLatin1(","));
  str.remove(QString::fromLatin1("$0"));
  group.deleteEntry("MonetaryThousandsSeparator", KConfig::Persistent | KConfig::Global);
  if (str != m_locale->monetaryThousandsSeparator())
    group.writeEntry("MonetaryThousandsSeparator",
                       QString::fromLatin1("$0%1$0")
                       .arg(m_locale->monetaryThousandsSeparator()),
                       KConfig::Persistent|KConfig::Global);

  int monetaryDecimalPlaces = entGrp.readEntry("MonetaryDecimalPlaces", m_locale->currency()->decimalPlaces());
  group.deleteEntry("MonetaryDecimalPlaces", KConfig::Persistent | KConfig::Global);
  if (monetaryDecimalPlaces != m_locale->monetaryDecimalPlaces()) {
    group.writeEntry("MonetaryDecimalPlaces", m_locale->monetaryDecimalPlaces(), KConfig::Persistent|KConfig::Global);
  }

  b = entGrp.readEntry("PositivePrefixCurrencySymbol", true);
  group.deleteEntry("PositivePrefixCurrencySymbol", KConfig::Persistent | KConfig::Global);
  if (b != m_locale->positivePrefixCurrencySymbol())
    group.writeEntry("PositivePrefixCurrencySymbol",
                       m_locale->positivePrefixCurrencySymbol(), KConfig::Persistent|KConfig::Global);

  b = entGrp.readEntry("NegativePrefixCurrencySymbol", true);
  group.deleteEntry("NegativePrefixCurrencySymbol", KConfig::Persistent | KConfig::Global);
  if (b != m_locale->negativePrefixCurrencySymbol())
    group.writeEntry("NegativePrefixCurrencySymbol",
                       m_locale->negativePrefixCurrencySymbol(), KConfig::Persistent|KConfig::Global);

  i = entGrp.readEntry("PositiveMonetarySignPosition",
                       (int)KLocale::BeforeQuantityMoney);
  group.deleteEntry("PositiveMonetarySignPosition", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->positiveMonetarySignPosition())
    group.writeEntry("PositiveMonetarySignPosition",
                       (int)m_locale->positiveMonetarySignPosition(),
                       KConfig::Persistent|KConfig::Global);

  i = entGrp.readEntry("NegativeMonetarySignPosition",
                       (int)KLocale::ParensAround);
  group.deleteEntry("NegativeMonetarySignPosition", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->negativeMonetarySignPosition())
    group.writeEntry("NegativeMonetarySignPosition",
                       (int)m_locale->negativeMonetarySignPosition(),
                       KConfig::Persistent|KConfig::Global);

  i = entGrp.readEntry("MonetaryDigitSet", (int)KLocale::ArabicDigits);
  group.deleteEntry("MonetaryDigitSet", KConfig::Persistent | KConfig::Global);
  if (i != m_locale->monetaryDigitSet())
    group.writeEntry("MonetaryDigitSet",
                     (int)m_locale->monetaryDigitSet(),
                     KConfig::Persistent|KConfig::Global);

  group.sync();
}

void KLocaleConfigMoney::slotLocaleChanged()
{
  m_comboCurrencyCode->setCurrentIndex( m_comboCurrencyCode->findData( QVariant( m_locale->currencyCode() ) ) );

  //Create the list of Currency Symbols for the selected Currency Code
  m_comboCurrencySymbol->clear();
  QStringList currencySymbolList = m_locale->currency()->symbolList();
  foreach ( const QString &currencySymbol, currencySymbolList ) {
      m_comboCurrencySymbol->addItem( currencySymbol, QVariant( currencySymbol ) );
  }
  int i = m_comboCurrencySymbol->findData( QVariant( m_locale->currencySymbol() ) );
  if ( i < 0 ) {
      i = m_comboCurrencySymbol->findData( QVariant( m_locale->currency()->defaultSymbol() ) );
  }
  m_comboCurrencySymbol->setCurrentIndex( i );

  m_edMonDecSym->setText( m_locale->monetaryDecimalSymbol() );
  m_edMonThoSep->setText( m_locale->monetaryThousandsSeparator() );
  m_intMonetaryDecimalPlaces->setValue( m_locale->monetaryDecimalPlaces() );

  m_chMonPosPreCurSym->setChecked( m_locale->positivePrefixCurrencySymbol() );
  m_chMonNegPreCurSym->setChecked( m_locale->negativePrefixCurrencySymbol() );
  m_cmbMonPosMonSignPos->setCurrentIndex( m_locale->positiveMonetarySignPosition() );
  m_cmbMonNegMonSignPos->setCurrentIndex( m_locale->negativeMonetarySignPosition() );

  m_cmbMonDigSet->setCurrentIndex( m_locale->monetaryDigitSet() );
}

void KLocaleConfigMoney::slotCurrencyCodeChanged( int i )
{
    m_locale->setCurrencyCode( m_comboCurrencyCode->itemData( i ).toString() );

    //Create the list of Currency Symbols for the selected Currency Code
    m_comboCurrencySymbol->clear();
    QStringList currencySymbolList = m_locale->currency()->symbolList();
    foreach ( const QString &currencySymbol, currencySymbolList ) {
        m_comboCurrencySymbol->addItem( currencySymbol, QVariant( currencySymbol ) );
    }
    m_comboCurrencySymbol->setCurrentIndex( m_comboCurrencySymbol->findData( QVariant( m_locale->currency()->defaultSymbol() ) ) );
    slotCurrencySymbolChanged( m_comboCurrencySymbol->currentIndex() );

    emit localeChanged();
}

void KLocaleConfigMoney::slotCurrencySymbolChanged( int i )
{
    m_locale->setCurrencySymbol( m_comboCurrencySymbol->itemData( i ).toString() );
    emit localeChanged();
}

void KLocaleConfigMoney::slotMonDecSymChanged(const QString &t)
{
  m_locale->setMonetaryDecimalSymbol(t);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonThoSepChanged(const QString &t)
{
  m_locale->setMonetaryThousandsSeparator(t);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonetaryDecimalPlacesChanged(int value)
{
    m_locale->setMonetaryDecimalPlaces(value);
    emit localeChanged();
}

void KLocaleConfigMoney::slotMonPosPreCurSymChanged()
{
  m_locale->setPositivePrefixCurrencySymbol(m_chMonPosPreCurSym->isChecked());
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonNegPreCurSymChanged()
{
  m_locale->setNegativePrefixCurrencySymbol(m_chMonNegPreCurSym->isChecked());
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonPosMonSignPosChanged(int i)
{
  m_locale->setPositiveMonetarySignPosition((KLocale::SignPosition)i);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonNegMonSignPosChanged(int i)
{
  m_locale->setNegativeMonetarySignPosition((KLocale::SignPosition)i);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonDigSetChanged(int i)
{
  m_locale->setMonetaryDigitSet((KLocale::DigitSet)i);
  emit localeChanged();
}

void KLocaleConfigMoney::slotTranslate()
{
  //Create the list of Currency Codes to choose from.
  //Visible text will be localised name plus unlocalised code.
  m_comboCurrencyCode->clear();
  //First put all the preferred currencies first in order of priority
  QStringList currencyCodeList = m_locale->currencyCodeList();
  foreach ( const QString &currencyCode, currencyCodeList ) {
      QString text = i18nc( "@item currency name and currency code", "%1 (%2)",
                            m_locale->currency()->currencyCodeToName( currencyCode ), currencyCode );
    m_comboCurrencyCode->addItem( text, QVariant( currencyCode ) );
  }
  //Next put all currencies available in alphabetical name order
  m_comboCurrencyCode->insertSeparator(m_comboCurrencyCode->count());
  currencyCodeList = m_locale->currency()->allCurrencyCodesList();
  QStringList currencyNameList;
  foreach ( const QString &currencyCode, currencyCodeList ) {
    currencyNameList.append( i18nc( "@item currency name and currency code", "%1 (%2)",
                                    m_locale->currency()->currencyCodeToName( currencyCode ), currencyCode ) );
  }
  currencyNameList.sort();
  foreach ( const QString &name, currencyNameList ) {
    m_comboCurrencyCode->addItem( name, QVariant( name.mid( name.length()-4, 3 ) ) );
  }
  m_comboCurrencyCode->setCurrentIndex( m_comboCurrencyCode->findData( QVariant( m_locale->currencyCode() ) ) );

  //Create the list of Currency Symbols for the selected Currency Code
  m_comboCurrencySymbol->clear();
  QStringList currencySymbolList = m_locale->currency()->symbolList();
  foreach ( const QString &currencySymbol, currencySymbolList ) {
    m_comboCurrencySymbol->addItem( currencySymbol, QVariant( currencySymbol ) );
  }
  m_comboCurrencySymbol->setCurrentIndex( m_comboCurrencySymbol->findData( QVariant( m_locale->currencySymbol() ) ) );

  QList<QComboBox*> list;
  list.append(m_cmbMonPosMonSignPos);
  list.append(m_cmbMonNegMonSignPos);

  foreach (QComboBox* wc, list)
  {
    wc->setItemText(0, ki18n("Parentheses Around").toString(m_locale));
    wc->setItemText(1, ki18n("Before Quantity Money").toString(m_locale));
    wc->setItemText(2, ki18n("After Quantity Money").toString(m_locale));
    wc->setItemText(3, ki18n("Before Money").toString(m_locale));
    wc->setItemText(4, ki18n("After Money").toString(m_locale));
  }

  QList<KLocale::DigitSet> digitSets = m_locale->allDigitSetsList();
  qSort(digitSets);
  m_cmbMonDigSet->clear();
  foreach (const KLocale::DigitSet &ds, digitSets)
  {
    m_cmbMonDigSet->addItem(m_locale->digitSetToName(ds, true));
  }
  m_cmbMonDigSet->setCurrentIndex(m_locale->monetaryDigitSet());

  QString str;

  str = ki18n( "Here you can choose the currency to be used "
               "when displaying monetary values, e.g. United "
               "States Dollar or Pound Sterling." ).toString( m_locale );
  m_labelCurrencyCode->setWhatsThis( str );
  m_comboCurrencyCode->setWhatsThis( str );

  str = ki18n( "Here you can choose the currency symbol to be "
               "displayed for your chosen currency, e.g. $, US$, "
               "or USD." ).toString( m_locale );
  m_labelCurrencySymbol->setWhatsThis( str );
  m_comboCurrencySymbol->setWhatsThis( str );

  str = ki18n( "<p>Here you can define the decimal separator used "
               "to display monetary values.</p>"
               "<p>Note that the decimal separator used to "
               "display other numbers has to be defined "
               "separately (see the 'Numbers' tab).</p>" ).toString( m_locale );
  m_labMonDecSym->setWhatsThis( str );
  m_edMonDecSym->setWhatsThis( str );

  str = ki18n( "<p>Here you can define the thousands separator "
               "used to display monetary values.</p>"
               "<p>Note that the thousands separator used to "
               "display other numbers has to be defined "
               "separately (see the 'Numbers' tab).</p>" ).toString( m_locale );
  m_labMonThoSep->setWhatsThis( str );
  m_edMonThoSep->setWhatsThis( str );

  str = ki18n( "<p>Here you can set the number of decimal places "
               "displayed for monetary values, i.e. the number "
               "of digits <em>after</em> the decimal separator. "
               "<p>Note that the decimal places used to "
               "display other numbers has to be defined "
               "separately (see the 'Numbers' tab).</p>" ).toString( m_locale );
  m_labelMonetaryDecimalPlaces->setWhatsThis( str );
  m_intMonetaryDecimalPlaces->setWhatsThis( str );

  str = ki18n( "If this option is checked, the currency sign "
               "will be prefixed (i.e. to the left of the "
               "value) for all positive monetary values. If "
               "not, it will be postfixed (i.e. to the right)." ).toString( m_locale );
  m_chMonPosPreCurSym->setWhatsThis( str );

  str = ki18n( "If this option is checked, the currency sign "
               "will be prefixed (i.e. to the left of the "
               "value) for all negative monetary values. If "
               "not, it will be postfixed (i.e. to the right)." ).toString( m_locale );
  m_chMonNegPreCurSym->setWhatsThis( str );

  str = ki18n( "Here you can select how a positive sign will be "
               "positioned. This only affects monetary values." ).toString( m_locale );
  m_labMonPosMonSignPos->setWhatsThis( str );
  m_cmbMonPosMonSignPos->setWhatsThis( str );

  str = ki18n( "Here you can select how a negative sign will "
               "be positioned. This only affects monetary "
               "values." ).toString( m_locale );
  m_labMonNegMonSignPos->setWhatsThis( str );
  m_cmbMonNegMonSignPos->setWhatsThis( str );

  str = ki18n( "<p>Here you can define the set of digits "
               "used to display monetary values. "
               "If digits other than Arabic are selected, "
               "they will appear only if used in the language "
               "of the application or the piece of text "
               "where the number is shown.</p>"
               "<p>Note that the set of digits used to "
               "display other numbers has to be defined "
               "separately (see the 'Numbers' tab).</p>" ).toString( m_locale );
  m_labMonDigSet->setWhatsThis( str );
  m_cmbMonDigSet->setWhatsThis( str );

}
