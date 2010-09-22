/*
 * localetime.cpp
 *
 * Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>
 * Copyright (c) 2008, 2010 John Layt <john@layt.net>
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

#include "localetime.h"

#include <QCheckBox>
#include <QLabel>
#include <QFormLayout>
#include <QComboBox>
#include <QGroupBox>

#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KDebug>
#include <KCalendarSystem>

#include "toplevel.h"
#include "localetime.moc"

QMap<QString, QString> KLocaleConfigTime::timeMap() const
{
    QMap<QString, QString> map;

    map.insert( QString( 'H' ), ki18n( "HH" ).toString( m_locale ) );
    map.insert( QString( 'k' ), ki18n( "hH" ).toString( m_locale ) );
    map.insert( QString( 'I' ), ki18n( "PH" ).toString( m_locale ) );
    map.insert( QString( 'l' ), ki18n( "pH" ).toString( m_locale ) );
    map.insert( QString( 'M' ), ki18nc( "Minute", "MM" ).toString( m_locale ) );
    map.insert( QString( 'S' ), ki18n( "SS" ).toString( m_locale ) );
    map.insert( QString( 'p' ), ki18n( "AMPM" ).toString( m_locale ) );

    return map;
}

QMap<QString, QString> KLocaleConfigTime::dateMap() const
{
    QMap<QString, QString> map;

    map.insert( QString( 'Y' ), ki18n("YYYY").toString( m_locale ) );
    map.insert( QString( 'y' ), ki18n( "YY" ).toString( m_locale ) );
    map.insert( QString( 'n' ), ki18n( "mM" ).toString( m_locale ) );
    map.insert( QString( 'm' ), ki18nc( "Month", "MM" ).toString( m_locale ) );
    map.insert( QString( 'b' ), ki18n( "SHORTMONTH" ).toString( m_locale ) );
    map.insert( QString( 'B' ), ki18n( "MONTH" ).toString( m_locale ) );
    map.insert( QString( 'e' ), ki18n( "dD" ).toString( m_locale ) );
    map.insert( QString( 'd' ), ki18n( "DD" ).toString( m_locale ) );
    map.insert( QString( 'a' ), ki18n( "SHORTWEEKDAY" ).toString( m_locale ) );
    map.insert( QString( 'A' ), ki18n( "WEEKDAY" ).toString( m_locale ) );
    map.insert( "EY", ki18n( "ERAYEAR" ).toString( m_locale ) );
    map.insert( "Ey", ki18n( "YEARINERA" ).toString( m_locale ) );
    map.insert( "EC", ki18n( "SHORTERANAME" ).toString( m_locale ) );
    map.insert( QString( 'j' ), ki18n( "DAYOFYEAR" ).toString( m_locale ) );
    map.insert( QString( 'V' ), ki18n( "ISOWEEK" ).toString( m_locale ) );
    map.insert( QString( 'u' ), ki18n( "DAYOFISOWEEK" ).toString( m_locale ) );

    return map;
}

QString KLocaleConfigTime::userToStore( const QMap<QString, QString> &map, const QString &userFormat ) const
{
    QString result;

    for ( int pos = 0; pos < userFormat.length(); ++pos ) {
        bool matchFound = false;
        QMap<QString, QString>::const_iterator it = map.constBegin();
        while ( it != map.constEnd() && !matchFound ) {
            QString s = it.value();

            if ( userFormat.mid( pos, s.length() ) == s ) {
                result += '%';
                result += it.key();
                pos += s.length() - 1;
                matchFound = true;
            }
            ++it;
        }

        if ( !matchFound ) {
            QChar c = userFormat.at( pos );
            if ( c == '%' ) {
                result += c;
            }
            result += c;
        }
    }

    return result;
}

QString KLocaleConfigTime::storeToUser( const QMap<QString, QString> &map, const QString &storeFormat ) const
{
    QString result;

    bool escaped = false;
    for ( int pos = 0; pos < storeFormat.length(); ++pos ) {
        QChar c = storeFormat.at( pos );
        if ( escaped ) {
            QString key = c;
            if ( c == 'E' ) {
                key += storeFormat.at( ++pos );
            }
            QString val = map.value( key, QString() );
            if ( !val.isEmpty() ) {
                result += val;
            } else {
                result += key;
            }
            escaped = false;
        } else if ( c == '%' ) {
            escaped = true;
        } else {
            result += c;
        }
    }

    return result;
}

KLocaleConfigTime::KLocaleConfigTime( KLocale *_locale, KSharedConfigPtr config, QWidget *parent )
                 : QWidget(parent),
                   m_locale(_locale),
                   m_config(config)
{
    // Time
    QFormLayout *lay = new QFormLayout( this );

    QLabel* labCalendarSystem = new QLabel(this);
    labCalendarSystem->setObjectName( QLatin1String( I18N_NOOP("Calendar system:" )) );
    m_comboCalendarSystem = new QComboBox(this);
    lay->addRow(labCalendarSystem, m_comboCalendarSystem);
    m_comboCalendarSystem->setEditable(false);
    connect(m_comboCalendarSystem, SIGNAL(activated(int)),
            this, SLOT(slotCalendarSystemChanged(int)));
    QStringList tmpCalendars;
    tmpCalendars << QString() << QString();
    m_comboCalendarSystem->addItems(tmpCalendars);

    QLabel *labelUseCommonEra = new QLabel(this);
    m_checkUseCommonEra = new QCheckBox(this);
    m_checkUseCommonEra->setObjectName( QLatin1String( I18N_NOOP("Use Common Era" )) );
    lay->addRow(labelUseCommonEra, m_checkUseCommonEra);
    connect(m_checkUseCommonEra, SIGNAL(clicked()),
            this,                SLOT(slotUseCommonEraChanged()));

    m_labDateTimeDigSet = new QLabel(this);
    m_labDateTimeDigSet->setObjectName( QLatin1String( I18N_NOOP("Di&git set:" )) );
    m_comboDateTimeDigSet = new QComboBox(this);
    lay->addRow(m_labDateTimeDigSet, m_comboDateTimeDigSet);
    connect( m_comboDateTimeDigSet, SIGNAL( activated(int) ),
             this, SLOT( slotDateTimeDigSetChanged(int) ) );
    m_labDateTimeDigSet->setBuddy( m_comboDateTimeDigSet );

    QGroupBox *fGr = new QGroupBox(/*("Format"),*/this);
    QFormLayout *fLay = new QFormLayout( fGr );

    m_labTimeFmt = new QLabel(this);
    m_labTimeFmt->setObjectName( QLatin1String( I18N_NOOP("Time format:" )) );
    m_comboTimeFmt = new QComboBox(this);
    fLay->addRow(m_labTimeFmt,m_comboTimeFmt);
    m_comboTimeFmt->setEditable(true);
    connect( m_comboTimeFmt, SIGNAL( editTextChanged(const QString &) ),
             this, SLOT( slotTimeFmtChanged(const QString &) ) );

    m_labDateFmt = new QLabel(this);
    m_labDateFmt->setObjectName( QLatin1String( I18N_NOOP("Date format:" )) );
    m_comboDateFmt = new QComboBox(this);
    fLay->addRow(m_labDateFmt,m_comboDateFmt);
    m_comboDateFmt->setEditable(true);
    connect( m_comboDateFmt, SIGNAL( editTextChanged(const QString &) ),
             this, SLOT( slotDateFmtChanged(const QString &) ) );

    m_labDateFmtShort = new QLabel(this);
    m_labDateFmtShort->setObjectName( QLatin1String( I18N_NOOP("Short date format:" )) );
    m_comboDateFmtShort = new QComboBox(this);
    fLay->addRow(m_labDateFmtShort,m_comboDateFmtShort);
    m_comboDateFmtShort->setEditable(true);
    connect( m_comboDateFmtShort, SIGNAL( editTextChanged(const QString &) ),
             this, SLOT( slotDateFmtShortChanged(const QString &) ) );

    m_chDateMonthNamePossessive = new QCheckBox(this);
    m_chDateMonthNamePossessive->setObjectName( QLatin1String(I18N_NOOP("Use declined form of month name" )));
    fLay->addWidget(m_chDateMonthNamePossessive);
    connect( m_chDateMonthNamePossessive, SIGNAL(clicked()),
             this,                        SLOT(slotDateMonthNamePossChanged()));


    QGroupBox *wGr = new QGroupBox(/*("Week"),*/this);
    QFormLayout *wLay = new QFormLayout( wGr );

    QLabel* labWeekStartDay = new QLabel(this);
    labWeekStartDay->setObjectName( QLatin1String( I18N_NOOP("First day of the week:" )) );
    m_comboWeekStartDay = new QComboBox(this);
    wLay->addRow(labWeekStartDay,m_comboWeekStartDay);
    m_comboWeekStartDay->setEditable(false);
    connect (m_comboWeekStartDay, SIGNAL(activated(int)),
             this, SLOT(slotWeekStartDayChanged(int)));

    QLabel* labWorkingWeekStartDay = new QLabel(this);
    labWorkingWeekStartDay->setObjectName( QLatin1String( I18N_NOOP("First working day of the week:" )) );
    m_comboWorkingWeekStartDay = new QComboBox(this);
    wLay->addRow(labWorkingWeekStartDay,m_comboWorkingWeekStartDay);
    m_comboWorkingWeekStartDay->setEditable(false);
    connect (m_comboWorkingWeekStartDay, SIGNAL(activated(int)),
             this, SLOT(slotWorkingWeekStartDayChanged(int)));

    QLabel* labWorkingWeekEndDay = new QLabel(this);
    labWorkingWeekEndDay->setObjectName( QLatin1String( I18N_NOOP("Last working day of the week:" )) );
    m_comboWorkingWeekEndDay = new QComboBox(this);
    wLay->addRow(labWorkingWeekEndDay,m_comboWorkingWeekEndDay);
    m_comboWorkingWeekEndDay->setEditable(false);
    connect (m_comboWorkingWeekEndDay, SIGNAL(activated(int)),
             this, SLOT(slotWorkingWeekEndDayChanged(int)));


    QLabel* labWeekDayOfPray = new QLabel(this);
    labWeekDayOfPray->setObjectName( QLatin1String( I18N_NOOP("Day of the week for religious observance:" )) );
    m_comboWeekDayOfPray = new QComboBox(this);
    wLay->addRow(labWeekDayOfPray,m_comboWeekDayOfPray);
    m_comboWeekDayOfPray->setEditable(false);
    connect (m_comboWeekDayOfPray, SIGNAL(activated(int)),
             this, SLOT(slotWeekDayOfPrayChanged(int)));

    QHBoxLayout * horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget( fGr );
    horizontalLayout->addWidget( wGr );

    lay->addRow( horizontalLayout );

    updateWeekDayNames();
    updateDigitSetNames();
}


