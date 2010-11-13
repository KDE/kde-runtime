/*  This file is part of the KDE libraries
 *  Copyright 2010 John Layt <john@layt.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KCMLOCALE_H
#define KCMLOCALE_H

#include <QMap>

#include <KCModule>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocale>

class QListWidgetItem;
class KPushButton;
class KComboBox;

namespace Ui {
  class KCMLocaleWidget;
}

/**
 * @short A KCM to configure locale settings
 *
 * This module is for changing the Users locale settings, which may override their Group and
 * Country defaults.
 *
 * The settings hierarchy is as follows:
 * - User settings from kdeglobals
 * - Group settings from $KDE_DIRS
 * - Country settings from l10n
 * - Default C settings from l10n
 *
 * This may be restricted by Kiosk Group settings locking the user from updating some/all settings.
 */

class KCMLocale : public KCModule
{
    Q_OBJECT

public:
    KCMLocale(QWidget *parent, const QVariantList &);
    virtual ~KCMLocale();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

private:

    //Common load/save utilities

    void loadCombo( const QString &key, const QString &loadValue, KComboBox *combo, KPushButton *resetButton );
    void loadCombo( const QString &key, int loadValue, KComboBox *combo, KPushButton *resetButton );
    void loadEditCombo( const QString &key, const QString &loadValue, KComboBox *combo, KPushButton *resetButton );

    void saveValue( const QString &key, const QString &saveValue );
    void saveValue( const QString &key, int saveValue );

    void defaultCombo( const QString &key, const QString &type, KComboBox *combo );
    void defaultCombo( const QString &key, int type, KComboBox *combo );
    void defaultEditCombo( const QString &key, const QString &type, KComboBox *combo );

    void setItemEnabled( const QString itemKey, QWidget *itemWidget, KPushButton *itemReset );

    //Common init utilities
    void initSeparatorCombo( KComboBox *separatorCombo );
    void initPositiveCombo( KComboBox *positiveCombo );
    void initNegativeCombo( KComboBox *negativeCombo );
    void initWeekDayCombo( KComboBox *dayCombo );
    void initDigitSetCombo( KComboBox *digitSetCombo );

    void initAllWidgets();
    void initResetButtons();
    void initTabs();
    void initSample();

    //Country tab

    void initCountry();
    void loadCountry();
    void saveCountry();

    void initCountryDivision();
    void loadCountryDivision();
    void saveCountryDivision();

    //Translations/Languages tab

    void initTranslations();
    void loadTranslations();
    void saveTranslations();

    void initTranslationsInstall();
    void loadTranslationsInstall();
    void saveTranslationsInstall();

    //Numeric tab

    void initNumericThousandsSeparator();
    void loadNumericThousandsSeparator();
    void saveNumericThousandsSeparator();

    void initNumericDecimalSymbol();
    void loadNumericDecimalSymbol();
    void saveNumericDecimalSymbol();

    void initNumericDecimalPlaces();
    void loadNumericDecimalPlaces();
    void saveNumericDecimalPlaces();

    void initNumericPositiveSign();
    void loadNumericPositiveSign();
    void saveNumericPositiveSign();

    void initNumericNegativeSign();
    void loadNumericNegativeSign();
    void saveNumericNegativeSign();

    void initNumericDigitSet();
    void loadNumericDigitSet();
    void saveNumericDigitSet();

    //Monetary tab
    void initCurrencyCode();
    void loadCurrencyCode();
    void saveCurrencyCode();

    void initCurrencySymbol();
    void loadCurrencySymbol();
    void saveCurrencySymbol();

    void initMonetaryThousandsSeparator();
    void loadMonetaryThousandsSeparator();
    void saveMonetaryThousandsSeparator();

    void initMonetaryDecimalSymbol();
    void loadMonetaryDecimalSymbol();
    void saveMonetaryDecimalSymbol();

    void initMonetaryDecimalPlaces();
    void loadMonetaryDecimalPlaces();
    void saveMonetaryDecimalPlaces();

    void initMonetaryPositiveFormat();
    void loadMonetaryPositiveFormat();
    void saveMonetaryPositiveFormat();

    void initMonetaryNegativeFormat();
    void loadMonetaryNegativeFormat();
    void saveMonetaryNegativeFormat();

    void initMonetaryDigitSet();
    void loadMonetaryDigitSet();
    void saveMonetaryDigitSet();

    //Calendar Tab

    void initCalendarSystem();
    void loadCalendarSystem();
    void saveCalendarSystem();

    void initUseCommonEra();
    void loadUseCommonEra();
    void saveUseCommonEra();

    void initShortYearWindow();
    void loadShortYearWindow();
    void saveShortYearWindow();

    void initWeekStartDay();
    void loadWeekStartDay();
    void saveWeekStartDay();

    void initWorkingWeekStartDay();
    void loadWorkingWeekStartDay();
    void saveWorkingWeekStartDay();

    void initWorkingWeekEndDay();
    void loadWorkingWeekEndDay();
    void saveWorkingWeekEndDay();

    void initWeekDayOfPray();
    void loadWeekDayOfPray();
    void saveWeekDayOfPray();

    //Date/Time tab

    void initTimeFormat();
    void loadTimeFormat();
    void saveTimeFormat();

    void initAmSymbol();
    void loadAmSymbol();
    void saveAmSymbol();

    void initPmSymbol();
    void loadPmSymbol();
    void savePmSymbol();

    void initDateFormat();
    void loadDateFormat();
    void saveDateFormat();

    void initShortDateFormat();
    void loadShortDateFormat();
    void saveShortDateFormat();

    void initMonthNamePossessive();
    void loadMonthNamePossessive();
    void saveMonthNamePossessive();

