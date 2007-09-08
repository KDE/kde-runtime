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

#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QTimer>

#include <stdio.h>

#include <klocale.h>

#include "klocalesample.h"
#include "klocalesample.moc"

KLocaleSample::KLocaleSample(KLocale *locale, QWidget *parent)
  : QWidget(parent),
    m_locale(locale)
{
  QGridLayout *lay = new QGridLayout(this);

  // Whatever the color scheme is, we want black text
  QPalette pal = palette();
  pal.setColor(QPalette::Disabled, QPalette::WindowText, Qt::black);
  pal.setColor(QPalette::Active, QPalette::WindowText, Qt::black);
  pal.setColor(QPalette::Inactive, QPalette::WindowText, Qt::black);
  setPalette(pal);

  m_labNumber = new QLabel(this);
  lay->addWidget(m_labNumber, 0, 0);
  m_labNumber->setObjectName( I18N_NOOP("Numbers:") );
  m_labNumber->setPalette(pal);
  m_numberSample = new QLabel(this);
  m_numberSample->setPalette(pal);
  lay->addWidget(m_numberSample, 0, 1);

  m_labMoney = new QLabel(this);
  lay->addWidget(m_labMoney, 1, 0);
  m_labMoney->setObjectName( I18N_NOOP("Money:") );
  m_labMoney->setPalette(pal);
  m_moneySample = new QLabel(this);
  m_moneySample->setPalette(pal);
  lay->addWidget(m_moneySample, 1, 1);

  m_labDate = new QLabel(this);
  lay->addWidget(m_labDate, 2, 0);
  m_labDate->setObjectName( I18N_NOOP("Date:") );
  m_labDate->setPalette(pal);
  m_dateSample = new QLabel(this);
  m_dateSample->setPalette(pal);
  lay->addWidget(m_dateSample, 2, 1);

  m_labDateShort = new QLabel(this);
  lay->addWidget(m_labDateShort, 3, 0);
  m_labDateShort->setObjectName( I18N_NOOP("Short date:") );
  m_labDateShort->setPalette(pal);
  m_dateShortSample = new QLabel(this);
  m_dateShortSample->setPalette(pal);
  lay->addWidget(m_dateShortSample, 3, 1);

  m_labTime = new QLabel(this);
  lay->addWidget(m_labTime, 4, 0);
  m_labTime->setObjectName( I18N_NOOP("Time:") );
  m_labTime->setPalette(pal);
  m_timeSample = new QLabel(this);
  lay->addWidget(m_timeSample, 4, 1);
  m_timeSample->setPalette(pal);

  lay->setColumnStretch(1, 3);

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

  m_dateSample->setText(m_locale->formatDate(dt.date(), KLocale::LongDate));
  m_dateShortSample->setText(m_locale->formatDate(dt.date(), KLocale::ShortDate));
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