KLocaleConfigTime::~KLocaleConfigTime()
{
}

void KLocaleConfigTime::save()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group(config, "Locale");
    KConfig ent(KStandardDirs::locate("locale",
        QString::fromLatin1("l10n/%1/entry.desktop")
        .arg(m_locale->country())));
    ent.setLocale(m_locale->language());
    KConfigGroup entGrp = ent.group("KCM Locale");

    QString str;

    str = entGrp.readEntry("CalendarSystem", QString::fromLatin1("gregorian"));
    group.deleteEntry("CalendarSystem", KConfig::Persistent | KConfig::Global);
    if (str != m_locale->calendarType())
      group.writeEntry("CalendarSystem", m_locale->calendarType(), KConfig::Persistent|KConfig::Global);

    if ( m_locale->calendarType() == "gregorian" ||
         m_locale->calendarType() == "gregorian-proleptic" ||
         m_locale->calendarType() == "julian" ) {
        KConfigGroup calendarGroup = ent.group( QString( "KCalendarSystem %1" ).arg( m_locale->calendarType() ) );
        if ( m_checkUseCommonEra->isChecked() ) {
            calendarGroup.writeEntry( "UseCommonEra", true, KConfig::Persistent | KConfig::Global );
        } else {
            calendarGroup.deleteEntry( "UseCommonEra", KConfig::Persistent | KConfig::Global );
        }
    }

    str = entGrp.readEntry("TimeFormat", QString::fromLatin1("%H:%M:%S"));
    group.deleteEntry("TimeFormat", KConfig::Persistent | KConfig::Global);
    if (str != m_locale->timeFormat())
      group.writeEntry("TimeFormat", m_locale->timeFormat(), KConfig::Persistent|KConfig::Global);

    str = entGrp.readEntry("DateFormat", QString::fromLatin1("%A %d %B %Y"));
    group.deleteEntry("DateFormat", KConfig::Persistent | KConfig::Global);
    if (str != m_locale->dateFormat())
      group.writeEntry("DateFormat", m_locale->dateFormat(), KConfig::Persistent|KConfig::Global);

    str = entGrp.readEntry("DateFormatShort", QString::fromLatin1("%Y-%m-%d"));
    group.deleteEntry("DateFormatShort", KConfig::Persistent | KConfig::Global);
    if (str != m_locale->dateFormatShort())
      group.writeEntry("DateFormatShort",
            m_locale->dateFormatShort(), KConfig::Persistent|KConfig::Global);

    int dateTimeDigSet;
    dateTimeDigSet = entGrp.readEntry("DateTimeDigitSet", (int) KLocale::ArabicDigits);
    group.deleteEntry("DateTimeDigitSet", KConfig::Persistent | KConfig::Global);
    if (dateTimeDigSet != m_locale->dateTimeDigitSet())
        group.writeEntry("DateTimeDigitSet", (int) m_locale->dateTimeDigitSet(), KConfig::Persistent|KConfig::Global);

    int firstDay;
    firstDay = entGrp.readEntry("WeekStartDay", 1);  //default to Monday
    group.deleteEntry("WeekStartDay", KConfig::Persistent | KConfig::Global);
    if (firstDay != m_locale->weekStartDay())
        group.writeEntry("WeekStartDay", m_locale->weekStartDay(), KConfig::Persistent|KConfig::Global);

    int firstWorkingDay;
    firstWorkingDay = entGrp.readEntry("WorkingWeekStartDay", 1);  //default to Monday
    group.deleteEntry("WorkingWeekStartDay", KConfig::Persistent | KConfig::Global);
    if (firstWorkingDay != m_locale->workingWeekStartDay())
        group.writeEntry("WorkingWeekStartDay", m_locale->workingWeekStartDay(), KConfig::Persistent|KConfig::Global);

    int lastWorkingDay;
    lastWorkingDay = entGrp.readEntry("WorkingWeekEndDay", 5);  //default to Friday
    group.deleteEntry("WorkingWeekEndDay", KConfig::Persistent | KConfig::Global);
    if (lastWorkingDay != m_locale->workingWeekEndDay())
        group.writeEntry("WorkingWeekEndDay", m_locale->workingWeekEndDay(), KConfig::Persistent|KConfig::Global);

    int prayDay;
    prayDay = entGrp.readEntry("WeekDayOfPray", 7);  //default to Sunday
    group.deleteEntry("WeekDayOfPray", KConfig::Persistent | KConfig::Global);
    if (prayDay != m_locale->weekDayOfPray())
        group.writeEntry("WeekDayOfPray", m_locale->weekDayOfPray(), KConfig::Persistent|KConfig::Global);

    bool b;
    b = entGrp.readEntry("DateMonthNamePossessive", false);
    group.deleteEntry("DateMonthNamePossessive", KConfig::Persistent | KConfig::Global);
    if (b != m_locale->dateMonthNamePossessive())
        group.writeEntry("DateMonthNamePossessive",
                        m_locale->dateMonthNamePossessive(), KConfig::Persistent|KConfig::Global);

    group.sync();
}

