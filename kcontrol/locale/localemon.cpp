/*
 * localemon.cpp
 *
 * Copyright (c) 1999-2001 Hans Petter Bieker <bieker@kde.org>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qobjectlist.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qvgroupbox.h>
#include <qvbox.h>
#include <qregexp.h>

#include <knuminput.h>
#include <kglobal.h>
#include <kdialog.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include "klocalesample.h"
#include "toplevel.h"
#include "localemon.h"
#include "localemon.moc"

KLocaleConfigMoney::KLocaleConfigMoney(KLocale *locale,
				       QWidget *parent, const char*name)
  : QWidget(parent, name),
    m_locale(locale)
{
  // Money
  QGridLayout *lay = new QGridLayout(this, 6, 2,
				     KDialog::marginHint(),
				     KDialog::spacingHint());

  m_labMonCurSym = new QLabel(this, I18N_NOOP("Currency symbol:"));
  lay->addWidget(m_labMonCurSym, 0, 0);
  m_edMonCurSym = new QLineEdit(this);
  lay->addWidget(m_edMonCurSym, 0, 1);
  connect( m_edMonCurSym, SIGNAL( textChanged(const QString &) ),
	   SLOT( slotMonCurSymChanged(const QString &) ) );
  
  m_labMonDecSym = new QLabel(this, I18N_NOOP("Decimal symbol:"));
  lay->addWidget(m_labMonDecSym, 1, 0);
  m_edMonDecSym = new QLineEdit(this);
  lay->addWidget(m_edMonDecSym, 1, 1);
  connect( m_edMonDecSym, SIGNAL( textChanged(const QString &) ),
	   SLOT( slotMonDecSymChanged(const QString &) ) );

  m_labMonThoSep = new QLabel(this, I18N_NOOP("Thousands separator:"));
  lay->addWidget(m_labMonThoSep, 2, 0);
  m_edMonThoSep = new QLineEdit(this);
  lay->addWidget(m_edMonThoSep, 2, 1);
  connect( m_edMonThoSep, SIGNAL( textChanged(const QString &) ),
	   SLOT( slotMonThoSepChanged(const QString &) ) );

  m_labMonFraDig = new QLabel(this, I18N_NOOP("Fract digits:"));
  lay->addWidget(m_labMonFraDig, 3, 0);
  m_inMonFraDig = new KIntNumInput(this);
  m_inMonFraDig->setRange(0, 10, 1, false);
  lay->addWidget(m_inMonFraDig, 3, 1);
  
  connect( m_inMonFraDig, SIGNAL( valueChanged(int) ),
	   SLOT( slotMonFraDigChanged(int) ) );

  QWidget *vbox = new QVBox(this);
  lay->addMultiCellWidget(vbox, 4, 4, 0, 1);
  QGroupBox *grp;
  grp = new QGroupBox( vbox, I18N_NOOP("Positive") );
  grp->setColumns(2);
  m_labMonPosPreCurSym = new QLabel(grp, I18N_NOOP("Prefix currency symbol:"));
  m_chMonPosPreCurSym = new QCheckBox(grp);
  connect( m_chMonPosPreCurSym, SIGNAL( clicked() ),
	   SLOT( slotMonPosPreCurSymChanged() ) );

  m_labMonPosMonSignPos = new QLabel(grp, I18N_NOOP("Sign position:"));
  m_cmbMonPosMonSignPos = new QComboBox(grp, "signpos");
  connect( m_cmbMonPosMonSignPos, SIGNAL( activated(int) ),
	   SLOT( slotMonPosMonSignPosChanged(int) ) );

  grp = new QGroupBox( vbox, I18N_NOOP("Negative") );
  grp->setColumns(2);
  m_labMonNegPreCurSym = new QLabel(grp, I18N_NOOP("Prefix currency symbol:"));
  m_chMonNegPreCurSym = new QCheckBox(grp);
  connect( m_chMonNegPreCurSym, SIGNAL( clicked() ),
	   SLOT( slotMonNegPreCurSymChanged() ) );

  m_labMonNegMonSignPos = new QLabel(grp, I18N_NOOP("Sign position:"));
  m_cmbMonNegMonSignPos = new QComboBox(grp, "signpos");
  connect( m_cmbMonNegMonSignPos, SIGNAL( activated(int) ),
	   SLOT( slotMonNegMonSignPosChanged(int) ) );

  // insert some items
  int i = 5;
  while (i--)
    {
      m_cmbMonPosMonSignPos->insertItem(QString::null);
      m_cmbMonNegMonSignPos->insertItem(QString::null);
    }

  lay->setColStretch(1, 1);
  lay->addRowSpacing(5, 0);

  adjustSize();
}

KLocaleConfigMoney::~KLocaleConfigMoney()
{
}

void KLocaleConfigMoney::save()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver saver(config, "Locale");

  KSimpleConfig ent(locate("locale",
			   QString::fromLatin1("l10n/%1/entry.desktop")
			   .arg(m_locale->country())), true);
  ent.setGroup("KCM Locale");

  QString str;
  int i;
  bool b;

  str = ent.readEntry("CurrencySymbol", QString::fromLatin1("$"));
  config->deleteEntry("CurrencySymbol", false, true);
  if (str != m_locale->currencySymbol())
    config->writeEntry("CurrencySymbol",
		       m_locale->currencySymbol(), true, true);

  str = ent.readEntry("MonetaryDecimalSymbol", QString::fromLatin1("."));
  config->deleteEntry("MonetaryDecimalSymbol", false, true);
  if (str != m_locale->monetaryDecimalSymbol())
    config->writeEntry("MonetaryDecimalSymbol",
		       m_locale->monetaryDecimalSymbol(), true, true);

  str = ent.readEntry("MonetaryThousandsSeparator", QString::fromLatin1(","));
  str.replace(QRegExp(QString::fromLatin1("$0")), QString::null);
  config->deleteEntry("MonetaryThousandsSeparator", false, true);
  if (str != m_locale->monetaryThousandsSeparator())
    config->writeEntry("MonetaryThousandsSeparator",
		       QString::fromLatin1("$0%1$0")
		       .arg(m_locale->monetaryThousandsSeparator()),
		       true, true);

  i = ent.readNumEntry("FracDigits", 2);
  config->deleteEntry("FracDigits", false, true);
  if (i != m_locale->fracDigits())
    config->writeEntry("FracDigits", m_locale->fracDigits(), true, true);

  b = ent.readNumEntry("PositivePrefixCurrencySymbol", true);
  config->deleteEntry("PositivePrefixCurrencySymbol", false, true);
  if (b != m_locale->positivePrefixCurrencySymbol())
    config->writeEntry("PositivePrefixCurrencySymbol",
		       m_locale->positivePrefixCurrencySymbol(), true, true);

  b = ent.readNumEntry("NegativePrefixCurrencySymbol", true);
  config->deleteEntry("NegativePrefixCurrencySymbol", false, true);
  if (b != m_locale->negativePrefixCurrencySymbol())
    config->writeEntry("NegativePrefixCurrencySymbol",
		       m_locale->negativePrefixCurrencySymbol(), true, true);

  i = ent.readNumEntry("PositiveMonetarySignPosition",
		       (int)KLocale::BeforeQuantityMoney);
  config->deleteEntry("PositiveMonetarySignPosition", false, true);
  if (i != m_locale->positiveMonetarySignPosition())
    config->writeEntry("PositiveMonetarySignPosition",
		       (int)m_locale->positiveMonetarySignPosition(),
		       true, true);

  i = ent.readNumEntry("NegativeMonetarySignPosition",
		       (int)KLocale::ParensAround);
  config->deleteEntry("NegativeMonetarySignPosition", false, true);
  if (i != m_locale->negativeMonetarySignPosition())
    config->writeEntry("NegativeMonetarySignPosition",
		       (int)m_locale->negativeMonetarySignPosition(),
		       true, true);

  config->sync();
}

void KLocaleConfigMoney::slotLocaleChanged()
{
  m_edMonCurSym->setText( m_locale->currencySymbol() );
  m_edMonDecSym->setText( m_locale->monetaryDecimalSymbol() );
  m_edMonThoSep->setText( m_locale->monetaryThousandsSeparator() );
  m_inMonFraDig->setValue( m_locale->fracDigits() );

  m_chMonPosPreCurSym->setChecked( m_locale->positivePrefixCurrencySymbol() );
  m_chMonNegPreCurSym->setChecked( m_locale->negativePrefixCurrencySymbol() );
  m_cmbMonPosMonSignPos->setCurrentItem( m_locale->positiveMonetarySignPosition() );
  m_cmbMonNegMonSignPos->setCurrentItem( m_locale->negativeMonetarySignPosition() );
}

void KLocaleConfigMoney::slotMonCurSymChanged(const QString &t)
{
  m_locale->setCurrencySymbol(t);
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

void KLocaleConfigMoney::slotMonFraDigChanged(int value)
{
  m_locale->setFracDigits(value);
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

void KLocaleConfigMoney::slotTranslate()
{
  QObjectList list;
  list.append(m_cmbMonPosMonSignPos);
  list.append(m_cmbMonNegMonSignPos);

  QComboBox *wc;
  for(QObjectListIt li(list) ; (wc = (QComboBox *)li.current()) != 0; ++li)
  {
    wc->changeItem(m_locale->translate("Parens around"), 0);
    wc->changeItem(m_locale->translate("Before quantity money"), 1);
    wc->changeItem(m_locale->translate("After quantity money"), 2);
    wc->changeItem(m_locale->translate("Before money"), 3);
    wc->changeItem(m_locale->translate("After money"), 4);
  }

  QString str;

  str = m_locale->translate( "Here you can enter your usual currency "
			     "symbol, e.g. $ or DM."
			     "<p>Please note that the Euro symbol may not be "
			     "avaim_lable on your system, depending on the "
			     "distribution you use." );
  QWhatsThis::add( m_labMonCurSym, str );
  QWhatsThis::add( m_edMonCurSym, str );
  str = m_locale->translate( "Here you can define the decimal separator used "
			   "to display monetary values."
			   "<p>Note that the decimal separator used to "
			   "display other numbers has to be defined "
			   "separately (see the 'Numbers' tab)." );
  QWhatsThis::add( m_labMonDecSym, str );
  QWhatsThis::add( m_edMonDecSym, str );

  str = m_locale->translate( "Here you can define the thousands separator "
			   "used to display monetary values."
			   "<p>Note that the thousands separator used to "
			   "display other numbers has to be defined "
			   "separately (see the 'Numbers' tab)." );
  QWhatsThis::add( m_labMonThoSep, str );
  QWhatsThis::add( m_edMonThoSep, str );

  str = m_locale->translate( "This determines the number of fract digits for "
			   "monetary values, i.e. the number of digits you "
			   "find <em>behind</em> the decimal separator. "
			   "Correct value is 2 for almost all people." );
  QWhatsThis::add( m_labMonFraDig, str );
  QWhatsThis::add( m_inMonFraDig, str );

  str = m_locale->translate( "If this option is checked, the currency sign "
			   "will be prefixed (i.e. to the left of the "
			   "value) for all positive monetary values. If "
			   "not, it will be postfixed (i.e. to the right)." );
  QWhatsThis::add( m_labMonPosPreCurSym, str );
  QWhatsThis::add( m_chMonPosPreCurSym, str );

  str = m_locale->translate( "If this option is checked, the currency sign "
			   "will be prefixed (i.e. to the left of the "
			   "value) for all negative monetary values. If "
			   "not, it will be postfixed (i.e. to the right)." );
  QWhatsThis::add( m_labMonNegPreCurSym, str );
  QWhatsThis::add( m_chMonNegPreCurSym, str );

  str = m_locale->translate( "Here you can select how a positive sign will be "
			   "positioned. This only affects monetary values." );
  QWhatsThis::add( m_labMonPosMonSignPos, str );
  QWhatsThis::add( m_cmbMonPosMonSignPos, str );

  str = m_locale->translate( "Here you can select how a negative sign will "
			   "be positioned. This only affects monetary "
			   "values." );
  QWhatsThis::add( m_labMonNegMonSignPos, str );
  QWhatsThis::add( m_cmbMonNegMonSignPos, str );
}
