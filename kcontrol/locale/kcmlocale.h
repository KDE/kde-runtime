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
class QCheckBox;
class KPushButton;
class KComboBox;
class KIntNumInput;

namespace Ui {
  class KCMLocaleWidget;
}

/**
 * @short A KCM to configure locale settings
 *
 * This module is for changing the User's Locale settings, which may override their Group and
 * Country defaults.
 *
 * The settings hierarchy is as follows:
 * - User settings from kdeglobals
 * - Group settings from $KDEDIRS
 * - Country settings from l10n
 * - C default settings from l10n
 *
 * The settings that apply to the User are a merger of all these.
 *
 * This may be restricted by Kiosk Group settings locking the user from updating some/all settings.
 *
 * The KCM starts by loading the fully merged settings including the User settings
 * In KCM terms, to Reload is to load the fully merged settings including the User settings
 * In KCM terms, to reset to Defaults is to remove the User settings only.
 * The user can also reset to Default each individual setting.
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

    void copySettings( KConfigGroup *fromGroup, KConfigGroup *toGroup );
    void copySetting( KConfigGroup *fromGroup, KConfigGroup *toGroup, const QString &key, const QString &type );
    void copySetting( KConfigGroup *fromGroup, KConfigGroup *toGroup, const QString &key, const QStringList &type );
    void copySetting( KConfigGroup *fromGroup, KConfigGroup *toGroup, const QString &key, int type );
    void copySetting( KConfigGroup *fromGroup, KConfigGroup *toGroup, const QString &key, bool type );
    void mergeSettings( const QString &countryCode );

    void setItem( const QString itemKey, const QString &itemValue,
                  QWidget *itemWidget, KPushButton *itemReset );
    void setItem( const QString itemKey, int itemValue,
                  QWidget *itemWidget, KPushButton *itemReset );
    void setItem( const QString itemKey, bool itemValue,
                  QWidget *itemWidget, KPushButton *itemReset );
    void setComboItem( const QString itemKey, const QString &itemValue,
                       KComboBox *itemCombo, KPushButton *itemDefaultButton );
    void setComboItem( const QString itemKey, int itemValue,
                       KComboBox *itemCombo, KPushButton *itemDefaultButton );
    void setEditComboItem( const QString itemKey, const QString &itemValue,
                           KComboBox *itemCombo, KPushButton *itemDefaultButton );
    void setEditComboItem( const QString itemKey, int itemValue,
                           KComboBox *itemCombo, KPushButton *itemDefaultButton );
    void setIntItem( const QString itemKey, int itemValue,
                     KIntNumInput *itemInput, KPushButton *itemDefaultButton );
    void setCheckItem( const QString itemKey, bool itemValue,
                       QCheckBox *itemCheck, KPushButton *itemDefaultButton );
    void setMonetaryFormat( const QString prefixCurrencySymbolKey, bool prefixCurrencySymbol,
                            const QString signPositionKey, KLocale::SignPosition signPosition,
                            QWidget *formatWidget, KPushButton *formatDefaultButton );
    void checkIfChanged();

    //Common init utilities
    void initSeparatorCombo( KComboBox *separatorCombo );
    void initWeekDayCombo( KComboBox *dayCombo );
    void initDigitSetCombo( KComboBox *digitSetCombo );
    void insertMonetaryPositiveFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition );
    void insertMonetaryNegativeFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition );

    void initAllWidgets();
    void initSettingsWidgets();
    void initResetButtons();
    void initTabs();
    void initSample();

    //Country tab

    void initCountry();
    void initCountryDivision();

    //Translations/Languages tab

    void initTranslations();
    void initTranslationsInstall();

    //Numeric tab

    void initNumericThousandsSeparator();
    void initNumericDecimalSymbol();
    void initNumericDecimalPlaces();
    void initNumericPositiveSign();
    void initNumericNegativeSign();
    void initNumericDigitSet();

    //Monetary tab
    void initCurrencyCode();
    void initCurrencySymbol();
    void initMonetaryThousandsSeparator();
    void initMonetaryDecimalSymbol();
    void initMonetaryDecimalPlaces();
    void initMonetaryPositiveFormat();
    void initMonetaryNegativeFormat();
    void initMonetaryDigitSet();

    //Calendar Tab

    void initCalendarSystem();
    void initUseCommonEra();
    void initShortYearWindow();
    void initWeekStartDay();
    void initWorkingWeekStartDay();
    void initWorkingWeekEndDay();
    void initWeekDayOfPray();

    //Date/Time tab

    void initTimeFormat();
    void initAmSymbol();
    void initPmSymbol();
    void initDateFormat();
    void initShortDateFormat();
    void initMonthNamePossessive();
    void initDateTimeDigitSet();

    //Other Tab

    void initPageSize();
    void initMeasureSystem();
    void initBinaryUnitDialect();

private Q_SLOTS:

    void updateSample();

    //Country tab

    void defaultCountry();
    void changedCountryIndex( int index );
    void changeCountry( const QString &newValue );

    void defaultCountryDivision();
    void changedCountryDivisionIndex( int index );
    void changeCountryDivision( const QString &newValue );

    //Translations/Languages tab

    void defaultTranslations();
    void changeTranslations();
    void changeTranslations( const QString &newValue );

    void translationsAvailableSelectionChanged();
    void translationsSelectedSelectionChanged();
    void translationsAdd();
    void translationsRemove();
    void translationsUp();
    void translationsDown();

    void translationsInstall();

    //Numeric tab

    void defaultNumericThousandsSeparator();
    void changeNumericThousandsSeparator( const QString &newValue );

    void defaultNumericDecimalSymbol();
    void changeNumericDecimalSymbol( const QString &newValue );

    void defaultNumericDecimalPlaces();
    void changeNumericDecimalPlaces( int newValue );

    void defaultNumericPositiveSign();
    void changeNumericPositiveSign( const QString &newValue );

    void defaultNumericNegativeSign();
    void changeNumericNegativeSign( const QString &newValue );

    void defaultNumericDigitSet();
    void changedNumericDigitSetIndex( int index );
    void changeNumericDigitSet( int newValue );

    //Monetary tab

    void defaultCurrencyCode();
    void changedCurrencyCodeIndex( int index );
    void changeCurrencyCode( const QString &newValue );

    void defaultCurrencySymbol();
    void changedCurrencySymbolIndex( int index );
    void changeCurrencySymbol( const QString &newValue );

    void defaultMonetaryThousandsSeparator();
    void changeMonetaryThousandsSeparator( const QString &newValue );

    void defaultMonetaryDecimalSymbol();
    void changeMonetaryDecimalSymbol( const QString &newValue );

    void defaultMonetaryDecimalPlaces();
    void changeMonetaryDecimalPlaces( int newValue );

    void defaultMonetaryPositiveFormat();
    void changedMonetaryPositiveFormatIndex( int index );
    void changeMonetaryPositiveFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition );

    void defaultMonetaryNegativeFormat();
    void changedMonetaryNegativeFormatIndex( int index );
    void changeMonetaryNegativeFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition );

    void defaultMonetaryDigitSet();
    void changedMonetaryDigitSetIndex( int index );
    void changeMonetaryDigitSet( int newValue );

    //Calendar Tab

    void defaultCalendarSystem();
    void changedCalendarSystemIndex( int index );
    void changeCalendarSystem( const QString &newValue );

    //void defaultUseCommonEra();
    //void changeUseCommonEra();

    //void defaultShortYearWindow();
    //void changeShortYearWindow( int newStartYear );

    void defaultWeekStartDay();
    void changedWeekStartDayIndex( int index );
    void changeWeekStartDay( int newValue );

    void defaultWorkingWeekStartDay();
    void changedWorkingWeekStartDayIndex( int index );
    void changeWorkingWeekStartDay( int newValue );

    void defaultWorkingWeekEndDay();
    void changedWorkingWeekEndDayIndex( int index );
    void changeWorkingWeekEndDay( int newValue );

    void defaultWeekDayOfPray();
    void changedWeekDayOfPrayIndex( int index );
    void changeWeekDayOfPray( int newValue );

    //Date/Time tab

    void defaultTimeFormat();
    void changeTimeFormat( const QString &newValue );

    //void defaultAmSymbol();
    //void changeAmSymbol( const QString &newValue );

    //void defaultPmSymbol();
    //void changePmSymbol( const QString &newValue );

    void defaultDateFormat();
    void changeDateFormat( const QString &newValue );

    void defaultShortDateFormat();
    void changeShortDateFormat( const QString &newValue );

    void defaultMonthNamePossessive();
    void changeMonthNamePossessive( bool newValue );

    void defaultDateTimeDigitSet();
    void changedDateTimeDigitSetIndex( int index );
    void changeDateTimeDigitSet( int newValue );

    //Other Tab

    void defaultPageSize();
    void changedPageSizeIndex( int index );
    void changePageSize( int newValue );

    void defaultMeasureSystem();
    void changedMeasureSystemIndex( int index );
    void changeMeasureSystem( int newValue );

    void defaultBinaryUnitDialect();
    void changedBinaryUnitDialectIndex( int index );
    void changeBinaryUnitDialect( int newValue );

private:
    // Calendar Type/System Conversion Utilities
    KLocale::CalendarSystem calendarTypeToCalendarSystem(const QString &calendarType) const;
    QString calendarSystemToCalendarType(KLocale::CalendarSystem calendarSystem) const;

    // Date/Time format map utilities
    QString posixToUserDate( const QString &posixFormat ) const;
    QString posixToUserTime( const QString &posixFormat ) const;
    QString posixToUser( const QString &posixFormat, const QMap<QString, QString> &map ) const;
    QString userToPosixDate( const QString &userFormat ) const;
    QString userToPosixTime( const QString &userFormat ) const;
    QString userToPosix( const QString &userFormat, const QMap<QString, QString> &map ) const;

    // The current User settings from .kde/share/config/kdeglobals
    // This gets updated with the users changes in the kcm and saved when requested
    KSharedConfigPtr m_userConfig;
    KConfigGroup m_userSettings;
    // The kcm config/settings, a merger of C, Country, Group and User settings
    // This is used to build the displayed settings and to initialise the sample locale, but never saved
    KSharedConfigPtr m_kcmConfig;
    KConfigGroup m_kcmSettings;
    // The currently saved user config/settings
    // This is used to check if anything has changed, but never saved
    // Wouldn't be needed if KConfig/KConfigGroup had a simple isDirty() call
    KSharedConfigPtr m_currentConfig;
    KConfigGroup m_currentSettings;

    // The KCM Default settings, a merger of C, Country, and Group, i.e. excluding User
    KSharedConfigPtr m_defaultConfig;
    KConfigGroup m_defaultSettings;
    // The current Group settings, i.e. does NOT include the User or Country settings
    KSharedConfigPtr m_groupConfig;
    KConfigGroup m_groupSettings;
    // The Country Locale config from l10n/<country>/entry.desktop
    KSharedConfigPtr m_countryConfig;
    KConfigGroup m_countrySettings;
    // The default C Locale config/settings from l10n/C/entry.desktop
    KSharedConfigPtr m_cConfig;
    KConfigGroup m_cSettings;

    QMap<QString, QString> m_dateFormatMap;
    QMap<QString, QString> m_timeFormatMap;

    // The system country, not the KDE one
    QString m_systemCountry;

    // NOTE: we need to mantain our own language list instead of using KLocale's
    // because KLocale does not add a language if there is no translation
    // for the current application so it would not be possible to set
    // a language which has no systemsettings/kcontrol module translation
    // The list of translations used in the kcm, is users list plus us_EN
    QStringList m_kcmTranslations;
    // The currently saved list of user translations, used to check if value changed
    QString m_currentTranslations;
    // The currently installed translations, used to check if users translations are valid
    QStringList m_installedTranslations;

    // The locale we use when displaying the sample output, not used for anything else
    KLocale *m_kcmLocale;

    Ui::KCMLocaleWidget *m_ui;
};

#endif //KCMLOCALE_H