void KLocaleConfigTime::showEvent( QShowEvent *e )
{
    // This option makes sense only for languages where nouns are declined
    if ( !m_locale->nounDeclension() )
        m_chDateMonthNamePossessive->hide();
    QWidget::showEvent( e );
}

void KLocaleConfigTime::slotCalendarSystemChanged( int calendarSystem )
{
    if ( calendarSystem >= 0 && calendarSystem < m_comboCalendarSystem->count() ) {
        m_locale->setCalendar( m_comboCalendarSystem->itemData( calendarSystem ).toString() );
        if ( m_locale->calendarType() == "gregorian" ||
            m_locale->calendarType() == "gregorian-proleptic" ||
            m_locale->calendarType() == "julian" ) {
            m_checkUseCommonEra->setEnabled( true );
            KConfigGroup group( KGlobal::config(), "Locale" );
            KConfig entry( KStandardDirs::locate( "locale",
                           QString::fromLatin1( "l10n/%1/entry.desktop" ).arg( m_locale->country() ) ) );
            entry.setLocale( m_locale->language() );
            KConfigGroup calendarGroup = entry.group( QString( "KCalendarSystem %1" ).arg( m_locale->calendarType() ) );
            m_checkUseCommonEra->setChecked( calendarGroup.readEntry( "UseCommonEra", false ) );
        } else {
            m_checkUseCommonEra->setEnabled( false );
            m_checkUseCommonEra->setChecked( false );
        }
        updateWeekDayNames();
        emit localeChanged();
    }
}

