/*
 * klocalesample.cpp
 *
 * Copyright (c) 1998 Matthias Hoelzer (hoelzer@physik.uni-wuerzburg.de)
 * Copyright (c) 1999 Preston Brown <pbrown@kde.org>
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

#include "klocalesample.h"

#include <QDateTime>
#include <QLabel>
#include <QFormLayout>
#include <QTimer>

#include <stdio.h>

#include <klocale.h>
#include <kcalendarsystem.h>

#include "klocalesample.moc"

KLocaleSample::KLocaleSample(KLocale *locale, QWidget *parent)
  : QWidget(parent),
    m_locale(locale)
{
  QFormLayout *lay = new QFormLayout(this);

  m_labNumber = new QLabel(this);
  m_labNumber->setObjectName( QLatin1String( I18N_NOOP("Numbers:" )) );
  m_numberSample = new QLabel(this);
  lay->addRow(m_labNumber, m_numberSample);

  m_labMoney = new QLabel(this);
  m_labMoney->setObjectName( QLatin1String( I18N_NOOP("Money:" )) );
  m_moneySample = new QLabel(this);
  lay->addRow(m_labMoney, m_moneySample);

  m_labDate = new QLabel(this);
  m_labDate->setObjectName( QLatin1String( I18N_NOOP("Date:" )) );
  m_dateSample = new QLabel(this);
  lay->addRow(m_labDate, m_dateSample);

  m_labDateShort = new QLabel(this);
  m_labDateShort->setObjectName( QLatin1String( I18N_NOOP("Short date:" )) );
  m_dateShortSample = new QLabel(this);
  lay->addRow(m_labDateShort, m_dateShortSample);

  m_labTime = new QLabel(this);
  m_labTime->setObjectName( QLatin1String( I18N_NOOP("Time:" )) );
  m_timeSample = new QLabel(this);
  lay->addRow(m_labTime, m_timeSample);

  QTimer *timer = new QTimer(this);
  timer->setObjectName(QLatin1String("clock_timer"));
  connect(timer, SIGNAL(timeout()), this, SLOT(slotUpdateTime()));
  timer->start(1000);
}

KLocaleSample::~KLocaleSample()
{
}

void KLocaleSample::slotUpdateTime()
{
  QDateTime dt = QDateTime::currentDateTime();

  m_dateSample->setText(m_locale->calendar()->formatDate(dt.date(), KLocale::LongDate));
  m_dateShortSample->setText(m_locale->calendar()->formatDate(dt.date(), KLocale::ShortDate));
  m_timeSample->setText(m_locale->formatTime(dt.time(), true));
}

void KLocaleSample::slotLocaleChanged()
{
  m_numberSample->setText(m_locale->formatNumber(1234567.89) +
                          QLatin1String(" / ") +
                          m_locale->formatNumber(-1234567.89));

  m_moneySample->setText(m_locale->formatMoney(123456789.00) +
                         QLatin1String(" / ") +
                         m_locale->formatMoney(-123456789.00));

  slotUpdateTime();

  QString str;

  str = ki18n("This is how numbers will be displayed.").toString(m_locale);
  m_labNumber->setWhatsThis( str );
  m_numberSample->setWhatsThis( str );

  str = ki18n("This is how monetary values will be displayed.").toString(m_locale);
  m_labMoney->setWhatsThis( str );
  m_moneySample->setWhatsThis( str );

  str = ki18n("This is how date values will be displayed.").toString(m_locale);
  m_labDate->setWhatsThis( str );
  m_dateSample->setWhatsThis( str );

  str = ki18n("This is how date values will be displayed using "
              "a short notation.").toString(m_locale);
  m_labDateShort->setWhatsThis( str );
  m_dateShortSample->setWhatsThis( str );

  str = ki18n("This is how the time will be displayed.").toString(m_locale);
  m_labTime->setWhatsThis( str );
  m_timeSample->setWhatsThis( str );
}