    void initDateTimeDigitSet();
    void loadDateTimeDigitSet();
    void saveDateTimeDigitSet();

    //Other Tab

    void initPageSize();
    void loadPageSize();
    void savePageSize();

    void initMeasureSystem();
    void loadMeasureSystem();
    void saveMeasureSystem();

    void initBinaryUnitDialect();
    void loadBinaryUnitDialect();
    void saveBinaryUnitDialect();

private Q_SLOTS:

    void updateSample();

    //Country tab

    void defaultCountry();
    void changeCountry( int activated );

    void defaultCountryDivision();
    void changeCountryDivision( int activated );

    //Translations/Languages tab

    void changeTranslations( QListWidgetItem *item );

    //void callTranslationsInstall();

    //Numeric tab

    void defaultNumericThousandsSeparator();
    void changeNumericThousandsSeparator( const QString &newValue );

    void defaultNumericDecimalSymbol();
    void changeNumericDecimalSymbol( const QString &newValue );

    //void defaultNumericDecimalPlaces();
    //void changeNumericDecimalPlaces( int newValue );

    void defaultNumericPositiveSign();
    void changeNumericPositiveSign( const QString &newValue );

    void defaultNumericNegativeSign();
    void changeNumericNegativeSign( const QString &newValue );

    void defaultNumericDigitSet();
    void changeNumericDigitSet( int activated );

    //Monetary tab

    void defaultCurrencyCode();
    void changeCurrencyCode( int activated );

    void defaultCurrencySymbol();
    void changeCurrencySymbol( int activated );

    void defaultMonetaryThousandsSeparator();
    void changeMonetaryThousandsSeparator( const QString &newValue );

    void defaultMonetaryDecimalSymbol();
    void changeMonetaryDecimalSymbol( const QString &newValue );

    void defaultMonetaryDecimalPlaces();
    void changeMonetaryDecimalPlaces( int newValue );

    void defaultMonetaryPositiveFormat();
    void changeMonetaryPositiveFormat( int activated );

    void defaultMonetaryNegativeFormat();
    void changeMonetaryNegativeFormat( int activated );

    void defaultMonetaryDigitSet();
    void changeMonetaryDigitSet( int activated );

    //Calendar Tab

    void defaultCalendarSystem();
    void changeCalendarSystem( int activated );

    //void defaultUseCommonEra();
    //void changeUseCommonEra();

    //void defaultShortYearWindow();
    //void changeShortYearWindow( int newStartYear );

    void defaultWeekStartDay();
    void changeWeekStartDay( int activated );

    void defaultWorkingWeekStartDay();
    void changeWorkingWeekStartDay( int activated );

    void defaultWorkingWeekEndDay();
    void changeWorkingWeekEndDay( int activated );

    void defaultWeekDayOfPray();
    void changeWeekDayOfPray( int activated );

    //Date/Time tab

    //void defaultTimeFormat();
    //void changeTimeFormat( const QString &newValue );

    //void defaultAmSymbol();
    //void changeAmSymbol( const QString &newValue );

    //void defaultPmSymbol();
    //void changePmSymbol( const QString &newValue );

    //void defaultDateFormat();
    //void changeDateFormat( const QString &newValue );

    //void defaultShortDateFormat();
    //void changeShortDateFormat( const QString &newValue );

    //void defaultMonthNamePossessive();
    //void changeMonthNamePossessive();

    void defaultDateTimeDigitSet();
    void changeDateTimeDigitSet( int activated );

    //Other Tab

    void defaultPageSize();
    void changePageSize( int activated );

    void defaultMeasureSystem();
    void changeMeasureSystem( int activated );

    void defaultBinaryUnitDialect();
    void changeBinaryUnitDialect( int activated );

private:
    //Utilities
    KLocale::CalendarSystem calendarTypeToCalendarSystem(const QString &calendarType) const;
    QString calendarSystemToCalendarType(KLocale::CalendarSystem calendarSystem) const;

    // Date/Time format map utilities
    QString posixToUserDate( const QString &posixFormat ) const;
    QString posixToUserTime( const QString &posixFormat ) const;
    QString posixToUser( const QString &posixFormat, const QMap<QString, QString> &map ) const;
    QString userToPosixDate( const QString &userFormat ) const;
    QString userToPosixTime( const QString &userFormat ) const;
    QString userToPosix( const QString &userFormat, const QMap<QString, QString> &map ) const;

    // The current merged system and user global config, i.e. KGlobal::config()
    // Note does NOT include the country default values
    KSharedConfigPtr m_globalConfig;
    KConfigGroup m_globalSettings;
    // The country Locale config, i.e. from l10n/<county>/entry.desktop
    KSharedConfigPtr m_countryConfig;
    KConfigGroup m_countrySettings;
    // The default C Locale config, i.e. from l10n/C/entry.desktop
    KSharedConfigPtr m_defaultConfig;
    KConfigGroup m_defaultSettings;
    // The kcm config, i.e. originally from global, but then modified in the kcm and not yet saved
    KConfig *m_kcmConfig;
    KConfigGroup m_kcmSettings;

    QMap<QString, QString> m_dateFormatMap;
    QMap<QString, QString> m_timeFormatMap;

    // NOTE: we need to mantain our own language list instead of using KLocale's
    // because KLocale does not add a language if there is no translation
    // for the current application so it would not be possible to set
    // a language which has no systemsettings/kcontrol module translation
    QStringList m_languages;
    QStringList m_installedLanguages;

    // The locale
    KLocale *m_kcmLocale;

    Ui::KCMLocaleWidget *m_ui;
};

#endif //KCMLOCALE_H