void KLocaleConfigTime::slotUseCommonEraChanged()
{
    KConfigGroup calendarGroup( m_config, QString( "KCalendarSystem %1" ).arg( m_locale->calendarType() ) );

    if ( m_checkUseCommonEra->isChecked() ) {
        calendarGroup.writeEntry( "UseCommonEra", true );
    } else {
        calendarGroup.deleteEntry( "UseCommonEra" );
    }

    emit localeChanged();
}

void KLocaleConfigTime::slotLocaleChanged()
{
    updateCalendarNames();
    updateWeekDayNames();
    updateDigitSetNames();

    //  m_edTimeFmt->setText( m_locale->timeFormat() );
    m_comboTimeFmt->setEditText( storeToUser( timeMap(), m_locale->timeFormat() ) );
    // m_edDateFmt->setText( m_locale->dateFormat() );
    m_comboDateFmt->setEditText( storeToUser( dateMap(), m_locale->dateFormat() ) );
    //m_edDateFmtShort->setText( m_locale->dateFormatShort() );
    m_comboDateFmtShort->setEditText( storeToUser( dateMap(), m_locale->dateFormatShort() ) );

    if ( m_locale->nounDeclension() )
        m_chDateMonthNamePossessive->setChecked( m_locale->dateMonthNamePossessive() );

    kDebug(173) << "converting: " << m_locale->timeFormat();
    kDebug(173) << storeToUser(timeMap(), m_locale->timeFormat()) << endl;
    kDebug(173) << userToStore(timeMap(), QString::fromLatin1("HH:MM:SS AMPM test")) << endl;

}

void KLocaleConfigTime::slotTimeFmtChanged(const QString &t)
{
  //  m_locale->setTimeFormat(t);
    m_locale->setTimeFormat( userToStore( timeMap(), t ) );

    emit localeChanged();
}

void KLocaleConfigTime::slotDateFmtChanged(const QString &t)
{
    // m_locale->setDateFormat(t);
    m_locale->setDateFormat( userToStore( dateMap(), t ) );
    emit localeChanged();
}

void KLocaleConfigTime::slotDateFmtShortChanged(const QString &t)
{
    //m_locale->setDateFormatShort(t);
    m_locale->setDateFormatShort( userToStore( dateMap(), t ) );
    emit localeChanged();
}

void KLocaleConfigTime::slotDateTimeDigSetChanged(int i)
{
    m_locale->setDateTimeDigitSet((KLocale::DigitSet) i);
    emit localeChanged();
}

void KLocaleConfigTime::slotWeekStartDayChanged(int firstDay) {
    kDebug(173) << "first day is now: " << firstDay;
    m_locale->setWeekStartDay(m_comboWeekStartDay->currentIndex() + 1);
    emit localeChanged();
}

void KLocaleConfigTime::slotWorkingWeekStartDayChanged(int startDay) {
    kDebug(173) << "first working day is now: " << startDay;
    m_locale->setWorkingWeekStartDay(m_comboWorkingWeekStartDay->currentIndex() + 1);
    emit localeChanged();
}

void KLocaleConfigTime::slotWorkingWeekEndDayChanged(int endDay) {
    kDebug(173) << "last working day is now: " << endDay;
    m_locale->setWorkingWeekEndDay(m_comboWorkingWeekEndDay->currentIndex() + 1);
    emit localeChanged();
}

void KLocaleConfigTime::slotWeekDayOfPrayChanged(int prayDay) {
    kDebug(173) << "day of pray is now: " << prayDay;
    m_locale->setWeekDayOfPray(m_comboWeekDayOfPray->currentIndex());  // First option is None=0
    emit localeChanged();
}

void KLocaleConfigTime::slotDateMonthNamePossChanged()
{
    if (m_locale->nounDeclension())
    {
      m_locale->setDateMonthNamePossessive(m_chDateMonthNamePossessive->isChecked());
      emit localeChanged();
    }
}

void KLocaleConfigTime::slotTranslate()
{
  // Obtain the currently selected country's locale file
  KConfig localeFile(KStandardDirs::locate("locale", QString::fromLatin1("l10n/%1/entry.desktop").arg(m_locale->country())));
  localeFile.setLocale(m_locale->language());
  KConfigGroup localeGroup(&localeFile, "KCM Locale");

  QString str;
  QStringList formatList;

  QString sep = QString::fromLatin1("\n");

  QString old;

  // Setup the format combos with
  // 1) Users currently selected/typed in choice
  // 2) The current country's default format
  // 3) The users current global format
  // 4) Some other popular formats for the users current language
  // 5) Remove duplicates

  old = m_comboTimeFmt->currentText();
  formatList.clear();
  formatList.append( old );
  formatList.append( storeToUser( timeMap(), m_locale->timeFormat() ) );
  formatList.append( storeToUser( timeMap(), localeGroup.readEntry( "TimeFormat", QString::fromLatin1( "%H:%M:%S" ) ) ) );
  str = i18nc( "some reasonable time formats for the language",
               "HH:MM:SS\n"
               "pH:MM:SS AMPM");
  formatList.append( str.split( sep ) );
  formatList.removeDuplicates();
  m_comboTimeFmt->clear();
  m_comboTimeFmt->addItems( formatList );
  m_comboTimeFmt->setEditText( old );

  old = m_comboDateFmt->currentText();
  formatList.clear();
  formatList.append( old );
  formatList.append( storeToUser( dateMap(), m_locale->dateFormat() ) );
  formatList.append( storeToUser( dateMap(), localeGroup.readEntry("DateFormat", QString::fromLatin1("%A %d %B %Y") ) ) );
  str = i18nc("some reasonable date formats for the language",
              "WEEKDAY MONTH dD YYYY\n"
              "SHORTWEEKDAY MONTH dD YYYY");
  formatList.append( str.split( sep ) );
  formatList.removeDuplicates();
  m_comboDateFmt->clear();
  m_comboDateFmt->addItems( formatList );
  m_comboDateFmt->setEditText( old );

  old = m_comboDateFmtShort->currentText();
  formatList.clear();
  formatList.append( old );
  formatList.append( storeToUser( dateMap(), m_locale->dateFormatShort() ) );
  formatList.append( storeToUser( dateMap(), localeGroup.readEntry("DateFormatShort", QString::fromLatin1("%Y-%m-%d") ) ) );
  formatList.append( storeToUser( dateMap(), QString::fromLatin1("%Y-%m-%d") ) );
  str = i18nc("some reasonable short date formats for the language",
              "YYYY-MM-DD\n"
              "dD.mM.YYYY\n"
              "DD.MM.YYYY");
  formatList.append( str.split( sep ) );
  formatList.removeDuplicates();
  m_comboDateFmtShort->clear();
  m_comboDateFmtShort->addItems( formatList );
  m_comboDateFmtShort->setEditText( old );

  updateCalendarNames();
  updateWeekDayNames();
  updateDigitSetNames();

  str = ki18n
    ("<p>The text in this textbox will be used to format "
     "time strings. The sequences below will be replaced:</p>"
     "<table>"
     "<tr><td><b>HH</b></td><td>The hour as a decimal number using a 24-hour "
     "clock (00-23).</td></tr>"
     "<tr><td><b>hH</b></td><td>The hour (24-hour clock) as a decimal number "
     "(0-23).</td></tr>"
     "<tr><td><b>PH</b></td><td>The hour as a decimal number using a 12-hour "
     "clock (01-12).</td></tr>"
     "<tr><td><b>pH</b></td><td>The hour (12-hour clock) as a decimal number "
     "(1-12).</td></tr>"
     "<tr><td><b>MM</b></td><td>The minutes as a decimal number (00-59)."
     "</td></tr>"
     "<tr><td><b>SS</b></td><td>The seconds as a decimal number (00-59)."
     "</td></tr>"
     "<tr><td><b>AMPM</b></td><td>Either \"am\" or \"pm\" according to the "
     "given time value. Noon is treated as \"pm\" and midnight as \"am\"."
     "</td></tr>"
     "</table>").toString(m_locale);
  m_labTimeFmt->setWhatsThis( str );
  m_comboTimeFmt->setWhatsThis(  str );

  QString datecodes = ki18n(
    "<table>"
    "<tr><td><b>YYYY</b></td><td>The year with century as a decimal number."
    "</td></tr>"
    "<tr><td><b>YY</b></td><td>The year without century as a decimal number "
    "(00-99).</td></tr>"
    "<tr><td><b>MM</b></td><td>The month as a decimal number (01-12)."
    "</td></tr>"
    "<tr><td><b>mM</b></td><td>The month as a decimal number (1-12).</td></tr>"
    "<tr><td><b>SHORTMONTH</b></td><td>The first three characters of the month name. "
    "</td></tr>"
    "<tr><td><b>MONTH</b></td><td>The full month name.</td></tr>"
    "<tr><td><b>DD</b></td><td>The day of month as a decimal number (01-31)."
    "</td></tr>"
    "<tr><td><b>dD</b></td><td>The day of month as a decimal number (1-31)."
    "</td></tr>"
    "<tr><td><b>SHORTWEEKDAY</b></td><td>The first three characters of the weekday name."
    "</td></tr>"
    "<tr><td><b>WEEKDAY</b></td><td>The full weekday name.</td></tr>"
    "<tr><td><b>ERAYEAR</b></td><td>The Era Year in local format (e.g. 2000 AD).</td></tr>"
    "<tr><td><b>SHORTERANAME</b></td><td>The short Era Name.</td></tr>"
    "<tr><td><b>YEARINERA</b></td><td>The Year in Era as a decimal number.</td></tr>"
    "<tr><td><b>DAYOFYEAR</b></td><td>The Day of Year as a decimal number.</td></tr>"
    "<tr><td><b>ISOWEEK</b></td><td>The ISO Week as a decimal number.</td></tr>"
    "<tr><td><b>DAYOFISOWEEK</b></td><td>The Day of the ISO Week as a decimal number.</td></tr>"
    "</table>").toString(m_locale);

  str = ki18n
    ( "<p>The text in this textbox will be used to format long "
      "dates. The sequences below will be replaced:</p>").toString(m_locale) + datecodes;
  m_labDateFmt->setWhatsThis( str );
  m_comboDateFmt->setWhatsThis(  str );

  str = ki18n
    ( "<p>The text in this textbox will be used to format short "
      "dates. For instance, this is used when listing files. "
      "The sequences below will be replaced:</p>").toString(m_locale) + datecodes;
  m_labDateFmtShort->setWhatsThis( str );
  m_comboDateFmtShort->setWhatsThis(  str );

  str = ki18n
    ( "<p>Here you can define the set of digits "
      "used to display time and dates. "
      "If digits other than Arabic are selected, "
      "they will appear only if used in the language "
      "of the application or the piece of text "
      "where the number is shown.</p>" ).toString( m_locale );
  m_labDateTimeDigSet->setWhatsThis( str );
  m_comboDateTimeDigSet->setWhatsThis( str );

  str = ki18n
    ("<p>This option determines which day will be considered as "
     "the first one of the week.</p>").toString(m_locale);
  m_comboWeekStartDay->setWhatsThis(  str );

  str = ki18n
    ("<p>This option determines which day will be considered as "
     "the first working day of the week.</p>").toString(m_locale);
  m_comboWorkingWeekStartDay->setWhatsThis(  str );

  str = ki18n
    ("<p>This option determines which day will be considered as "
     "the last working day of the week.</p>").toString(m_locale);
  m_comboWorkingWeekEndDay->setWhatsThis(  str );

  str = ki18n
    ("<p>This option determines which day will be considered as "
     "the day of the week for religious observance.</p>").toString(m_locale);
  m_comboWeekDayOfPray->setWhatsThis(  str );

  if ( m_locale->nounDeclension() )
  {
    str = ki18n
      ("<p>This option determines whether possessive form of month "
       "names should be used in dates.</p>").toString(m_locale);
    m_chDateMonthNamePossessive->setWhatsThis(  str );
  }

  str = ki18n
    ("<p>This option determines if the Common Era (CE/BCE) should "
     "be used instead of the Christian Era (AD/BC).</p>").toString(m_locale);
  m_checkUseCommonEra->setWhatsThis( str );
}

void KLocaleConfigTime::updateWeekDayNames()
{
  const KCalendarSystem * calendar = m_locale->calendar();
  int daysInWeek = calendar->daysInWeek(QDate::currentDate());
  QString weekDayName = i18nc("Day name list, option for no day of religious observance", "None");

  m_comboWeekStartDay->clear();
  m_comboWorkingWeekStartDay->clear();
  m_comboWorkingWeekEndDay->clear();
  m_comboWeekDayOfPray->clear();

  m_comboWeekDayOfPray->insertItem(0, weekDayName);

  for ( int i = 1; i <= daysInWeek; ++i )
  {
    weekDayName = calendar->weekDayName(i);
    m_comboWeekStartDay->insertItem(i - 1, weekDayName);
    m_comboWorkingWeekStartDay->insertItem(i - 1, weekDayName);
    m_comboWorkingWeekEndDay->insertItem(i - 1, weekDayName);
    m_comboWeekDayOfPray->insertItem(i, weekDayName);
  }

  m_comboWeekStartDay->setCurrentIndex( m_locale->weekStartDay() - 1 );
  m_comboWorkingWeekStartDay->setCurrentIndex( m_locale->workingWeekStartDay() - 1 );
  m_comboWorkingWeekEndDay->setCurrentIndex( m_locale->workingWeekEndDay() - 1 );
  m_comboWeekDayOfPray->setCurrentIndex( m_locale->weekDayOfPray() );  // First option is None=0

}

void KLocaleConfigTime::updateDigitSetNames()
{
  QList<KLocale::DigitSet> digitSets = m_locale->allDigitSetsList();
  qSort(digitSets);
  m_comboDateTimeDigSet->clear();
  foreach ( const KLocale::DigitSet &ds, digitSets )
  {
     m_comboDateTimeDigSet->addItem(m_locale->digitSetToName(ds, true));
  }
  m_comboDateTimeDigSet->setCurrentIndex(m_locale->dateTimeDigitSet());
}

void KLocaleConfigTime::updateCalendarNames()
{
    QStringList calendars = KCalendarSystem::calendarSystems();

    m_comboCalendarSystem->clear();

    foreach ( const QString &cal, calendars )
    {
        m_comboCalendarSystem->addItem( KCalendarSystem::calendarLabel( cal ), QVariant( cal ) );
    }

    m_comboCalendarSystem->setCurrentIndex( m_comboCalendarSystem->findData( QVariant( m_locale->calendarType() ) ) );
}
