/*
 *  Copyright (c) 1998 Matthias Hoelzer (hoelzer@physik.uni-wuerzburg.de)
 *  Copyright (c) 1999 Preston Brown <pbrown@kde.org>
 *  Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>
 *  Copyright 2010 John Layt <john@layt.net>
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
 *
 */

#include "kcmlocale.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QPrinter>
#include <QTimer>

#include <KAboutData>
#include <KActionSelector>
#include <KBuildSycocaProgressDialog>
#include <KCalendarSystem>
#include <KDateTime>
#include <KDebug>
#include <KLocale>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KStandardDirs>
#include <KCurrencyCode>

#include "ui_kcmlocalewidget.h"

K_PLUGIN_FACTORY( KCMLocaleFactory, registerPlugin<KCMLocale>(); )
K_EXPORT_PLUGIN( KCMLocaleFactory( "kcmlocale" ) )

KCMLocale::KCMLocale( QWidget *parent, const QVariantList &args )
         : KCModule( KCMLocaleFactory::componentData(), parent, args ),
           m_userConfig( 0 ),
           m_kcmConfig( 0 ),
           m_currentConfig( 0 ),
           m_defaultConfig( 0 ),
           m_groupConfig( 0 ),
           m_countryConfig( 0 ),
           m_cConfig( 0 ),
           m_kcmLocale( 0 ),
           m_ui( new Ui::KCMLocaleWidget )
{
    KAboutData *about = new KAboutData( "kcmlocale", 0, ki18n( "Localization options for KDE applications" ),
                                        0, KLocalizedString(), KAboutData::License_GPL,
                                        ki18n( "Copyright 2010 John Layt" ) );

    about->addAuthor( ki18n( "John Layt" ), ki18n( "Maintainer" ), "john@layt.net" );

    setAboutData( about );

    m_ui->setupUi( this );

    initSettings();

    // Country tab
    connect( m_ui->m_comboCountry,       SIGNAL( activated( int ) ),
             this,                       SLOT( changedCountryIndex( int ) ) );
    connect( m_ui->m_buttonResetCountry, SIGNAL( clicked() ),
             this,                       SLOT( defaultCountry() ) );

    connect( m_ui->m_comboCountryDivision,       SIGNAL( activated( int ) ),
             this,                               SLOT( changedCountryDivisionIndex( int ) ) );
    connect( m_ui->m_buttonResetCountryDivision, SIGNAL( clicked() ),
             this,                               SLOT( defaultCountryDivision() ) );

    // Translations tab

    // User has changed the translations selection in some way
    connect( m_ui->m_selectTranslations,         SIGNAL( added( QListWidgetItem* ) ),
             this,                               SLOT( changeTranslations() ) );
    connect( m_ui->m_selectTranslations,         SIGNAL( removed( QListWidgetItem* ) ),
             this,                               SLOT( changeTranslations() ) );
    connect( m_ui->m_selectTranslations,         SIGNAL( movedUp( QListWidgetItem* ) ),
             this,                               SLOT( changeTranslations() ) );
    connect( m_ui->m_selectTranslations,         SIGNAL( movedDown( QListWidgetItem* ) ),
             this,                               SLOT( changeTranslations() ) );
    // User has clicked Install button, trigger distro specific install routine
    connect( m_ui->m_buttonTranslationsInstall,  SIGNAL( clicked() ),
             this,                               SLOT( installTranslations() ) );
    // User has clicked Default button, resest lists to Defaults
    connect( m_ui->m_buttonResetTranslations,    SIGNAL( clicked() ),
             this,                               SLOT( defaultTranslations() ) );

    // Numbers tab

    connect( m_ui->m_comboThousandsSeparator,       SIGNAL( editTextChanged( const QString & ) ),
             this,                                  SLOT( changeNumericThousandsSeparator( const QString & ) ) );
    connect( m_ui->m_buttonResetThousandsSeparator, SIGNAL( clicked() ),
             this,                                  SLOT( defaultNumericThousandsSeparator() ) );

    connect( m_ui->m_comboDecimalSymbol,       SIGNAL( editTextChanged( const QString & ) ),
             this,                             SLOT( changeNumericDecimalSymbol( const QString & ) ) );
    connect( m_ui->m_buttonResetDecimalSymbol, SIGNAL( clicked() ),
             this,                             SLOT( defaultNumericDecimalSymbol() ) );

    connect( m_ui->m_intDecimalPlaces,         SIGNAL( valueChanged( int ) ),
             this,                             SLOT( changeNumericDecimalPlaces( int ) ) );
    connect( m_ui->m_buttonResetDecimalPlaces, SIGNAL( clicked() ),
             this,                             SLOT( defaultNumericDecimalPlaces() ) );

    connect( m_ui->m_comboPositiveSign,       SIGNAL( editTextChanged( const QString & ) ),
             this,                            SLOT( changeNumericPositiveSign( const QString & ) ) );
    connect( m_ui->m_buttonResetPositiveSign, SIGNAL( clicked() ),
             this,                            SLOT( defaultNumericPositiveSign() ) );

    connect( m_ui->m_comboNegativeSign,       SIGNAL( editTextChanged( const QString & ) ),
             this,                            SLOT( changeNumericNegativeSign( const QString & ) ) );
    connect( m_ui->m_buttonResetNegativeSign, SIGNAL( clicked() ),
             this,                            SLOT( defaultNumericNegativeSign() ) );

    connect( m_ui->m_comboDigitSet,       SIGNAL( currentIndexChanged( int ) ),
             this,                        SLOT( changedNumericDigitSetIndex( int ) ) );
    connect( m_ui->m_buttonResetDigitSet, SIGNAL( clicked() ),
             this,                        SLOT( defaultNumericDigitSet() ) );

    // Money tab

    connect( m_ui->m_comboCurrencyCode,       SIGNAL( currentIndexChanged( int ) ),
             this,                              SLOT( changedCurrencyCodeIndex( int ) ) );
    connect( m_ui->m_buttonResetCurrencyCode, SIGNAL( clicked() ),
             this,                              SLOT( defaultCurrencyCode() ) );

    connect( m_ui->m_comboCurrencySymbol,       SIGNAL( currentIndexChanged( int ) ),
             this,                              SLOT( changedCurrencySymbolIndex( int ) ) );
    connect( m_ui->m_buttonResetCurrencySymbol, SIGNAL( clicked() ),
             this,                              SLOT( defaultCurrencySymbol() ) );

    connect( m_ui->m_comboMonetaryThousandsSeparator,       SIGNAL( editTextChanged( const QString & ) ),
             this,                                          SLOT( changeMonetaryThousandsSeparator( const QString & ) ) );
    connect( m_ui->m_buttonResetMonetaryThousandsSeparator, SIGNAL( clicked() ),
             this,                                          SLOT( defaultMonetaryThousandsSeparator() ) );

    connect( m_ui->m_comboMonetaryDecimalSymbol,       SIGNAL( editTextChanged( const QString & ) ),
             this,                                     SLOT( changeMonetaryDecimalSymbol( const QString & ) ) );
    connect( m_ui->m_buttonResetMonetaryDecimalSymbol, SIGNAL( clicked() ),
             this,                                     SLOT( defaultMonetaryDecimalSymbol() ) );

    connect( m_ui->m_intMonetaryDecimalPlaces,         SIGNAL( valueChanged( int ) ),
             this,                                     SLOT( changeMonetaryDecimalPlaces( int ) ) );
    connect( m_ui->m_buttonResetMonetaryDecimalPlaces, SIGNAL( clicked() ),
             this,                                     SLOT( defaultMonetaryDecimalPlaces() ) );

    connect( m_ui->m_comboMonetaryPositiveFormat,       SIGNAL( currentIndexChanged( int ) ),
             this,                                      SLOT( changedMonetaryPositiveFormatIndex( int ) ) );
    connect( m_ui->m_buttonResetMonetaryPositiveFormat, SIGNAL( clicked() ),
             this,                                      SLOT( defaultMonetaryPositiveFormat() ) );

    connect( m_ui->m_comboMonetaryNegativeFormat,       SIGNAL( currentIndexChanged( int ) ),
             this,                                      SLOT( changedMonetaryNegativeFormatIndex( int ) ) );
    connect( m_ui->m_buttonResetMonetaryNegativeFormat, SIGNAL( clicked() ),
             this,                                      SLOT( defaultMonetaryNegativeFormat() ) );

    connect( m_ui->m_comboMonetaryDigitSet,       SIGNAL( currentIndexChanged( int ) ),
             this,                                SLOT( changedMonetaryDigitSetIndex( int ) ) );
    connect( m_ui->m_buttonResetMonetaryDigitSet, SIGNAL( clicked() ),
             this,                                SLOT( defaultMonetaryDigitSet() ) );

    // Calendar tab

    connect( m_ui->m_comboCalendarSystem,       SIGNAL( currentIndexChanged( int ) ),
             this,                              SLOT( changedCalendarSystemIndex( int ) ) );
    connect( m_ui->m_buttonResetCalendarSystem, SIGNAL( clicked() ),
             this,                              SLOT( defaultCalendarSystem() ) );

    connect( m_ui->m_checkCalendarGregorianUseCommonEra,       SIGNAL( clicked( bool ) ),
             this,                                             SLOT( changeUseCommonEra( bool ) ) );
    connect( m_ui->m_buttonResetCalendarGregorianUseCommonEra, SIGNAL( clicked() ),
             this,                                             SLOT( defaultUseCommonEra() ) );

    connect( m_ui->m_intShortYearWindowStartYear, SIGNAL( valueChanged( int ) ),
             this,                                SLOT( changeShortYearWindow( int ) ) );
    connect( m_ui->m_buttonResetShortYearWindow,  SIGNAL( clicked() ),
             this,                                SLOT( defaultShortYearWindow() ) );

    connect( m_ui->m_comboWeekStartDay,       SIGNAL( currentIndexChanged( int ) ),
             this,                            SLOT( changedWeekStartDayIndex( int ) ) );
    connect( m_ui->m_buttonResetWeekStartDay, SIGNAL( clicked() ),
             this,                            SLOT( defaultWeekStartDay() ) );

    connect( m_ui->m_comboWorkingWeekStartDay,       SIGNAL( currentIndexChanged( int ) ),
             this,                                   SLOT( changedWorkingWeekStartDayIndex( int ) ) );
    connect( m_ui->m_buttonResetWorkingWeekStartDay, SIGNAL( clicked() ),
             this,                                   SLOT( defaultWorkingWeekStartDay() ) );

    connect( m_ui->m_comboWorkingWeekEndDay,       SIGNAL( currentIndexChanged( int ) ),
             this,                                 SLOT( changedWorkingWeekEndDayIndex( int ) ) );
    connect( m_ui->m_buttonResetWorkingWeekEndDay, SIGNAL( clicked() ),
             this,                                 SLOT( defaultWorkingWeekEndDay() ) );

    connect( m_ui->m_comboWeekDayOfPray,       SIGNAL( currentIndexChanged( int ) ),
             this,                             SLOT( changedWeekDayOfPrayIndex( int ) ) );
    connect( m_ui->m_buttonResetWeekDayOfPray, SIGNAL( clicked() ),
             this,                             SLOT( defaultWeekDayOfPray() ) );

    // Date / Time tab

    connect( m_ui->m_comboTimeFormat,       SIGNAL( editTextChanged( const QString & ) ),
             this,                          SLOT( changeTimeFormat( const QString & ) ) );
    connect( m_ui->m_buttonResetTimeFormat, SIGNAL( clicked() ),
             this,                          SLOT( defaultTimeFormat() ) );

    connect( m_ui->m_comboAmSymbol,       SIGNAL( editTextChanged( const QString & ) ),
             this,                        SLOT( changeAmSymbol( const QString & ) ) );
    connect( m_ui->m_buttonResetAmSymbol, SIGNAL( clicked() ),
             this,                        SLOT( defaultAmSymbol() ) );

    connect( m_ui->m_comboPmSymbol,       SIGNAL( editTextChanged( const QString & ) ),
             this,                        SLOT( changePmSymbol( const QString & ) ) );
    connect( m_ui->m_buttonResetPmSymbol, SIGNAL( clicked() ),
             this,                        SLOT( defaultPmSymbol() ) );

    connect( m_ui->m_comboDateFormat,       SIGNAL( editTextChanged( const QString & ) ),
             this,                          SLOT( changeDateFormat( const QString & ) ) );
    connect( m_ui->m_buttonResetDateFormat, SIGNAL( clicked() ),
             this,                          SLOT( defaultDateFormat() ) );

    connect( m_ui->m_comboShortDateFormat,       SIGNAL( editTextChanged( const QString & ) ),
             this,                               SLOT( changeShortDateFormat( const QString & ) ) );
    connect( m_ui->m_buttonResetShortDateFormat, SIGNAL( clicked() ),
             this,                               SLOT( defaultShortDateFormat() ) );

    connect( m_ui->m_checkMonthNamePossessive,       SIGNAL( clicked( bool ) ),
             this,                                   SLOT( changeMonthNamePossessive( bool ) ) );
    connect( m_ui->m_buttonResetMonthNamePossessive, SIGNAL( clicked() ),
             this,                                   SLOT( defaultMonthNamePossessive() ) );

    connect( m_ui->m_comboDateTimeDigitSet,       SIGNAL( currentIndexChanged( int ) ),
             this,                                SLOT( changedDateTimeDigitSetIndex( int ) ) );
    connect( m_ui->m_buttonResetDateTimeDigitSet, SIGNAL( clicked() ),
             this,                                SLOT( defaultDateTimeDigitSet() ) );

    // Other tab

    connect( m_ui->m_comboPageSize,       SIGNAL( currentIndexChanged( int ) ),
             this,                        SLOT( changedPageSizeIndex( int ) ) );
    connect( m_ui->m_buttonResetPageSize, SIGNAL( clicked() ),
             this,                        SLOT( defaultPageSize() ) );

    connect( m_ui->m_comboMeasureSystem,       SIGNAL( currentIndexChanged( int ) ),
             this,                             SLOT( changedMeasureSystemIndex( int ) ) );
    connect( m_ui->m_buttonResetMeasureSystem, SIGNAL( clicked() ),
             this,                             SLOT( defaultMeasureSystem() ) );

    connect( m_ui->m_comboBinaryUnitDialect,       SIGNAL( currentIndexChanged( int ) ),
             this,                                 SLOT( changedBinaryUnitDialectIndex( int ) ) );
    connect( m_ui->m_buttonResetBinaryUnitDialect, SIGNAL( clicked() ),
             this,                                 SLOT( defaultBinaryUnitDialect() ) );
}

KCMLocale::~KCMLocale()
{
    // Throw away any unsaved changes as delete calls an unwanted sync()
    m_kcmConfig->markAsClean();
    m_userConfig->markAsClean();
    m_defaultConfig->markAsClean();
    delete m_kcmLocale;
    delete m_ui;
}

// Init all the config/settings objects, only called at the very start but inits everything so
// load() and mergeSettings() can assume everything exists every time they are called
void KCMLocale::initSettings()
{
    // Setup the KCM Config/Settings
    // These are the effective settings merging KCM Changes, User, Group, Country, and C settings
    // This will be used to display current state of settings in the KCM
    // These settings should never be saved anywhere
    m_kcmConfig = KSharedConfig::openConfig( "kcmlocale-kcm", KConfig::SimpleConfig );
    m_kcmSettings = KConfigGroup( m_kcmConfig, "Locale" );
    m_kcmSettings.deleteGroup();
    m_kcmSettings.markAsClean();

    // Setup the Default Config/Settings
    // These will be a merge of the C, Country and Group settings
    // If the user clicks on the Defaults button, these are the settings that will be used
    // These settings should never be saved anywhere
    m_defaultConfig = KSharedConfig::openConfig( "kcmlocale-default", KConfig::SimpleConfig );
    m_defaultSettings = KConfigGroup( m_defaultConfig, "Locale" );

    // Setup the User Config/Settings
    // These are the User overrides, they exclude any Group, Country, or C settings
    // This will be used to store the User changes
    // These are the only settings that should ever be saved
    m_userConfig = KSharedConfig::openConfig( "kcmlocale-user", KConfig::IncludeGlobals );
    m_userSettings = KConfigGroup( m_userConfig, "Locale" );

    // Setup the Current Config/Settings
    // These are the currently saved User settings
    // This will be used to check if the kcm settings have been changed
    // These settings should never be saved anywhere
    m_currentConfig = KSharedConfig::openConfig( "kcmlocale-current", KConfig::IncludeGlobals );
    m_currentSettings = KConfigGroup( m_currentConfig, "Locale" );

    // Setup the Group Config/Settings
    // These are the Group overrides, they exclude any User, Country, or C settings
    // This will be used in the merge to obtain the KCM Defaults
    // These settings should never be saved anywhere
    m_groupConfig = new KConfig( "kcmlocale-group", KConfig::NoGlobals );
    m_groupSettings = KConfigGroup( m_groupConfig, "Locale" );

    // Setup the C Config Settings
    // These are the C/Posix defaults and KDE defaults where a setting doesn't exist in Posix
    // This will be used as the lowest level in the merge to obtain the KCM Defaults
    // These settings should never be saved anywhere
    m_cConfig = new KConfig( KStandardDirs::locate( "locale",
                             QString::fromLatin1("l10n/C/entry.desktop") ) );
    m_cSettings= KConfigGroup( m_cConfig, "KCM Locale" );

    initCountrySettings( KGlobal::locale()->country() );

    initCalendarSettings();

    m_kcmLocale = new KLocale( QLatin1String("kcmlocale"), m_kcmConfig );
    // Find out the system country using a null config
    m_systemCountry = m_kcmLocale->country();
}

void KCMLocale::initCountrySettings( const QString &countryCode )
{
    // Setup a the Country Config/Settings
    // These are the Country overrides, they exclude any User, Group, or C settings
    // This will be used in the merge to obtain the KCM Defaults
    // These settings should never be saved anywhere
    delete m_countryConfig;
    m_countryConfig = new KConfig( KStandardDirs::locate( "locale",
                                   QString::fromLatin1("l10n/%1/entry.desktop")
                                   .arg( countryCode ) ) );
    m_countrySettings = KConfigGroup( m_countryConfig, "KCM Locale" );
    QString calendarType = m_countrySettings.readEntry( "CalendarSystem", "gregorian" );
    QString calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_countryCalendarSettings = m_countrySettings.group( calendarGroup );
}

// Init all the calendar settings sub-group objects, called both at the start and whenever the
// calendar system is changed
void KCMLocale::initCalendarSettings()
{
    // Setup the User Config/Settings
    // These are the User overrides, they exclude any Group, Country, or C settings
    // This will be used to store the User changes
    QString calendarType = m_kcmSettings.readEntry( "CalendarSystem", QString() );
    QString calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_userCalendarSettings = m_userSettings.group( calendarGroup );

    // Setup the Current Config/Settings
    // These are the currently saved User settings
    // This will be used to check if the kcm settings have been changed
    calendarType = m_currentSettings.readEntry( "CalendarSystem", KGlobal::locale()->calendar()->calendarType() );
    calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_currentCalendarSettings = m_currentSettings.group( calendarGroup );

    // Setup the Group Config/Settings
    // These are the Group overrides, they exclude any User, Country, or C settings
    // This will be used in the merge to obtain the KCM Defaults
    // These settings should never be saved anywhere
    calendarType = m_groupSettings.readEntry( "CalendarSystem", KGlobal::locale()->calendar()->calendarType() );
    calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_groupCalendarSettings = m_groupSettings.group( calendarGroup );

    // Setup the C Config Settings
    // These are the C/Posix defaults and KDE defaults where a setting doesn't exist in Posix
    // This will be used as the lowest level in the merge to obtain the KCM Defaults
    // These settings should never be saved anywhere
    calendarType = m_cSettings.readEntry( "CalendarSystem", QString() );
    calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_cCalendarSettings = m_cSettings.group( calendarGroup );

    // Setup a the Country Config/Settings
    // These are the Country overrides, they exclude any User, Group, or C settings
    // This will be used in the merge to obtain the KCM Defaults
    // These settings should never be saved anywhere
    calendarType = m_countrySettings.readEntry( "CalendarSystem", "gregorian" );
    calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_countryCalendarSettings = m_countrySettings.group( calendarGroup );

}

// Load == User has clicked on Reset to restore load saved settings
// Also gets called automatically called after constructor
void KCMLocale::load()
{
    // Throw away any unsaved changes then reload from file
    m_userConfig->markAsClean();
    m_userConfig->reparseConfiguration();
    m_currentConfig->reparseConfiguration();

    // Then create the new settings using the default, include user settings
    mergeSettings();

    // Save the current translations for checking later
    m_currentTranslations.clear();
    m_currentTranslations = m_kcmSettings.readEntry( "Language", QString() );

    // The update all the widgets to use the new settings
    initAllWidgets();
}

// Defaults == User has clicked on Defaults to load default settings
// We interpret this to mean the defaults for the system country and language
void KCMLocale::defaults()
{
    // Clear out the user config but dont sync or reparse as we want to ignore the user settings
    m_userCalendarSettings.deleteGroup( KConfig::Persistent | KConfig::Global );
    m_userSettings.deleteGroup( KConfig::Persistent | KConfig::Global );

    // Reload the system country
    initCountrySettings( m_systemCountry );

    // Then create the new settings using the default, exclude user settings
    mergeSettings();

    // Save the current translations for checking later
    m_currentTranslations.clear();
    m_currentTranslations = m_kcmSettings.readEntry( "Language", QString() );

    // The update all the widgets to use the new settings
    initAllWidgets();
}

// Write our own copy routine as we need to be selective about what values get copied, e.g. exclude
// Name, Region, etc, and copyTo() seems to barf on some uses
void KCMLocale::copySettings( KConfigGroup *fromGroup, KConfigGroup *toGroup, KConfig::WriteConfigFlags flags )
{
    copySetting( fromGroup, toGroup, "Country", flags );
    copySetting( fromGroup, toGroup, "CountryDivision", flags );
    copySetting( fromGroup, toGroup, "Language", flags );
    copySetting( fromGroup, toGroup, "DecimalPlaces", flags );
    copySetting( fromGroup, toGroup, "DecimalSymbol", flags );
    copySetting( fromGroup, toGroup, "ThousandsSeparator", flags );
    copySetting( fromGroup, toGroup, "PositiveSign", flags );
    copySetting( fromGroup, toGroup, "NegativeSign", flags );
    copySetting( fromGroup, toGroup, "DigitSet", flags );
    copySetting( fromGroup, toGroup, "CurrencyCode", flags );
    copySetting( fromGroup, toGroup, "CurrencySymbol", flags );
    copySetting( fromGroup, toGroup, "MonetaryDecimalPlaces", flags );
    copySetting( fromGroup, toGroup, "MonetaryDecimalSymbol", flags );
    copySetting( fromGroup, toGroup, "MonetaryThousandsSeparator", flags );
    copySetting( fromGroup, toGroup, "PositivePrefixCurrencySymbol", flags );
    copySetting( fromGroup, toGroup, "NegativePrefixCurrencySymbol", flags );
    copySetting( fromGroup, toGroup, "PositiveMonetarySignPosition", flags );
    copySetting( fromGroup, toGroup, "NegativeMonetarySignPosition", flags );
    copySetting( fromGroup, toGroup, "MonetaryDigitSet", flags );
    copySetting( fromGroup, toGroup, "CalendarSystem", flags );
    copySetting( fromGroup, toGroup, "TimeFormat", flags );
    copySetting( fromGroup, toGroup, "DateFormat", flags );
    copySetting( fromGroup, toGroup, "DateFormatShort", flags );
    copySetting( fromGroup, toGroup, "DateMonthNamePossessive", flags );
    copySetting( fromGroup, toGroup, "WeekStartDay", flags );
    copySetting( fromGroup, toGroup, "WorkingWeekStartDay", flags );
    copySetting( fromGroup, toGroup, "WorkingWeekEndDay", flags );
    copySetting( fromGroup, toGroup, "WeekDayOfPray", flags );
    copySetting( fromGroup, toGroup, "DateTimeDigitSet", flags );
    copySetting( fromGroup, toGroup, "BinaryUnitDialect", flags );
    copySetting( fromGroup, toGroup, "PageSize", flags );
    copySetting( fromGroup, toGroup, "MeasureSystem", flags );
}

void KCMLocale::copyCalendarSettings( KConfigGroup *fromGroup, KConfigGroup *toGroup, KConfig::WriteConfigFlags flags )
{
    copySetting( fromGroup, toGroup, "ShortYearWindowStartYear", flags );
    copySetting( fromGroup, toGroup, "UseCommonEra", flags );
    QString eraKey = QString::fromLatin1("Era1");
    int i = 1;
    while ( fromGroup->hasKey( eraKey ) ) {
        copySetting( fromGroup, toGroup, eraKey, flags );
        ++i;
        eraKey = QString::fromLatin1("Era%1").arg( i );
    }
}

void KCMLocale::copySetting( KConfigGroup *fromGroup, KConfigGroup *toGroup, const QString &key, KConfig::WriteConfigFlags flags )
{
    if ( fromGroup->hasKey( key ) ) {
        toGroup->writeEntry( key, fromGroup->readEntry( key, QString() ), flags );
    }
}

void KCMLocale::mergeSettings()
{
    // Merge the default settings, i.e. C, Country, and Group
    m_defaultSettings.deleteGroup();
    copySettings( &m_cSettings, &m_defaultSettings );
    copySettings( &m_countrySettings, &m_defaultSettings );
    copySettings( &m_groupSettings, &m_defaultSettings );
    m_defaultConfig->markAsClean();

    // Merge the KCM settings, i.e. C, Country, Group, and User
    m_kcmSettings.deleteGroup();
    copySettings( &m_defaultSettings, &m_kcmSettings );
    copySettings( &m_userSettings, &m_kcmSettings );

    // Merge the calendar settings sub-groups
    mergeCalendarSettings();

    // Reload the kcm locale from the kcm settings
    // Need to mark as clean before passing into a KLocale, as that will force a setLocale() which does a sync()
    m_kcmConfig->markAsClean();
    m_kcmLocale->setCountry( m_kcmSettings.readEntry( "Country", QString() ), m_kcmConfig.data() );

    // Create the kcm translations list
    m_kcmTranslations.clear();
    m_kcmTranslations = m_kcmSettings.readEntry( "Language", QString() ).split( ':', QString::SkipEmptyParts );
}

void KCMLocale::mergeCalendarSettings()
{
    // Merge the default settings, i.e. C, Country, and Group
    QString calendarType = m_defaultSettings.readEntry( "CalendarSystem", "gregorian" );
    QString calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_defaultCalendarSettings = m_defaultSettings.group( calendarGroup );
    m_defaultCalendarSettings.deleteGroup();
    copyCalendarSettings( &m_cCalendarSettings, &m_defaultCalendarSettings );
    copyCalendarSettings( &m_countryCalendarSettings, &m_defaultCalendarSettings );
    copyCalendarSettings( &m_groupCalendarSettings, &m_defaultCalendarSettings );

    // Merge the KCM settings, i.e. C, Country, Group, and User
    calendarType = m_kcmSettings.readEntry( "CalendarSystem", "gregorian" );
    calendarGroup = QString::fromLatin1( "KCalendarSystem %1" ).arg( calendarType );
    m_kcmCalendarSettings = m_kcmSettings.group( calendarGroup );
    m_kcmCalendarSettings.deleteGroup();
    copyCalendarSettings( &m_defaultCalendarSettings, &m_kcmCalendarSettings );
    copyCalendarSettings( &m_userCalendarSettings, &m_kcmCalendarSettings );
}

void KCMLocale::save()
{
    m_userConfig->sync();

    // rebuild the date base if language was changed
    if ( m_currentTranslations != m_kcmSettings.readEntry( "Language", QString() ) ) {
        KMessageBox::information(this, ki18n("Changed language settings apply only to "
                                            "newly started applications.\nTo change the "
                                            "language of all programs, you will have to "
                                            "logout first.").toString(m_kcmLocale),
                                ki18n("Applying Language Settings").toString(m_kcmLocale),
                                QLatin1String("LanguageChangesApplyOnlyToNewlyStartedPrograms"));
        KBuildSycocaProgressDialog::rebuildKSycoca(this);
    }

    load();
}

QString KCMLocale::quickHelp() const
{
    return ki18n("<h1>Country/Region & Language</h1>\n"
                 "<p>Here you can set your localization settings such as language, "
                 "numeric formats, date and time formats, etc.  Choosing a country "
                 "will load a set of default formats which you can then change to "
                 "your personal preferences.  These personal preferences will remain "
                 "set even if you change the country.  The reset buttons allow you "
                 "to easily see where you have personal settings and to restore "
                 "those items to the country's default value.</p>"
                 ).toString(m_kcmLocale);
}

void KCMLocale::initAllWidgets()
{
    //Common
    initTabs();
    initSample();
    initResetButtons();

    //Country tab
    initCountry();
    initCountryDivision();

    //Translations tab
    initTranslations();
    initTranslationsInstall();

    initSettingsWidgets();
}

void KCMLocale::initSettingsWidgets()
{
    // Initialise the settings widgets with the default values whenever the country or language changes

    //Numeric tab
    initNumericThousandsSeparator();
    initNumericDecimalSymbol();
    initNumericDecimalPlaces();
    initNumericPositiveSign();
    initNumericNegativeSign();
    initNumericDigitSet();

    //Monetary tab
    initCurrencyCode();  // Also inits CurrencySymbol
    initMonetaryThousandsSeparator();
    initMonetaryDecimalSymbol();
    initMonetaryDecimalPlaces();
    initMonetaryPositiveFormat();
    initMonetaryNegativeFormat();
    initMonetaryDigitSet();

    //Calendar tab
    initCalendarSystem();
    // this also inits all the Calendar System dependent settings

    //Date/Time tab
    initTimeFormat();
    initAmSymbol();
    initPmSymbol();
    initDateFormat();
    initShortDateFormat();
    initMonthNamePossessive();
    initDateTimeDigitSet();

    //Other tab
    initPageSize();
    initMeasureSystem();
    initBinaryUnitDialect();

    checkIfChanged();
}

void KCMLocale::initResetButtons()
{
    KGuiItem defaultItem( QString(), "document-revert", ki18n( "Reset item to its default value" ).toString( m_kcmLocale ) );

    //Country tab
    m_ui->m_buttonResetCountry->setGuiItem( defaultItem );
    m_ui->m_buttonResetCountryDivision->setGuiItem( defaultItem );

    //Translations tab
    m_ui->m_buttonResetTranslations->setGuiItem( defaultItem );

    //Numeric tab
    m_ui->m_buttonResetThousandsSeparator->setGuiItem( defaultItem );
    m_ui->m_buttonResetDecimalSymbol->setGuiItem( defaultItem );
    m_ui->m_buttonResetDecimalPlaces->setGuiItem( defaultItem );
    m_ui->m_buttonResetPositiveSign->setGuiItem( defaultItem );
    m_ui->m_buttonResetNegativeSign->setGuiItem( defaultItem );
    m_ui->m_buttonResetDigitSet->setGuiItem( defaultItem );

    //Monetary tab
    m_ui->m_buttonResetCurrencyCode->setGuiItem( defaultItem );
    m_ui->m_buttonResetCurrencySymbol->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonetaryThousandsSeparator->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonetaryDecimalSymbol->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonetaryDecimalPlaces->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonetaryPositiveFormat->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonetaryNegativeFormat->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonetaryDigitSet->setGuiItem( defaultItem );

    //Calendar tab
    m_ui->m_buttonResetCalendarSystem->setGuiItem( defaultItem );
    m_ui->m_buttonResetCalendarGregorianUseCommonEra->setGuiItem( defaultItem );
    m_ui->m_buttonResetShortYearWindow->setGuiItem( defaultItem );
    m_ui->m_buttonResetWeekStartDay->setGuiItem( defaultItem );
    m_ui->m_buttonResetWorkingWeekStartDay->setGuiItem( defaultItem );
    m_ui->m_buttonResetWorkingWeekEndDay->setGuiItem( defaultItem );
    m_ui->m_buttonResetWeekDayOfPray->setGuiItem( defaultItem );

    //Date/Time tab
    m_ui->m_buttonResetTimeFormat->setGuiItem( defaultItem );
    m_ui->m_buttonResetAmSymbol->setGuiItem( defaultItem );
    m_ui->m_buttonResetPmSymbol->setGuiItem( defaultItem );
    m_ui->m_buttonResetDateFormat->setGuiItem( defaultItem );
    m_ui->m_buttonResetShortDateFormat->setGuiItem( defaultItem );
    m_ui->m_buttonResetMonthNamePossessive->setGuiItem( defaultItem );
    m_ui->m_buttonResetDateTimeDigitSet->setGuiItem( defaultItem );

    //Other tab
    m_ui->m_buttonResetPageSize->setGuiItem( defaultItem );
    m_ui->m_buttonResetMeasureSystem->setGuiItem( defaultItem );
    m_ui->m_buttonResetBinaryUnitDialect->setGuiItem( defaultItem );
}

void KCMLocale::checkIfChanged()
{
    if ( m_userSettings.keyList() != m_currentSettings.keyList() ||
         m_userCalendarSettings.keyList() != m_currentCalendarSettings.keyList() ) {
        emit changed( true );
    } else {
        foreach( const QString & key, m_currentSettings.keyList() ) {
            if ( m_userSettings.readEntry( key, QString() ) !=
                 m_currentSettings.readEntry( key, QString() ) ) {
                emit changed( true );
                return;
            }
        }
        foreach( const QString & key, m_currentCalendarSettings.keyList() ) {
            if ( m_userCalendarSettings.readEntry( key, QString() ) !=
                 m_currentCalendarSettings.readEntry( key, QString() ) ) {
                emit changed( true );
                return;
            }
        }
        emit changed( false );
    }
}

void KCMLocale::setItem( const QString itemKey, const QString &itemValue, QWidget *itemWidget, KPushButton *itemDefaultButton )
{
    // If the setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userSettings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemDefaultButton->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        // If the new value is not the default, then enable the default button
        m_kcmSettings.writeEntry( itemKey, itemValue );
        if ( itemValue != m_defaultSettings.readEntry( itemKey, QString() ) ) {
            m_userSettings.writeEntry( itemKey, itemValue, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( true );
        } else {
            m_userSettings.deleteEntry( itemKey, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( false );
        }
        checkIfChanged();
    }
}

void KCMLocale::setItem( const QString itemKey, int itemValue, QWidget *itemWidget, KPushButton *itemDefaultButton )
{
    // If the setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userSettings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemDefaultButton->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        // If the new value is not the default (i.e. is set in user), then save it and enable the default button
        m_kcmSettings.writeEntry( itemKey, itemValue );
        if ( itemValue != m_defaultSettings.readEntry( itemKey, 0 ) ) {
            m_userSettings.writeEntry( itemKey, itemValue, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( true );
        } else {  // Is the default so delete any user setting
            m_userSettings.deleteEntry( itemKey, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( false );
        }
        checkIfChanged();
    }
}

void KCMLocale::setItem( const QString itemKey, bool itemValue, QWidget *itemWidget, KPushButton *itemDefaultButton )
{
    // If the setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userSettings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemDefaultButton->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        // If the new value is not the default, then save it and enable the default button
        m_kcmSettings.writeEntry( itemKey, itemValue );
        if ( itemValue != m_defaultSettings.readEntry( itemKey, false ) ) {
            m_userSettings.writeEntry( itemKey, itemValue, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( true );
        } else {
            m_userSettings.deleteEntry( itemKey, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( false );
        }
        checkIfChanged();
    }
}

void KCMLocale::setCalendarItem( const QString itemKey, const QString &itemValue, QWidget *itemWidget, KPushButton *itemDefaultButton )
{
    // If the setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userCalendarSettings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemDefaultButton->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        // If the new value is not the default, then enable the default button
        m_kcmCalendarSettings.writeEntry( itemKey, itemValue );
        if ( itemValue != m_defaultCalendarSettings.readEntry( itemKey, QString() ) ) {
            m_userCalendarSettings.writeEntry( itemKey, itemValue, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( true );
        } else {
            m_userCalendarSettings.deleteEntry( itemKey, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( false );
        }
        checkIfChanged();
    }
}

void KCMLocale::setCalendarItem( const QString itemKey, int itemValue, QWidget *itemWidget, KPushButton *itemDefaultButton )
{
    // If the setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userCalendarSettings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemDefaultButton->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        // If the new value is not the default, then enable the default button
        m_kcmCalendarSettings.writeEntry( itemKey, itemValue );
        if ( itemValue != m_defaultCalendarSettings.readEntry( itemKey, 0 ) ) {
            m_userCalendarSettings.writeEntry( itemKey, itemValue, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( true );
        } else {
            m_userCalendarSettings.deleteEntry( itemKey, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( false );
        }
        checkIfChanged();
    }
}

void KCMLocale::setCalendarItem( const QString itemKey, bool itemValue, QWidget *itemWidget, KPushButton *itemDefaultButton )
{
    // If the setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userCalendarSettings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemDefaultButton->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        // If the new value is not the default, then enable the default button
        m_kcmCalendarSettings.writeEntry( itemKey, itemValue );
        if ( itemValue != m_defaultCalendarSettings.readEntry( itemKey, false ) ) {
            m_userCalendarSettings.writeEntry( itemKey, itemValue, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( true );
        } else {
            m_userCalendarSettings.deleteEntry( itemKey, KConfig::Persistent | KConfig::Global );
            itemDefaultButton->setEnabled( false );
        }
        checkIfChanged();
    }
}

void KCMLocale::setComboItem( const QString itemKey, const QString &itemValue, KComboBox *itemCombo, KPushButton *itemDefaultButton )
{
    setItem( itemKey, itemValue, itemCombo, itemDefaultButton );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    itemCombo->setCurrentIndex( itemCombo->findData( m_kcmSettings.readEntry( itemKey, QString() ) ) );
}

void KCMLocale::setComboItem( const QString itemKey, int itemValue, KComboBox *itemCombo, KPushButton *itemDefaultButton )
{
    setItem( itemKey, itemValue, itemCombo, itemDefaultButton );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    itemCombo->setCurrentIndex( itemCombo->findData( m_kcmSettings.readEntry( itemKey, 0 ) ) );
}

void KCMLocale::setEditComboItem( const QString itemKey, const QString &itemValue, KComboBox *itemCombo, KPushButton *itemDefaultButton )
{
    setItem( itemKey, itemValue, itemCombo, itemDefaultButton );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    itemCombo->setEditText( m_kcmSettings.readEntry( itemKey, QString() ) );
}

void KCMLocale::setEditComboItem( const QString itemKey, int itemValue, KComboBox *itemCombo, KPushButton *itemDefaultButton )
{
    setItem( itemKey, itemValue, itemCombo, itemDefaultButton );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    itemCombo->setEditText( QString::number( m_kcmSettings.readEntry( itemKey, 0 ) ) );
}

void KCMLocale::setIntItem( const QString itemKey, int itemValue, KIntNumInput *itemInput, KPushButton *itemDefaultButton )
{
    setItem( itemKey, itemValue, itemInput, itemDefaultButton );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    itemInput->setValue( m_kcmSettings.readEntry( itemKey, 0 ) );
}

void KCMLocale::setCheckItem( const QString itemKey, bool itemValue, QCheckBox *itemCheck, KPushButton *itemDefaultButton )
{
    setItem( itemKey, itemValue, itemCheck, itemDefaultButton );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    itemCheck->setChecked( m_kcmSettings.readEntry( itemKey, false ) );
}

void KCMLocale::initWeekDayCombo( KComboBox *dayCombo )
{
    dayCombo->clear();
    int daysInWeek = m_kcmLocale->calendar()->daysInWeek( QDate::currentDate() );
    for ( int i = 1; i <= daysInWeek; ++i )
    {
        dayCombo->insertItem( i - 1, m_kcmLocale->calendar()->weekDayName( i ), QVariant( i ) );
    }
}

void KCMLocale::initSeparatorCombo( KComboBox *separatorCombo )
{
    separatorCombo->clear();
    separatorCombo->addItem( ki18nc( "No separator symbol" , "None" ).toString( m_kcmLocale ), QString() );
    separatorCombo->addItem( QString(','), QString(',') );
    separatorCombo->addItem( QString('.'), QString('.') );
    separatorCombo->addItem( ki18nc( "Space separator symbol", "Single Space" ).toString( m_kcmLocale ), QString(' ') );
}

// Generic utility to set up a DigitSet combo, used for numbers, dates, money
void KCMLocale::initDigitSetCombo( KComboBox *digitSetCombo )
{
    digitSetCombo->clear();
    QList<KLocale::DigitSet> digitSets = m_kcmLocale->allDigitSetsList();
    foreach ( const KLocale::DigitSet &digitSet, digitSets )
    {
        digitSetCombo->addItem( m_kcmLocale->digitSetToName( digitSet, true ), QVariant( digitSet ) );
    }
}

void KCMLocale::initTabs()
{
    m_ui->m_tabWidgetSettings->setTabText( 0, ki18n( "Country" ).toString( m_kcmLocale ) );
    m_ui->m_tabWidgetSettings->setTabText( 1, ki18n( "Languages" ).toString( m_kcmLocale ) );
    m_ui->m_tabWidgetSettings->setTabText( 2, ki18n( "Numbers" ).toString( m_kcmLocale ) );
    m_ui->m_tabWidgetSettings->setTabText( 3, ki18n( "Money" ).toString( m_kcmLocale ) );
    m_ui->m_tabWidgetSettings->setTabText( 4, ki18n( "Calendar" ).toString( m_kcmLocale ) );
    m_ui->m_tabWidgetSettings->setTabText( 5, ki18n( "Date && Time" ).toString( m_kcmLocale ) );
    m_ui->m_tabWidgetSettings->setTabText( 6, ki18n( "Other" ).toString( m_kcmLocale ) );
}

void KCMLocale::initSample()
{
    m_ui->m_labelNumbersSample->setText( ki18n( "Numbers:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "This is how positive numbers will be displayed.").toString( m_kcmLocale );
    m_ui->m_textNumbersPositiveSample->setToolTip( helpText );
    m_ui->m_textNumbersPositiveSample->setWhatsThis( helpText );
    helpText = ki18n( "This is how negative numbers will be displayed.").toString( m_kcmLocale );
    m_ui->m_textNumbersNegativeSample->setToolTip( helpText );
    m_ui->m_textNumbersNegativeSample->setWhatsThis( helpText );

    m_ui->m_labelMoneySample->setText( ki18n( "Money:" ).toString( m_kcmLocale ) );
    helpText = ki18n( "This is how positive monetary values will be displayed.").toString( m_kcmLocale );
    m_ui->m_textMoneyPositiveSample->setToolTip( helpText );
    m_ui->m_textMoneyPositiveSample->setWhatsThis( helpText );
    helpText = ki18n( "This is how negative monetary values will be displayed.").toString( m_kcmLocale );
    m_ui->m_textMoneyPositiveSample->setToolTip( helpText );
    m_ui->m_textMoneyPositiveSample->setWhatsThis( helpText );

    m_ui->m_labelDateSample->setText( ki18n( "Date:" ).toString( m_kcmLocale ) );
    helpText = ki18n( "This is how long dates will be displayed.").toString( m_kcmLocale );
    m_ui->m_textDateSample->setToolTip( helpText );
    m_ui->m_textDateSample->setWhatsThis( helpText );

    m_ui->m_labelShortDateSample->setText( ki18n( "Short date:" ).toString( m_kcmLocale ) );
    helpText = ki18n( "This is how short dates will be displayed.").toString( m_kcmLocale );
    m_ui->m_textShortDateSample->setToolTip( helpText );
    m_ui->m_textShortDateSample->setWhatsThis( helpText );

    m_ui->m_labelTimeSample->setText( ki18n( "Time:" ).toString( m_kcmLocale ) );
    helpText = ki18n( "This is how time will be displayed.").toString( m_kcmLocale );
    m_ui->m_textTimeSample->setToolTip( helpText );
    m_ui->m_textTimeSample->setWhatsThis( helpText );

    QTimer *timer = new QTimer( this );
    timer->setObjectName( QLatin1String( "clock_timer" ) );
    connect( timer, SIGNAL( timeout() ), this, SLOT( updateSample() ) );
    timer->start( 1000 );
}

void KCMLocale::updateSample()
{
    m_ui->m_textNumbersPositiveSample->setText( m_kcmLocale->formatNumber( 123456.78 ) );
    m_ui->m_textNumbersNegativeSample->setText( m_kcmLocale->formatNumber( -123456.78 ) );

    m_ui->m_textMoneyPositiveSample->setText( m_kcmLocale->formatMoney( 123456.78 ) );
    m_ui->m_textMoneyNegativeSample->setText( m_kcmLocale->formatMoney( -123456.78 ) );

    KDateTime dateTime = KDateTime::currentLocalDateTime();
    m_ui->m_textDateSample->setText( m_kcmLocale->formatDate( dateTime.date(), KLocale::LongDate ) );
    m_ui->m_textShortDateSample->setText( m_kcmLocale->formatDate( dateTime.date(), KLocale::ShortDate ) );
    m_ui->m_textTimeSample->setText( m_kcmLocale->formatTime( dateTime.time(), true ) );
}

void KCMLocale::initCountry()
{
    m_ui->m_comboCountry->blockSignals( true );

    m_ui->m_labelCountry->setText( ki18n( "Country:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This is the country where you live.  The KDE Workspace will use "
                              "the settings for this country or region.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCountry->setToolTip( helpText );
    m_ui->m_comboCountry->setWhatsThis( helpText );

    m_ui->m_comboCountry->clear();

    QStringList countryCodes = m_kcmLocale->allCountriesList();
    countryCodes.removeDuplicates();
    QMap<QString, QString> countryNames;

    foreach ( const QString &countryCode, countryCodes ) {
        countryNames.insert( m_kcmLocale->countryCodeToName( countryCode ), countryCode );
    }

    QString systemCountryName = m_kcmLocale->countryCodeToName( m_systemCountry );
    QString systemCountry = ki18nc( "%1 is the system country name", "System Country (%1)" ).subs( systemCountryName ).toString( m_kcmLocale );
    m_ui->m_comboCountry->addItem( systemCountry , QString() );

    QString defaultLocale = ki18n( "No Country (Default Settings)" ).toString( m_kcmLocale );
    m_ui->m_comboCountry->addItem( defaultLocale , "C" );

    QMapIterator<QString, QString> it( countryNames );
    while ( it.hasNext() ) {
        it.next();
        KIcon flag( KStandardDirs::locate( "locale", QString::fromLatin1( "l10n/%1/flag.png" ).arg( it.value() ) ) );
        m_ui->m_comboCountry->addItem( flag, it.key(), it.value() );
    }

    changeCountry( m_kcmSettings.readEntry( "Country", QString() ) );

    m_ui->m_comboCountry->blockSignals( false );
}

void KCMLocale::defaultCountry()
{
    changeCountry( m_defaultSettings.readEntry( "Country", QString() ) );
}

void KCMLocale::changedCountryIndex( int index )
{
    m_ui->m_comboCountry->blockSignals( true );
    changeCountry( m_ui->m_comboCountry->itemData( index ).toString() );
    initCountrySettings( m_kcmSettings.readEntry( "Country", QString() ) );
    mergeSettings();
    m_ui->m_comboCountry->blockSignals( false );
    initSettingsWidgets();
}

void KCMLocale::changeCountry( const QString &newValue )
{
    setComboItem( "Country", newValue,
                  m_ui->m_comboCountry, m_ui->m_buttonResetCountry );
}

void KCMLocale::initCountryDivision()
{
    m_ui->m_comboCountryDivision->blockSignals( true );

    m_ui->m_labelCountryDivision->setText( ki18n( "Subdivision:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This is the country subdivision where you live, e.g. your state "
                              "or province.  The KDE Workspace will use this setting for local "
                              "information services such as holidays.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCountryDivision->setToolTip( helpText );
    m_ui->m_comboCountryDivision->setWhatsThis( helpText );

    changeCountryDivision( m_kcmSettings.readEntry( "CountryDivision", QString() ) );

    m_ui->m_labelCountryDivision->setHidden( true );
    m_ui->m_comboCountryDivision->setHidden( true );
    m_ui->m_buttonResetCountryDivision->setEnabled( false );
    m_ui->m_buttonResetCountryDivision->setHidden( true );

    m_ui->m_comboCountryDivision->blockSignals( false );
}

void KCMLocale::defaultCountryDivision()
{
    changeCountryDivision( m_defaultSettings.readEntry( "CountryDivision", QString() ) );
}

void KCMLocale::changedCountryDivisionIndex( int index )
{
    changeCountryDivision( m_ui->m_comboCountryDivision->itemData( index ).toString() );
}

void KCMLocale::changeCountryDivision( const QString &newValue )
{
    setComboItem( "CountryDivision", newValue,
                m_ui->m_comboCountryDivision, m_ui->m_buttonResetCountryDivision );
    m_kcmLocale->setCountryDivisionCode( m_kcmSettings.readEntry( "CountryDivision", QString() ) );
}

void KCMLocale::initTranslations()
{
kDebug() << "initTranslations()";
    m_ui->m_selectTranslations->blockSignals( true );

    m_ui->m_selectTranslations->setAvailableLabel( ki18n( "Available Languages:" ).toString( m_kcmLocale ) );
    QString availableHelp = ki18n( "<p>This is the list of installed KDE Workspace language "
                                   "translations not currently being used.  To use a language "
                                   "translation move it to the 'Preferred Languages' list in "
                                   "the order of preference.  If no suitable languages are "
                                   "listed, then you may need to install more language packages "
                                   "using your usual installation method.</p>" ).toString( m_kcmLocale );
    m_ui->m_selectTranslations->availableListWidget()->setToolTip( availableHelp );
    m_ui->m_selectTranslations->availableListWidget()->setWhatsThis( availableHelp );

    m_ui->m_selectTranslations->setSelectedLabel( ki18n( "Preferred Languages:" ).toString( m_kcmLocale ) );
    QString selectedHelp = ki18n( "<p>This is the list of installed KDE Workspace language "
                                  "translations currently being used, listed in order of "
                                  "preference.  If a translation is not available for the "
                                  "first language in the list, the next language will be used.  "
                                  "If no other translations are available then US English will "
                                  "be used.</p>").toString( m_kcmLocale );
    m_ui->m_selectTranslations->selectedListWidget()->setToolTip( selectedHelp );
    m_ui->m_selectTranslations->selectedListWidget()->setWhatsThis( selectedHelp );

    QString enUS;
    QString defaultLang = ki18nc( "%1 = default language name", "%1 (Default)" ).subs( enUS ).toString( m_kcmLocale );

    // Get the installed translations
    m_installedTranslations.clear();
    m_installedTranslations = m_kcmLocale->installedLanguages();
    // Get the users choice of languages
    m_kcmTranslations.clear();
    m_kcmTranslations = m_kcmSettings.readEntry( "Language" ).split( ':', QString::SkipEmptyParts );
    // Set up the locale, note it will throw away invalid langauges
    m_kcmLocale->setLanguage( m_kcmTranslations );

    // Check if any of the user requested translations are no longer installed
    // If any missing remove them, we'll tell the user later
    QStringList missingLanguages;
    foreach ( const QString &languageCode, m_kcmTranslations ) {
        if ( !m_installedTranslations.contains( languageCode ) ) {
            missingLanguages.append( languageCode );
            m_kcmTranslations.removeAll( languageCode );
        }
    }

    // Clear the selector before reloading
    m_ui->m_selectTranslations->availableListWidget()->clear();
    m_ui->m_selectTranslations->selectedListWidget()->clear();

    // Load each user selected language into the selected list
    int i = 0;
    foreach ( const QString &languageCode, m_kcmTranslations ) {
        QListWidgetItem *listItem = new QListWidgetItem( m_ui->m_selectTranslations->selectedListWidget() );
        listItem->setText( m_kcmLocale->languageCodeToName( languageCode ) );
        listItem->setData( Qt::UserRole, languageCode );
        ++i;
    }

    // Load all the available languages the user hasn't selected into the available list
    i = 0;
    foreach ( const QString &languageCode, m_installedTranslations ) {
        if ( !m_kcmTranslations.contains( languageCode ) ) {
            QListWidgetItem *listItem = new QListWidgetItem( m_ui->m_selectTranslations->availableListWidget() );
            listItem->setText( m_kcmLocale->languageCodeToName( languageCode ) );
            listItem->setData( Qt::UserRole, languageCode );
            ++i;
        }
    }

    // Default to selecting the first Selected language,
    // otherwise the first Available language,
    // otherwise no languages so disable all buttons
    if ( m_ui->m_selectTranslations->selectedListWidget()->count() > 1 ) {
        m_ui->m_selectTranslations->selectedListWidget()->setCurrentRow( 0 );
    } else if ( m_ui->m_selectTranslations->availableListWidget()->count() > 1 ) {
        m_ui->m_selectTranslations->availableListWidget()->setCurrentRow( 0 );
    }

    if ( m_kcmSettings.readEntry( "Language", QString() ) ==
         m_defaultSettings.readEntry( "Language", QString() ) ) {
        m_ui->m_buttonResetTranslations->setEnabled( false );
    } else {
        m_ui->m_buttonResetTranslations->setEnabled( true );
    }

    m_ui->m_selectTranslations->blockSignals( false );

    // Now tell the user about the missing languages
    foreach ( const QString &languageCode, missingLanguages ) {
        KMessageBox::information(this, ki18n("You have the language with code '%1' in your list "
                                             "of languages to use for translation but the "
                                             "localization files for it could not be found. The "
                                             "language has been removed from your configuration. "
                                             "If you want to add it again please install the "
                                             "localization files for it and add the language again.")
                                             .subs( languageCode ).toString( m_kcmLocale ) );
    }
kDebug() << "  done init";
}

void KCMLocale::defaultTranslations()
{
    changeTranslations( m_defaultSettings.readEntry( "Language", QString() ) );
}

void KCMLocale::changeTranslations()
{
    // Read the list of all Selected translations from the selector widget
    QStringList selectedTranslations;
    for ( int i = 0; i < m_ui->m_selectTranslations->selectedListWidget()->count(); ++i ) {
        selectedTranslations.append(  m_ui->m_selectTranslations->selectedListWidget()->item( i )->data( Qt::UserRole ).toString() );
    }

    changeTranslations( selectedTranslations.join( ":" ) );
}

void KCMLocale::changeTranslations( const QString &newValue )
{
    setItem( "Language", newValue, m_ui->m_selectTranslations, m_ui->m_buttonResetTranslations );

    // Create the kcm translations list
    m_kcmTranslations.clear();
    m_kcmTranslations = m_kcmSettings.readEntry( "Language", QString() ).split( ':', QString::SkipEmptyParts );
    m_kcmLocale->setLanguage( m_kcmTranslations );

    initAllWidgets();
}

void KCMLocale::initTranslationsInstall()
{
    m_ui->m_buttonTranslationsInstall->blockSignals( true );
    m_ui->m_buttonTranslationsInstall->setText( ki18n( "Install more languages" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Click here to install more languages</p>" ).toString( m_kcmLocale );
    m_ui->m_buttonTranslationsInstall->setToolTip( helpText );
    m_ui->m_buttonTranslationsInstall->setWhatsThis( helpText );
    m_ui->m_buttonTranslationsInstall->blockSignals( false );
}

void KCMLocale::installTranslations()
{
    // User has clicked Install Languages button, trigger distro specific install routine
}

void KCMLocale::initNumericThousandsSeparator()
{
    m_ui->m_comboThousandsSeparator->blockSignals( true );

    m_ui->m_labelThousandsSeparator->setText( ki18n( "Group separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the digit group separator used to display "
                              "numbers.</p><p>Note that the digit group separator used to display "
                              "monetary values has to be set separately (see the 'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboThousandsSeparator->setToolTip( helpText );
    m_ui->m_comboThousandsSeparator->setWhatsThis( helpText );

    initSeparatorCombo( m_ui->m_comboThousandsSeparator );

    changeNumericThousandsSeparator( m_kcmSettings.readEntry( "ThousandsSeparator", QString() ) );

    m_ui->m_comboThousandsSeparator->blockSignals( false );
}

void KCMLocale::defaultNumericThousandsSeparator()
{
    changeNumericThousandsSeparator( m_defaultSettings.readEntry( "ThousandsSeparator", QString() ) );
}

void KCMLocale::changeNumericThousandsSeparator( const QString &newValue )
{
    QString useValue = newValue;
    int item = m_ui->m_comboThousandsSeparator->findText( newValue );
    if ( item >= 0 ) {
        useValue = m_ui->m_comboThousandsSeparator->itemData( item ).toString();
    }
    setEditComboItem( "ThousandsSeparator", useValue,
                      m_ui->m_comboThousandsSeparator, m_ui->m_buttonResetThousandsSeparator );
    m_kcmLocale->setThousandsSeparator( m_kcmSettings.readEntry( "ThousandsSeparator", QString() ) );
}

void KCMLocale::initNumericDecimalSymbol()
{
    m_ui->m_comboDecimalSymbol->blockSignals( true );

    m_ui->m_labelDecimalSymbol->setText( ki18n( "Decimal separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the decimal separator used to display "
                              "numbers (i.e. a dot or a comma in most countries).</p><p>Note "
                              "that the decimal separator used to display monetary values has "
                              "to be set separately (see the 'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboDecimalSymbol->setToolTip( helpText );
    m_ui->m_comboDecimalSymbol->setWhatsThis( helpText );

    initSeparatorCombo( m_ui->m_comboDecimalSymbol );

    changeNumericDecimalSymbol( m_kcmSettings.readEntry( "DecimalSymbol", QString() ) );

    m_ui->m_comboDecimalSymbol->blockSignals( false );
}

void KCMLocale::defaultNumericDecimalSymbol()
{
    changeNumericDecimalSymbol( m_defaultSettings.readEntry( "DecimalSymbol", QString() ) );
}

void KCMLocale::changeNumericDecimalSymbol( const QString &newValue )
{
    QString useValue = newValue;
    int item = m_ui->m_comboDecimalSymbol->findText( newValue );
    if ( item >= 0 ) {
        useValue = m_ui->m_comboDecimalSymbol->itemData( item ).toString();
    }
    setEditComboItem( "DecimalSymbol", useValue,
                      m_ui->m_comboDecimalSymbol, m_ui->m_buttonResetDecimalSymbol );
    m_kcmLocale->setDecimalSymbol( m_kcmSettings.readEntry( "DecimalSymbol", QString() ) );
}

void KCMLocale::initNumericDecimalPlaces()
{
    m_ui->m_intDecimalPlaces->blockSignals( true );

    m_ui->m_labelDecimalPlaces->setText( ki18n( "Decimal places:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the number of decimal places displayed for "
                              "numeric values, i.e. the number of digits <em>after</em> the "
                              "decimal separator.</p><p>Note that the decimal places used "
                              "to display monetary values has to be set separately (see the "
                              "'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_intDecimalPlaces->setToolTip( helpText );
    m_ui->m_intDecimalPlaces->setWhatsThis( helpText );

    changeNumericDecimalPlaces( m_kcmSettings.readEntry( "DecimalPlaces", 0 ) );

    m_ui->m_intDecimalPlaces->blockSignals( false );
}

void KCMLocale::defaultNumericDecimalPlaces()
{
    changeNumericDecimalPlaces( m_defaultSettings.readEntry( "DecimalPlaces", 0 ) );
}

void KCMLocale::changeNumericDecimalPlaces( int newValue )
{
    setIntItem( "DecimalPlaces", newValue,
                m_ui->m_intDecimalPlaces, m_ui->m_buttonResetDecimalPlaces );
    m_kcmLocale->setDecimalPlaces( m_kcmSettings.readEntry( "DecimalPlaces", 0 ) );
}

void KCMLocale::initNumericPositiveSign()
{
    m_ui->m_comboPositiveSign->blockSignals( true );

    m_ui->m_labelPositiveFormat->setText( ki18n( "Positive sign:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can specify text used to prefix positive numbers. "
                              "Most locales leave this blank.</p><p>Note that the positive sign "
                              "used to display monetary values has to be set separately (see the "
                              "'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboPositiveSign->setToolTip( helpText );
    m_ui->m_comboPositiveSign->setWhatsThis( helpText );

    m_ui->m_comboPositiveSign->clear();
    m_ui->m_comboPositiveSign->addItem( ki18nc( "No positive symbol", "None" ).toString( m_kcmLocale ), QString() );
    m_ui->m_comboPositiveSign->addItem( QString('+'), QString('+') );

    changeNumericPositiveSign( m_kcmSettings.readEntry( "PositiveSign", QString() ) );

    m_ui->m_comboPositiveSign->blockSignals( false );
}

void KCMLocale::defaultNumericPositiveSign()
{
    changeNumericPositiveSign( m_defaultSettings.readEntry( "PositiveSign", QString() ) );
}

void KCMLocale::changeNumericPositiveSign( const QString &newValue )
{
    QString useValue = newValue;
    int item = m_ui->m_comboPositiveSign->findText( newValue );
    if ( item >= 0 ) {
        useValue = m_ui->m_comboPositiveSign->itemData( item ).toString();
    }
    setEditComboItem( "PositiveSign", useValue,
                      m_ui->m_comboPositiveSign, m_ui->m_buttonResetPositiveSign );
    m_kcmLocale->setPositiveSign( m_kcmSettings.readEntry( "PositiveSign", QString() ) );

    // Update the monetary format samples to relect new setting
    initMonetaryPositiveFormat();
}

void KCMLocale::initNumericNegativeSign()
{
    m_ui->m_comboNegativeSign->blockSignals( true );

    m_ui->m_labelNegativeFormat->setText( ki18n( "Negative sign:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can specify text used to prefix negative numbers. "
                              "This should not be empty, so you can distinguish positive and "
                              "negative numbers. It is normally set to minus (-).</p><p>Note "
                              "that the negative sign used to display monetary values has to "
                              "be set separately (see the 'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboNegativeSign->setToolTip( helpText );
    m_ui->m_comboNegativeSign->setWhatsThis( helpText );

    m_ui->m_comboNegativeSign->clear();
    m_ui->m_comboNegativeSign->addItem( ki18nc("No negative symbol", "None" ).toString( m_kcmLocale ), QString() );
    m_ui->m_comboNegativeSign->addItem( QString('-'), QString('-') );

    changeNumericNegativeSign( m_kcmSettings.readEntry( "NegativeSign", QString() ) );

    m_ui->m_comboNegativeSign->blockSignals( false );
}

void KCMLocale::defaultNumericNegativeSign()
{
    changeNumericNegativeSign( m_defaultSettings.readEntry( "NegativeSign", QString() ) );
}

void KCMLocale::changeNumericNegativeSign( const QString &newValue )
{
    QString useValue = newValue;
    int item = m_ui->m_comboNegativeSign->findText( newValue );
    if ( item >= 0 ) {
        useValue = m_ui->m_comboNegativeSign->itemData( item ).toString();
    }
    setEditComboItem( "NegativeSign", useValue,
                      m_ui->m_comboNegativeSign, m_ui->m_buttonResetNegativeSign );
    m_kcmLocale->setNegativeSign( m_kcmSettings.readEntry( "NegativeSign", QString() ) );

    // Update the monetary format samples to relect new setting
    initMonetaryNegativeFormat();
}

void KCMLocale::initNumericDigitSet()
{
    m_ui->m_comboDigitSet->blockSignals( true );

    m_ui->m_labelDigitSet->setText( ki18n( "Digit set:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the set of digits used to display numbers. "
                              "If digits other than Arabic are selected, they will appear only if "
                              "used in the language of the application or the piece of text where "
                              "the number is shown.</p> <p>Note that the set of digits used to "
                              "display monetary values has to be set separately (see the 'Money' "
                              "tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboDigitSet->setToolTip( helpText );
    m_ui->m_comboDigitSet->setWhatsThis( helpText );

    initDigitSetCombo( m_ui->m_comboDigitSet );

    changeNumericDigitSet( m_kcmSettings.readEntry( "DigitSet", 0 ) );

    m_ui->m_comboDigitSet->blockSignals( false );
}

void KCMLocale::defaultNumericDigitSet()
{
    changeNumericDigitSet( m_defaultSettings.readEntry( "DigitSet", 0 ) );
}

void KCMLocale::changedNumericDigitSetIndex( int index )
{
    changeNumericDigitSet( m_ui->m_comboDigitSet->itemData( index ).toInt() );
}

void KCMLocale::changeNumericDigitSet( int newValue )
{
    setComboItem( "DigitSet", newValue,
                  m_ui->m_comboDigitSet, m_ui->m_buttonResetDigitSet );
    m_kcmLocale->setDigitSet( (KLocale::DigitSet) m_kcmSettings.readEntry( "DigitSet", 0 ) );
}

void KCMLocale::initCurrencyCode()
{
    m_ui->m_comboCurrencyCode->blockSignals( true );

    m_ui->m_labelCurrencyCode->setText( ki18n( "Currency:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can choose the currency to be used when displaying "
                              "monetary values, e.g. United States Dollar or Pound Sterling.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCurrencyCode->setToolTip( helpText );
    m_ui->m_comboCurrencyCode->setWhatsThis( helpText );

    //Create the list of Currency Codes to choose from.
    //Visible text will be localised name plus unlocalised code.
    m_ui->m_comboCurrencyCode->clear();
    //First put all the preferred currencies first in order of priority
    QStringList currencyCodeList = m_kcmLocale->currencyCodeList();
    foreach ( const QString &currencyCode, currencyCodeList ) {
        QString text = ki18nc( "@item currency name and currency code", "%1 (%2)")
                       .subs( m_kcmLocale->currency()->currencyCodeToName( currencyCode ) )
                       .subs( currencyCode )
                       .toString( m_kcmLocale );
        m_ui->m_comboCurrencyCode->addItem( text, QVariant( currencyCode ) );
    }
    //Next put all currencies available in alphabetical name order
    m_ui->m_comboCurrencyCode->insertSeparator(m_ui->m_comboCurrencyCode->count());
    currencyCodeList = m_kcmLocale->currency()->allCurrencyCodesList();
    QStringList currencyNameList;
    foreach ( const QString &currencyCode, currencyCodeList ) {
        currencyNameList.append( ki18nc( "@item currency name and currency code", "%1 (%2)")
                                 .subs( m_kcmLocale->currency()->currencyCodeToName( currencyCode ) )
                                 .subs( currencyCode )
                                 .toString( m_kcmLocale ) );
    }
    currencyNameList.sort();
    foreach ( const QString &name, currencyNameList ) {
        m_ui->m_comboCurrencyCode->addItem( name, QVariant( name.mid( name.length()-4, 3 ) ) );
    }

    changeCurrencyCode( m_kcmSettings.readEntry( "CurrencyCode", QString() ) );

    m_ui->m_comboCurrencyCode->blockSignals( false );
}

void KCMLocale::defaultCurrencyCode()
{
    changeCurrencyCode( m_defaultSettings.readEntry( "CurrencyCode", QString() ) );
}

void KCMLocale::changedCurrencyCodeIndex( int index )
{
    changeCurrencyCode( m_ui->m_comboCurrencyCode->itemData( index ).toString() );
}

void KCMLocale::changeCurrencyCode( const QString &newValue )
{
    setComboItem( "CurrencyCode", newValue,
                  m_ui->m_comboCurrencyCode, m_ui->m_buttonResetCurrencyCode );
    m_kcmLocale->setCurrencyCode( m_kcmSettings.readEntry( "CurrencyCode", QString() ) );
    // Update the Currency dependent widgets with the new Currency details
    initCurrencySymbol();
}

void KCMLocale::initCurrencySymbol()
{
    m_ui->m_comboCurrencySymbol->blockSignals( true );

    m_ui->m_labelCurrencySymbol->setText( ki18n( "Currency symbol:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can choose the symbol to be used when displaying "
                              "monetary values, e.g. $, US$ or USD.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCurrencySymbol->setToolTip( helpText );
    m_ui->m_comboCurrencySymbol->setWhatsThis( helpText );

    //Create the list of Currency Symbols for the selected Currency Code
    m_ui->m_comboCurrencySymbol->clear();
    QStringList currencySymbolList = m_kcmLocale->currency()->symbolList();
    foreach ( const QString &currencySymbol, currencySymbolList ) {
        if ( currencySymbol == m_kcmLocale->currency()->defaultSymbol() ) {
            m_ui->m_comboCurrencySymbol->addItem( currencySymbol, QVariant( QString() ) );
        } else {
            m_ui->m_comboCurrencySymbol->addItem( currencySymbol, QVariant( currencySymbol ) );
        }
    }

    if ( !currencySymbolList.contains( m_kcmSettings.readEntry( "CurrencySymbol", QString() ) ) ) {
        m_kcmSettings.deleteEntry( "CurrencySymbol" );
        m_userSettings.deleteEntry( "CurrencySymbol", KConfig::Persistent | KConfig::Global );
    }

    changeCurrencySymbol( m_kcmSettings.readEntry( "CurrencySymbol", QString() ) );

    m_ui->m_comboCurrencySymbol->blockSignals( false );
}

void KCMLocale::defaultCurrencySymbol()
{
    changeCurrencySymbol( m_defaultSettings.readEntry( "CurrencySymbol", QString() ) );
}

void KCMLocale::changedCurrencySymbolIndex( int index )
{
    changeCurrencySymbol( m_ui->m_comboCurrencySymbol->itemData( index ).toString() );
}

void KCMLocale::changeCurrencySymbol( const QString &newValue )
{
    setComboItem( "CurrencySymbol", newValue,
                  m_ui->m_comboCurrencySymbol, m_ui->m_buttonResetCurrencySymbol );
    if ( m_kcmSettings.readEntry( "CurrencySymbol", QString() ) != QString() ) {
        m_kcmLocale->setCurrencySymbol( m_kcmSettings.readEntry( "CurrencySymbol", QString() ) );
    } else {
        m_kcmLocale->setCurrencySymbol( m_kcmLocale->currency()->defaultSymbol() );
    }

    // Update the monetary format samples to relect new setting
    initMonetaryPositiveFormat();
    initMonetaryNegativeFormat();
}

void KCMLocale::initMonetaryThousandsSeparator()
{
    m_ui->m_comboMonetaryThousandsSeparator->blockSignals( true );

    m_ui->m_labelMonetaryThousandsSeparator->setText( ki18n( "Group separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the group separator used to display monetary "
                              "values.</p><p>Note that the thousands separator used to display "
                              "other numbers has to be defined separately (see the 'Numbers' tab)."
                              "</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryThousandsSeparator->setToolTip( helpText );
    m_ui->m_comboMonetaryThousandsSeparator->setWhatsThis( helpText );

    initSeparatorCombo( m_ui->m_comboMonetaryThousandsSeparator );

    changeMonetaryThousandsSeparator( m_kcmSettings.readEntry( "MonetaryThousandsSeparator", QString() ) );

    m_ui->m_comboMonetaryThousandsSeparator->blockSignals( false );
}

void KCMLocale::defaultMonetaryThousandsSeparator()
{
    changeMonetaryThousandsSeparator( m_defaultSettings.readEntry( "MonetaryThousandsSeparator", QString() ) );
}

void KCMLocale::changeMonetaryThousandsSeparator( const QString &newValue )
{
    QString useValue = newValue;
    int item = m_ui->m_comboMonetaryThousandsSeparator->findText( newValue );
    if ( item >= 0 ) {
        useValue = m_ui->m_comboMonetaryThousandsSeparator->itemData( item ).toString();
    }
    setEditComboItem( "MonetaryThousandsSeparator", useValue,
                      m_ui->m_comboMonetaryThousandsSeparator, m_ui->m_buttonResetMonetaryThousandsSeparator );
    m_kcmLocale->setMonetaryThousandsSeparator( m_kcmSettings.readEntry( "MonetaryThousandsSeparator", QString() ) );

    // Update the monetary format samples to relect new setting
    initMonetaryPositiveFormat();
    initMonetaryNegativeFormat();
}

void KCMLocale::initMonetaryDecimalSymbol()
{
    m_ui->m_comboMonetaryDecimalSymbol->blockSignals( true );

    m_ui->m_labelMonetaryDecimalSymbol->setText( ki18n( "Decimal separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the decimal separator used to display "
                              "monetary values.</p><p>Note that the decimal separator used to "
                              "display other numbers has to be defined separately (see the "
                              "'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryDecimalSymbol->setToolTip( helpText );
    m_ui->m_comboMonetaryDecimalSymbol->setWhatsThis( helpText );

    initSeparatorCombo( m_ui->m_comboMonetaryDecimalSymbol );

    changeMonetaryDecimalSymbol( m_kcmSettings.readEntry( "MonetaryDecimalSymbol", QString() ) );

    m_ui->m_comboMonetaryDecimalSymbol->blockSignals( false );
}

void KCMLocale::defaultMonetaryDecimalSymbol()
{
    changeMonetaryDecimalSymbol( m_defaultSettings.readEntry( "MonetaryDecimalSymbol", QString() ) );
}

void KCMLocale::changeMonetaryDecimalSymbol( const QString &newValue )
{
    QString useValue = newValue;
    int item = m_ui->m_comboMonetaryDecimalSymbol->findText( newValue );
    if ( item >= 0 ) {
        useValue = m_ui->m_comboMonetaryDecimalSymbol->itemData( item ).toString();
    }
    setEditComboItem( "MonetaryDecimalSymbol", useValue,
                      m_ui->m_comboMonetaryDecimalSymbol, m_ui->m_buttonResetMonetaryDecimalSymbol );
    m_kcmLocale->setMonetaryDecimalSymbol( m_kcmSettings.readEntry( "MonetaryDecimalSymbol", QString() ) );

    // Update the monetary format samples to relect new setting
    initMonetaryPositiveFormat();
    initMonetaryNegativeFormat();
}

void KCMLocale::initMonetaryDecimalPlaces()
{
    m_ui->m_intMonetaryDecimalPlaces->blockSignals( true );

    m_ui->m_labelMonetaryDecimalPlaces->setText( ki18n( "Decimal places:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the number of decimal places displayed for "
                              "monetary values, i.e. the number of digits <em>after</em> the "
                              "decimal separator.</p><p>Note that the decimal places used to "
                              "display other numbers has to be defined separately (see the "
                              "'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_intMonetaryDecimalPlaces->setToolTip( helpText );
    m_ui->m_intMonetaryDecimalPlaces->setWhatsThis( helpText );

    changeMonetaryDecimalPlaces( m_kcmSettings.readEntry( "MonetaryDecimalPlaces", 0 ) );

    m_ui->m_intMonetaryDecimalPlaces->blockSignals( false );
}

void KCMLocale::defaultMonetaryDecimalPlaces()
{
    changeMonetaryDecimalPlaces( m_defaultSettings.readEntry( "MonetaryDecimalPlaces", 0 ) );
}

void KCMLocale::changeMonetaryDecimalPlaces( int newValue )
{
    setIntItem( "MonetaryDecimalPlaces", newValue,
                m_ui->m_intMonetaryDecimalPlaces, m_ui->m_buttonResetMonetaryDecimalPlaces );
    m_kcmLocale->setMonetaryDecimalPlaces( m_kcmSettings.readEntry( "MonetaryDecimalPlaces", 0 ) );

    // Update the monetary format samples to relect new setting
    initMonetaryPositiveFormat();
    initMonetaryNegativeFormat();
}

void KCMLocale::insertMonetaryPositiveFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition )
{
    KLocale custom( *m_kcmLocale );
    custom.setPositivePrefixCurrencySymbol( prefixCurrencySymbol );
    custom.setPositiveMonetarySignPosition( signPosition );
    QVariantList options;
    options.append( QVariant( prefixCurrencySymbol ) );
    options.append( QVariant( signPosition ) );
    m_ui->m_comboMonetaryPositiveFormat->addItem( custom.formatMoney( 123456.78 ), options );
}

void KCMLocale::initMonetaryPositiveFormat()
{
    m_ui->m_comboMonetaryPositiveFormat->blockSignals( true );

    m_ui->m_labelMonetaryPositiveFormat->setText( ki18n( "Positive format:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the format of positive monetary values.</p>"
                              "<p>Note that the positive sign used to display other numbers has "
                              "to be defined separately (see the 'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryPositiveFormat->setToolTip( helpText );
    m_ui->m_comboMonetaryPositiveFormat->setWhatsThis( helpText );

    m_ui->m_comboMonetaryPositiveFormat->clear();
    // If the positive sign is null, then all the sign options will look the same, so only show a
    // choice between parens and no sign, but preserve original position in case they do set sign
    // Also keep options in same order, i.e. sign options then parens option
    if ( m_kcmSettings.readEntry( "PositiveSign", QString() ).isEmpty() ) {
        KLocale::SignPosition currentSignPosition = (KLocale::SignPosition) m_currentSettings.readEntry( "PositiveMonetarySignPosition", 0 );
        KLocale::SignPosition kcmSignPosition = (KLocale::SignPosition) m_kcmSettings.readEntry( "PositiveMonetarySignPosition", 0 );
        if ( currentSignPosition == KLocale::ParensAround && kcmSignPosition == KLocale::ParensAround ) {
            //Both are parens, so also give a sign option
            insertMonetaryPositiveFormat( true, KLocale::BeforeQuantityMoney );
            insertMonetaryPositiveFormat( false, KLocale::BeforeQuantityMoney );
            insertMonetaryPositiveFormat( true, kcmSignPosition );
            insertMonetaryPositiveFormat( false, kcmSignPosition );
        } else if ( kcmSignPosition == KLocale::ParensAround ) {
            //kcm is parens, current is sign, use both in right order
            insertMonetaryPositiveFormat( true, currentSignPosition );
            insertMonetaryPositiveFormat( false, currentSignPosition );
            insertMonetaryPositiveFormat( true, kcmSignPosition );
            insertMonetaryPositiveFormat( false, kcmSignPosition );
        } else {
            // kcm is sign, current is parens, use both in right order
            insertMonetaryPositiveFormat( true, kcmSignPosition );
            insertMonetaryPositiveFormat( false, kcmSignPosition );
            insertMonetaryPositiveFormat( true, currentSignPosition );
            insertMonetaryPositiveFormat( false, currentSignPosition );
        }
    } else {
        // Show the sign options first, then parens
        // Could do a loop, but lets keep it simple
        insertMonetaryPositiveFormat( true, KLocale::BeforeQuantityMoney );
        insertMonetaryPositiveFormat( false, KLocale::BeforeQuantityMoney );
        insertMonetaryPositiveFormat( true, KLocale::AfterQuantityMoney );
        insertMonetaryPositiveFormat( false, KLocale::AfterQuantityMoney );
        insertMonetaryPositiveFormat( true, KLocale::BeforeMoney );
        insertMonetaryPositiveFormat( false, KLocale::BeforeMoney );
        insertMonetaryPositiveFormat( true, KLocale::AfterMoney );
        insertMonetaryPositiveFormat( false, KLocale::AfterMoney );
        insertMonetaryPositiveFormat( true, KLocale::ParensAround );
        insertMonetaryPositiveFormat( false, KLocale::ParensAround );
    }

    changeMonetaryPositiveFormat( m_kcmSettings.readEntry( "PositivePrefixCurrencySymbol", false ),
                                  (KLocale::SignPosition) m_defaultSettings.readEntry( "PositiveMonetarySignPosition", 0 ) );

    // These are the old strings, keep around for now in case new implementation isn't usable
    QString format = ki18n( "Sign position:" ).toString( m_kcmLocale );
    format = ki18n( "Parentheses Around" ).toString( m_kcmLocale );
    format = ki18n( "Before Quantity Money" ).toString( m_kcmLocale );
    format = ki18n( "After Quantity Money" ).toString( m_kcmLocale );
    format = ki18n( "Before Money" ).toString( m_kcmLocale );
    format = ki18n( "After Money" ).toString( m_kcmLocale );
    format = ki18n( "Here you can select how a positive sign will be "
                    "positioned. This only affects monetary values." ).toString( m_kcmLocale );

    QString check = ki18n( "Prefix currency symbol" ).toString( m_kcmLocale );
    check = ki18n( "If this option is checked, the currency sign "
                    "will be prefixed (i.e. to the left of the "
                    "value) for all positive monetary values. If "
                    "not, it will be postfixed (i.e. to the right)." ).toString( m_kcmLocale );

    m_ui->m_comboMonetaryPositiveFormat->blockSignals( false );
}

void KCMLocale::defaultMonetaryPositiveFormat()
{
    changeMonetaryPositiveFormat( m_defaultSettings.readEntry( "PositivePrefixCurrencySymbol", false ),
                                  (KLocale::SignPosition) m_defaultSettings.readEntry( "PositiveMonetarySignPosition", 0 ) );
}

void KCMLocale::changedMonetaryPositiveFormatIndex( int index )
{
    QVariantList options = m_ui->m_comboMonetaryPositiveFormat->itemData( index ).toList();
    bool prefixCurrencySymbol = options.at( 0 ).toBool();
    KLocale::SignPosition signPosition = (KLocale::SignPosition) options.at( 1 ).toInt();
    changeMonetaryPositiveFormat( prefixCurrencySymbol, signPosition );
}

void KCMLocale::setMonetaryFormat( const QString prefixCurrencySymbolKey, bool prefixCurrencySymbol,
                                   const QString signPositionKey, KLocale::SignPosition signPosition,
                                   QWidget *formatWidget, KPushButton *formatDefaultButton )
{
    // If either setting is locked down by Kiosk, then don't let the user make any changes, and disable the widgets
    if ( m_userSettings.isEntryImmutable( prefixCurrencySymbolKey ) ||
         m_userSettings.isEntryImmutable( signPositionKey ) ) {
            formatWidget->setEnabled( false );
            formatDefaultButton->setEnabled( false );
    } else {
        formatWidget->setEnabled( true );
        formatDefaultButton->setEnabled( false );

        m_kcmSettings.writeEntry( prefixCurrencySymbolKey, prefixCurrencySymbol );
        m_kcmSettings.writeEntry( signPositionKey, (int) signPosition );

        // If the new value is not the default (i.e. is set in user), then save it and enable the default button
        if ( prefixCurrencySymbol != m_defaultSettings.readEntry( prefixCurrencySymbolKey, false ) ) {
            m_userSettings.writeEntry( prefixCurrencySymbolKey, prefixCurrencySymbol, KConfig::Persistent | KConfig::Global );
            formatDefaultButton->setEnabled( true );
        } else {  // Is the default so delete any user setting
            m_userSettings.deleteEntry( prefixCurrencySymbolKey, KConfig::Persistent | KConfig::Global );
        }

        // If the new value is not the default (i.e. is set in user), then save it and enable the default button
        if ( signPosition != m_defaultSettings.readEntry( signPositionKey, 0 ) ) {
            m_userSettings.writeEntry( signPositionKey, (int) signPosition, KConfig::Persistent | KConfig::Global );
            formatDefaultButton->setEnabled( true );
        } else {  // Is the default so delete any user setting
            m_userSettings.deleteEntry( signPositionKey, KConfig::Persistent | KConfig::Global );
        }

        checkIfChanged();
    }
}

void KCMLocale::changeMonetaryPositiveFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition )
{
    setMonetaryFormat( "PositivePrefixCurrencySymbol", prefixCurrencySymbol,
                       "PositiveMonetarySignPosition", signPosition,
                       m_ui->m_comboMonetaryPositiveFormat, m_ui->m_buttonResetMonetaryPositiveFormat );

    // Read back the kcm values and use them in the sample locale
    prefixCurrencySymbol = m_kcmSettings.readEntry( "PositivePrefixCurrencySymbol", false );
    signPosition = (KLocale::SignPosition) m_kcmSettings.readEntry( "PositiveMonetarySignPosition", 0 );
    m_kcmLocale->setPositivePrefixCurrencySymbol( prefixCurrencySymbol );
    m_kcmLocale->setPositiveMonetarySignPosition( signPosition );

    // Set the combo to the kcm value
    QVariantList options;
    options.append( QVariant( prefixCurrencySymbol ) );
    options.append( QVariant( signPosition ) );
    int index = m_ui->m_comboMonetaryPositiveFormat->findData( options );
    m_ui->m_comboMonetaryPositiveFormat->setCurrentIndex( index );
}

void KCMLocale::insertMonetaryNegativeFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition )
{
    KLocale custom( *m_kcmLocale );
    custom.setNegativePrefixCurrencySymbol( prefixCurrencySymbol );
    custom.setNegativeMonetarySignPosition( signPosition );
    QVariantList options;
    options.append( QVariant( prefixCurrencySymbol ) );
    options.append( QVariant( signPosition ) );
    m_ui->m_comboMonetaryNegativeFormat->addItem( custom.formatMoney( -123456.78 ), options );
}

void KCMLocale::initMonetaryNegativeFormat()
{
    m_ui->m_comboMonetaryNegativeFormat->blockSignals( true );

    m_ui->m_labelMonetaryNegativeFormat->setText( ki18n( "Negative format:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the format of negative monetary values.</p>"
                              "<p>Note that the negative sign used to display other numbers has "
                              "to be defined separately (see the 'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryNegativeFormat->setToolTip( helpText );
    m_ui->m_comboMonetaryNegativeFormat->setWhatsThis( helpText );

    QString format = ki18n( "Sign position:" ).toString( m_kcmLocale );
    format = ki18n( "Parentheses Around" ).toString( m_kcmLocale );
    format = ki18n( "Before Quantity Money" ).toString( m_kcmLocale );
    format = ki18n( "After Quantity Money" ).toString( m_kcmLocale );
    format = ki18n( "Before Money" ).toString( m_kcmLocale );
    format = ki18n( "After Money" ).toString( m_kcmLocale );
    format = ki18n( "Here you can select how a negative sign will "
                    "be positioned. This only affects monetary "
                    "values." ).toString( m_kcmLocale );

    QString check = ki18n( "Prefix currency symbol" ).toString( m_kcmLocale );
    check = ki18n( "If this option is checked, the currency sign "
                    "will be prefixed (i.e. to the left of the "
                    "value) for all negative monetary values. If "
                    "not, it will be postfixed (i.e. to the right)." ).toString( m_kcmLocale );

    m_ui->m_comboMonetaryNegativeFormat->clear();
    // If the negative sign is null, then all the sign options will look the same, so only show a
    // choice between parens and no sign, but preserve original position in case they do set sign
    // Also keep options in same order, i.e. sign options then parens option
    if ( m_kcmSettings.readEntry( "NegativeSign", QString() ).isEmpty() ) {
        KLocale::SignPosition currentSignPosition = (KLocale::SignPosition) m_currentSettings.readEntry( "NegativeMonetarySignPosition", 0 );
        KLocale::SignPosition kcmSignPosition = (KLocale::SignPosition) m_kcmSettings.readEntry( "NegativeMonetarySignPosition", 0 );
        if ( currentSignPosition == KLocale::ParensAround && kcmSignPosition == KLocale::ParensAround ) {
            //Both are parens, so also give a sign option
            insertMonetaryNegativeFormat( true, KLocale::BeforeQuantityMoney );
            insertMonetaryNegativeFormat( false, KLocale::BeforeQuantityMoney );
            insertMonetaryNegativeFormat( true, kcmSignPosition );
            insertMonetaryNegativeFormat( false, kcmSignPosition );
        } else if ( kcmSignPosition == KLocale::ParensAround ) {
            //kcm is parens, current is sign, use both in right order
            insertMonetaryNegativeFormat( true, currentSignPosition );
            insertMonetaryNegativeFormat( false, currentSignPosition );
            insertMonetaryNegativeFormat( true, kcmSignPosition );
            insertMonetaryNegativeFormat( false, kcmSignPosition );
        } else {
            // kcm is sign, current is parens, use both in right order
            insertMonetaryNegativeFormat( true, kcmSignPosition );
            insertMonetaryNegativeFormat( false, kcmSignPosition );
            insertMonetaryNegativeFormat( true, currentSignPosition );
            insertMonetaryNegativeFormat( false, currentSignPosition );
        }
    } else {
        // Show the sign options first, then parens
        insertMonetaryNegativeFormat( true, KLocale::BeforeQuantityMoney );
        insertMonetaryNegativeFormat( false, KLocale::BeforeQuantityMoney );
        insertMonetaryNegativeFormat( true, KLocale::AfterQuantityMoney );
        insertMonetaryNegativeFormat( false, KLocale::AfterQuantityMoney );
        insertMonetaryNegativeFormat( true, KLocale::BeforeMoney );
        insertMonetaryNegativeFormat( false, KLocale::BeforeMoney );
        insertMonetaryNegativeFormat( true, KLocale::AfterMoney );
        insertMonetaryNegativeFormat( false, KLocale::AfterMoney );
        insertMonetaryNegativeFormat( true, KLocale::ParensAround );
        insertMonetaryNegativeFormat( false, KLocale::ParensAround );
    }

    changeMonetaryNegativeFormat( m_kcmSettings.readEntry( "NegativePrefixCurrencySymbol", false ),
                                  (KLocale::SignPosition) m_defaultSettings.readEntry( "NegativeMonetarySignPosition", 0 ) );

    m_ui->m_comboMonetaryNegativeFormat->blockSignals( false );
}

void KCMLocale::defaultMonetaryNegativeFormat()
{
    changeMonetaryNegativeFormat( m_defaultSettings.readEntry( "NegativePrefixCurrencySymbol", false ),
                                  (KLocale::SignPosition) m_defaultSettings.readEntry( "NegativeMonetarySignPosition", 0 ) );
}

void KCMLocale::changedMonetaryNegativeFormatIndex( int index )
{
    QVariantList options = m_ui->m_comboMonetaryNegativeFormat->itemData( index ).toList();
    bool prefixCurrencySymbol = options.at( 0 ).toBool();
    KLocale::SignPosition signPosition = (KLocale::SignPosition) options.at( 1 ).toInt();
    changeMonetaryNegativeFormat( prefixCurrencySymbol, signPosition );
}

void KCMLocale::changeMonetaryNegativeFormat( bool prefixCurrencySymbol, KLocale::SignPosition signPosition )
{
    setMonetaryFormat( "NegativePrefixCurrencySymbol", prefixCurrencySymbol,
                       "NegativeMonetarySignPosition", signPosition,
                       m_ui->m_comboMonetaryNegativeFormat, m_ui->m_buttonResetMonetaryNegativeFormat );

    // Read back the kcm values and use them in the sample locale
    prefixCurrencySymbol = m_kcmSettings.readEntry( "NegativePrefixCurrencySymbol", false );
    signPosition = (KLocale::SignPosition) m_kcmSettings.readEntry( "NegativeMonetarySignPosition", 0 );
    m_kcmLocale->setNegativePrefixCurrencySymbol( prefixCurrencySymbol );
    m_kcmLocale->setNegativeMonetarySignPosition( signPosition );

    // Set the combo to the kcm value
    QVariantList options;
    options.append( QVariant( prefixCurrencySymbol ) );
    options.append( QVariant( signPosition ) );
    int index = m_ui->m_comboMonetaryNegativeFormat->findData( options );
    m_ui->m_comboMonetaryNegativeFormat->setCurrentIndex( index );
}

void KCMLocale::initMonetaryDigitSet()
{
    m_ui->m_comboMonetaryDigitSet->blockSignals( true );

    m_ui->m_labelMonetaryDigitSet->setText( ki18n( "Digit set:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the set of digits used to display monetary "
                              "values. If digits other than Arabic are selected, they will appear "
                              "only if used in the language of the application or the piece of "
                              "text where the number is shown.</p><p>Note that the set of digits "
                              "used to display other numbers has to be defined separately (see the "
                              "'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryDigitSet->setToolTip( helpText );
    m_ui->m_comboMonetaryDigitSet->setWhatsThis( helpText );

    initDigitSetCombo( m_ui->m_comboMonetaryDigitSet );

    changeMonetaryDigitSet( m_kcmSettings.readEntry( "MonetaryDigitSet", 0 ) );

    m_ui->m_comboMonetaryDigitSet->blockSignals( false );
}

void KCMLocale::defaultMonetaryDigitSet()
{
    changeNumericDigitSet( m_defaultSettings.readEntry( "MonetaryDigitSet", 0 ) );
}

void KCMLocale::changedMonetaryDigitSetIndex( int index )
{
    changeMonetaryDigitSet( m_ui->m_comboMonetaryDigitSet->itemData( index ).toInt() );
}

void KCMLocale::changeMonetaryDigitSet( int newValue )
{
    setComboItem( "MonetaryDigitSet", newValue,
                  m_ui->m_comboMonetaryDigitSet, m_ui->m_buttonResetMonetaryDigitSet );
    m_kcmLocale->setMonetaryDigitSet( (KLocale::DigitSet) m_kcmSettings.readEntry( "MonetaryDigitSet", 0 ) );
}

void KCMLocale::initCalendarSystem()
{
    m_ui->m_comboCalendarSystem->blockSignals( true );

    m_ui->m_labelCalendarSystem->setText( ki18n( "Calendar system:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the Calendar System to use to display dates.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCalendarSystem->setToolTip( helpText );
    m_ui->m_comboCalendarSystem->setWhatsThis( helpText );

    m_ui->m_comboCalendarSystem->clear();

    QStringList calendarSystems = KCalendarSystem::calendarSystems();

    foreach ( const QString &calendarType, calendarSystems ) {
        m_ui->m_comboCalendarSystem->addItem( KCalendarSystem::calendarLabel(
                                              KCalendarSystem::calendarSystemForCalendarType( calendarType ), m_kcmLocale ),
                                              QVariant( calendarType ) );
    }

    changeCalendarSystem( m_kcmSettings.readEntry( "CalendarSystem", QString() ) );

    m_ui->m_comboCalendarSystem->blockSignals( false );
}

void KCMLocale::defaultCalendarSystem()
{
    changeCalendarSystem( m_defaultSettings.readEntry( "CalendarSystem", QString() ) );
}

void KCMLocale::changedCalendarSystemIndex( int index )
{
    changeCalendarSystem( m_ui->m_comboCalendarSystem->itemData( index ).toString() );
}

void KCMLocale::changeCalendarSystem( const QString &newValue )
{
    setComboItem( "CalendarSystem", newValue,
                  m_ui->m_comboCalendarSystem, m_ui->m_buttonResetCalendarSystem );

    // Load the correct settings group for the new calendar
    initCalendarSettings();
    mergeCalendarSettings();

    // If item was changed, i.e. not immutable, then update locale
    m_kcmLocale->setCalendar( m_kcmSettings.readEntry( "CalendarSystem", QString() ) );

    // Update the Calendar dependent widgets with the new Calendar System details
    initUseCommonEra();
    initShortYearWindow();
    initWeekStartDay();
    initWorkingWeekStartDay();
    initWorkingWeekEndDay();
    initWeekDayOfPray();
}

void KCMLocale::initUseCommonEra()
{
    m_ui->m_checkCalendarGregorianUseCommonEra->blockSignals( true );

    m_ui->m_checkCalendarGregorianUseCommonEra->setText( ki18n( "Use Common Era" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines if the Common Era (CE/BCE) should be used "
                              "instead of the Christian Era (AD/BC).</p>" ).toString( m_kcmLocale );
    m_ui->m_checkCalendarGregorianUseCommonEra->setToolTip( helpText );
    m_ui->m_checkCalendarGregorianUseCommonEra->setWhatsThis( helpText );

    QString calendarType = m_kcmSettings.readEntry( "CalendarSystem", QString() );
    if ( calendarType == "gregorian" || calendarType == "gregorian-proleptic" ) {
        changeUseCommonEra( m_kcmCalendarSettings.readEntry( "UseCommonEra", false ) );
    } else {
        m_ui->m_checkCalendarGregorianUseCommonEra->setChecked( false );
        m_ui->m_checkCalendarGregorianUseCommonEra->setEnabled( false );
        m_ui->m_buttonResetCalendarGregorianUseCommonEra->setEnabled( false );
    }

    m_ui->m_checkCalendarGregorianUseCommonEra->blockSignals( false );
}

void KCMLocale::defaultUseCommonEra()
{
    changeUseCommonEra( m_defaultCalendarSettings.readEntry( "UseCommonEra", false ) );
}

void KCMLocale::changeUseCommonEra( bool newValue )
{
    setCalendarItem( "UseCommonEra", newValue,
                     m_ui->m_checkCalendarGregorianUseCommonEra, m_ui->m_buttonResetCalendarGregorianUseCommonEra );
    m_ui->m_checkCalendarGregorianUseCommonEra->setChecked( m_kcmCalendarSettings.readEntry( "UseCommonEra", false ) );

    // No api to set, so need to force reload the locale
    m_kcmLocale->setCountry( m_kcmCalendarSettings.readEntry( "Country", QString() ), m_kcmConfig.data() );
    m_kcmLocale->setCalendar( m_kcmCalendarSettings.readEntry( "CalendarSystem", QString() ) );
}

void KCMLocale::initShortYearWindow()
{
    m_ui->m_intShortYearWindowStartYear->blockSignals( true );

    m_ui->m_labelShortYearWindow->setText( ki18n( "Short year window:" ).toString( m_kcmLocale ) );
    m_ui->m_labelShortYearWindowTo->setText( ki18nc( "label between two year inputs, i.e. 1930 to 2029", "to" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines what year range a two digit date is "
                              "interpreted as, for example with a range of 1950 to 2049 the "
                              "value 10 is interpreted as 2010.  This range is only applied when "
                              "reading the Short Year (YY) date format.</p>" ).toString( m_kcmLocale );
    m_ui->m_intShortYearWindowStartYear->setToolTip( helpText );
    m_ui->m_intShortYearWindowStartYear->setWhatsThis( helpText );
    m_ui->m_spinShortYearWindowEndYear->setToolTip( helpText );
    m_ui->m_spinShortYearWindowEndYear->setWhatsThis( helpText );

    changeShortYearWindow( m_kcmCalendarSettings.readEntry( "ShortYearWindowStartYear", 0 ) );

    m_ui->m_intShortYearWindowStartYear->blockSignals( false );
}

void KCMLocale::defaultShortYearWindow()
{
    changeShortYearWindow( m_defaultCalendarSettings.readEntry( "ShortYearWindowStartYear", 0 ) );
}

void KCMLocale::changeShortYearWindow( int newValue )
{
    setCalendarItem( "ShortYearWindowStartYear", newValue,
                     m_ui->m_intShortYearWindowStartYear, m_ui->m_buttonResetShortYearWindow );
    int startYear = m_kcmCalendarSettings.readEntry( "ShortYearWindowStartYear", 0 );
    m_ui->m_intShortYearWindowStartYear->setValue( startYear );
    m_ui->m_spinShortYearWindowEndYear->setValue( startYear + 99 );

    // No api to set, so need to force reload the locale and calendar
    m_kcmLocale->setCountry( m_kcmCalendarSettings.readEntry( "Country", QString() ), m_kcmConfig.data() );
    m_kcmLocale->setCalendar( m_kcmCalendarSettings.readEntry( "CalendarSystem", QString() ) );
}

void KCMLocale::initWeekStartDay()
{
    m_ui->m_comboWeekStartDay->blockSignals( true );

    m_ui->m_labelWeekStartDay->setText( ki18n( "First day of week:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day will be considered as the first "
                              "one of the week.</p> " ).toString( m_kcmLocale );
    m_ui->m_comboWeekStartDay->setToolTip( helpText );
    m_ui->m_comboWeekStartDay->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWeekStartDay );

    changeWeekStartDay( m_kcmSettings.readEntry( "WeekStartDay", 0 ) );

    m_ui->m_comboWeekStartDay->blockSignals( false );
}

void KCMLocale::defaultWeekStartDay()
{
    changeWeekStartDay( m_defaultSettings.readEntry( "WeekStartDay", 0 ) );
}

void KCMLocale::changedWeekStartDayIndex( int index )
{
    changeWeekStartDay( m_ui->m_comboWeekStartDay->itemData( index ).toInt() );
}

void KCMLocale::changeWeekStartDay( int newValue )
{
    setComboItem( "WeekStartDay", newValue,
                  m_ui->m_comboWeekStartDay, m_ui->m_buttonResetWeekStartDay );
    m_kcmLocale->setWeekStartDay( m_kcmSettings.readEntry( "WeekStartDay", 0 ) );
}

void KCMLocale::initWorkingWeekStartDay()
{
    m_ui->m_comboWorkingWeekStartDay->blockSignals( true );

    m_ui->m_labelWorkingWeekStartDay->setText( ki18n( "First working day of week:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day will be considered as the first "
                              "working day of the week.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboWorkingWeekStartDay->setToolTip( helpText );
    m_ui->m_comboWorkingWeekStartDay->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWorkingWeekStartDay );

    changeWorkingWeekStartDay( m_kcmSettings.readEntry( "WorkingWeekStartDay", 0 ) );

    m_ui->m_comboWorkingWeekStartDay->blockSignals( false );
}

void KCMLocale::defaultWorkingWeekStartDay()
{
    changeWorkingWeekStartDay( m_defaultSettings.readEntry( "WorkingWeekStartDay", 0 ) );
}

void KCMLocale::changedWorkingWeekStartDayIndex( int index )
{
    changeWorkingWeekStartDay( m_ui->m_comboWorkingWeekStartDay->itemData( index ).toInt() );
}

void KCMLocale::changeWorkingWeekStartDay( int newValue )
{
    setComboItem( "WorkingWeekStartDay", newValue,
                  m_ui->m_comboWorkingWeekStartDay, m_ui->m_buttonResetWorkingWeekStartDay );
    m_kcmLocale->setWorkingWeekStartDay( m_kcmSettings.readEntry( "WorkingWeekStartDay", 0 ) );
}

void KCMLocale::initWorkingWeekEndDay()
{
    m_ui->m_comboWorkingWeekEndDay->blockSignals( true );

    m_ui->m_labelWorkingWeekEndDay->setText( ki18n( "Last working day of week:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day will be considered as the last "
                              "working day of the week.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboWorkingWeekEndDay->setToolTip( helpText );
    m_ui->m_comboWorkingWeekEndDay->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWorkingWeekEndDay );

    changeWorkingWeekEndDay( m_kcmSettings.readEntry( "WorkingWeekEndDay", 0 ) );

    m_ui->m_comboWorkingWeekEndDay->blockSignals( false );
}

void KCMLocale::defaultWorkingWeekEndDay()
{
    changeWorkingWeekEndDay( m_defaultSettings.readEntry( "WorkingWeekEndDay", 0 ) );
}

void KCMLocale::changedWorkingWeekEndDayIndex( int index )
{
    changeWorkingWeekEndDay( m_ui->m_comboWorkingWeekEndDay->itemData( index ).toInt() );
}

void KCMLocale::changeWorkingWeekEndDay( int newValue )
{
    setComboItem( "WorkingWeekEndDay", newValue,
                  m_ui->m_comboWorkingWeekEndDay, m_ui->m_buttonResetWorkingWeekEndDay );
    m_kcmLocale->setWorkingWeekEndDay( m_kcmSettings.readEntry( "WorkingWeekEndDay", 0 ) );
}

void KCMLocale::initWeekDayOfPray()
{
    m_ui->m_comboWeekDayOfPray->blockSignals( true );

    m_ui->m_labelWeekDayOfPray->setText( ki18n( "Week day for special religious observance:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day if any will be considered as "
                              "the day of the week for special religious observance.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboWeekDayOfPray->setToolTip( helpText );
    m_ui->m_comboWeekDayOfPray->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWeekDayOfPray );
    m_ui->m_comboWeekDayOfPray->insertItem( 0, ki18nc( "Day name list, option for no special day of religious observance", "None / None in particular" ).toString( m_kcmLocale ) );

    changeWeekDayOfPray( m_kcmSettings.readEntry( "WeekDayOfPray", 0 ) );

    m_ui->m_comboWeekDayOfPray->blockSignals( false );
}

void KCMLocale::defaultWeekDayOfPray()
{
    changeWeekDayOfPray( m_defaultSettings.readEntry( "WeekDayOfPray", 0 ) );
}

void KCMLocale::changedWeekDayOfPrayIndex( int index )
{
    changeWeekDayOfPray( m_ui->m_comboWeekDayOfPray->itemData( index ).toInt() );
}

void KCMLocale::changeWeekDayOfPray( int newValue )
{
    setComboItem( "WeekDayOfPray", newValue,
                  m_ui->m_comboWeekDayOfPray, m_ui->m_buttonResetWeekDayOfPray );
    m_kcmLocale->setWeekDayOfPray( m_kcmSettings.readEntry( "WeekDayOfPray", 0 ) );
}

void KCMLocale::initTimeFormat()
{
    m_ui->m_comboTimeFormat->blockSignals( true );

    m_ui->m_labelTimeFormat->setText( ki18n( "Time format:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>The text in this textbox will be used to format time strings. "
                              "The sequences below will be replaced:</p>"
                              "<table>"
                              "<tr><td><b>HH</b></td>"
                                  "<td>The hour as a decimal number using a 24-hour clock (00-23).</td></tr>"
                              "<tr><td><b>hH</b></td>"
                                  "<td>The hour (24-hour clock) as a decimal number (0-23).</td></tr>"
                              "<tr><td><b>PH</b></td>"
                                  "<td>The hour as a decimal number using a 12-hour clock (01-12).</td></tr>"
                              "<tr><td><b>pH</b></td>"
                                  "<td>The hour (12-hour clock) as a decimal number (1-12).</td></tr>"
                              "<tr><td><b>MM</b></td>"
                                  "<td>The minutes as a decimal number (00-59).</td></tr>"
                              "<tr><td><b>SS</b></td>"
                                  "<td>The seconds as a decimal number (00-59).</td></tr>"
                              "<tr><td><b>AMPM</b></td>"
                                  "<td>Either 'AM' or 'PM' according to the given time value. "
                                      "Noon is treated as 'PM' and midnight as 'AM'.</td></tr>"
                              "</table>" ).toString( m_kcmLocale );
    m_ui->m_comboTimeFormat->setToolTip( helpText );
    m_ui->m_comboTimeFormat->setWhatsThis( helpText );

    m_timeFormatMap.clear();
    m_timeFormatMap.insert( QString( 'H' ), ki18n( "HH" ).toString( m_kcmLocale ) );
    m_timeFormatMap.insert( QString( 'k' ), ki18n( "hH" ).toString( m_kcmLocale ) );
    m_timeFormatMap.insert( QString( 'I' ), ki18n( "PH" ).toString( m_kcmLocale ) );
    m_timeFormatMap.insert( QString( 'l' ), ki18n( "pH" ).toString( m_kcmLocale ) );
    m_timeFormatMap.insert( QString( 'M' ), ki18nc( "Minute", "MM" ).toString( m_kcmLocale ) );
    m_timeFormatMap.insert( QString( 'S' ), ki18n( "SS" ).toString( m_kcmLocale ) );
    m_timeFormatMap.insert( QString( 'p' ), ki18n( "AMPM" ).toString( m_kcmLocale ) );

    QString formats =ki18nc( "some reasonable time formats for the language",
               "HH:MM:SS\n"
               "pH:MM:SS AMPM").toString( m_kcmLocale );

    changeTimeFormat( posixToUserTime( m_kcmSettings.readEntry( "TimeFormat", QString() ) ) );

    m_ui->m_comboTimeFormat->blockSignals( false );
}

void KCMLocale::defaultTimeFormat()
{
    changeTimeFormat( posixToUserTime( m_defaultSettings.readEntry( "TimeFormat", QString() ) ) );
}

void KCMLocale::changeTimeFormat( const QString &newValue )
{
    m_ui->m_comboTimeFormat->blockSignals( true );

    setItem( "TimeFormat", userToPosixTime( newValue ),
             m_ui->m_comboTimeFormat, m_ui->m_buttonResetTimeFormat );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    m_ui->m_comboTimeFormat->setEditText( posixToUserTime( m_kcmSettings.readEntry( "TimeFormat", QString() ) ) );

    m_ui->m_comboTimeFormat->blockSignals( false );

    m_kcmLocale->setTimeFormat( m_kcmSettings.readEntry( "TimeFormat", QString() ) );
}

void KCMLocale::initAmSymbol()
{
    m_ui->m_comboAmSymbol->blockSignals( true );
    m_ui->m_labelAmSymbol->setText( ki18n( "AM symbol:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the text to be displayed for AM.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboAmSymbol->setToolTip( helpText );
    m_ui->m_comboAmSymbol->setWhatsThis( helpText );
    m_ui->m_comboAmSymbol->blockSignals( false );
}

void KCMLocale::initPmSymbol()
{
    m_ui->m_comboPmSymbol->blockSignals( true );
    m_ui->m_labelPmSymbol->setText( ki18n( "PM symbol:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the text to be displayed for PM.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboPmSymbol->setToolTip( helpText );
    m_ui->m_comboPmSymbol->setWhatsThis( helpText );
    m_ui->m_comboPmSymbol->blockSignals( false );
}

void KCMLocale::initDateFormat()
{
    m_ui->m_comboDateFormat->blockSignals( true );

    m_ui->m_labelDateFormat->setText( ki18n( "Long date format:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>The text in this textbox will be used to format long dates. "
                              "The sequences below will be replaced:</p>"
                              "<table>"
                              "<tr>"
                                  "<td><b>YYYY</b></td>"
                                  "<td>The year with century as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>YY</b></td>"
                                  "<td>The year without century as a decimal number (00-99).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>MM</b></td>"
                                  "<td>The month as a decimal number (01-12).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>mM</b></td>"
                                  "<td>The month as a decimal number (1-12).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>SHORTMONTH</b></td>"
                                  "<td>The first three characters of the month name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>MONTH</b></td>"
                                  "<td>The full month name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>DD</b></td>"
                                  "<td>The day of month as a decimal number (01-31).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>dD</b></td>"
                                  "<td>The day of month as a decimal number (1-31).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>SHORTWEEKDAY</b></td>"
                                  "<td>The first three characters of the weekday name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>WEEKDAY</b></td>"
                                  "<td>The full weekday name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>ERAYEAR</b></td>"
                                  "<td>The Era Year in local format (e.g. 2000 AD).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>SHORTERANAME</b></td>"
                                  "<td>The short Era Name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>YEARINERA</b></td>"
                                  "<td>The Year in Era as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>DAYOFYEAR</b></td>"
                                  "<td>The Day of Year as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>ISOWEEK</b></td>"
                                  "<td>The ISO Week as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>DAYOFISOWEEK</b></td>"
                                  "<td>The Day of the ISO Week as a decimal number.</td>"
                              "</tr>"
                              "</table>" ).toString( m_kcmLocale );
    m_ui->m_comboDateFormat->setToolTip( helpText );
    m_ui->m_comboDateFormat->setWhatsThis( helpText );

    m_dateFormatMap.clear();
    m_dateFormatMap.insert( QString( 'Y' ), ki18n("YYYY").toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'y' ), ki18n( "YY" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'n' ), ki18n( "mM" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'm' ), ki18nc( "Month", "MM" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'b' ), ki18n( "SHORTMONTH" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'B' ), ki18n( "MONTH" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'e' ), ki18n( "dD" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'd' ), ki18n( "DD" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'a' ), ki18n( "SHORTWEEKDAY" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'A' ), ki18n( "WEEKDAY" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( "EY", ki18n( "ERAYEAR" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( "Ey", ki18n( "YEARINERA" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( "EC", ki18n( "SHORTERANAME" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'j' ), ki18n( "DAYOFYEAR" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'V' ), ki18n( "ISOWEEK" ).toString( m_kcmLocale ) );
    m_dateFormatMap.insert( QString( 'u' ), ki18n( "DAYOFISOWEEK" ).toString( m_kcmLocale ) );

    QString formats = ki18nc("some reasonable date formats for the language",
              "WEEKDAY MONTH dD YYYY\n"
              "SHORTWEEKDAY MONTH dD YYYY").toString( m_kcmLocale );

    changeDateFormat( posixToUserDate( m_kcmSettings.readEntry( "DateFormat", QString() ) ) );

    m_ui->m_comboDateFormat->blockSignals( true );
}

void KCMLocale::defaultDateFormat()
{
    changeDateFormat( posixToUserDate( m_defaultSettings.readEntry( "DateFormat", QString() ) ) );
}

void KCMLocale::changeDateFormat( const QString &newValue )
{
    m_ui->m_comboDateFormat->blockSignals( true );

    setItem( "DateFormat", userToPosixDate( newValue ),
             m_ui->m_comboDateFormat, m_ui->m_buttonResetDateFormat );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    m_ui->m_comboDateFormat->setEditText( posixToUserDate( m_kcmSettings.readEntry( "DateFormat", QString() ) ) );

    m_ui->m_comboDateFormat->blockSignals( false );

    m_kcmLocale->setDateFormat( m_kcmSettings.readEntry( "DateFormat", QString() ) );
}

void KCMLocale::initShortDateFormat()
{
    m_ui->m_comboShortDateFormat->blockSignals( true );

    m_ui->m_labelShortDateFormat->setText( ki18n( "Short date format:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>The text in this textbox will be used to format short dates. "
                              "For instance, this is used when listing files. The sequences below "
                              "will be replaced:</p>"
                              "<table>"
                              "<tr>"
                                  "<td><b>YYYY</b></td>"
                                  "<td>The year with century as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>YY</b></td>"
                                  "<td>The year without century as a decimal number (00-99).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>MM</b></td>"
                                  "<td>The month as a decimal number (01-12).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>mM</b></td>"
                                  "<td>The month as a decimal number (1-12).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>SHORTMONTH</b></td>"
                                  "<td>The first three characters of the month name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>MONTH</b></td>"
                                  "<td>The full month name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>DD</b></td>"
                                  "<td>The day of month as a decimal number (01-31).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>dD</b></td>"
                                  "<td>The day of month as a decimal number (1-31).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>SHORTWEEKDAY</b></td>"
                                  "<td>The first three characters of the weekday name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>WEEKDAY</b></td>"
                                  "<td>The full weekday name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>ERAYEAR</b></td>"
                                  "<td>The Era Year in local format (e.g. 2000 AD).</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>SHORTERANAME</b></td>"
                                  "<td>The short Era Name.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>YEARINERA</b></td>"
                                  "<td>The Year in Era as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>DAYOFYEAR</b></td>"
                                  "<td>The Day of Year as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>ISOWEEK</b></td>"
                                  "<td>The ISO Week as a decimal number.</td>"
                              "</tr>"
                              "<tr>"
                                  "<td><b>DAYOFISOWEEK</b></td>"
                                  "<td>The Day of the ISO Week as a decimal number.</td>"
                              "</tr>"
                              "</table>" ).toString( m_kcmLocale );
    m_ui->m_comboShortDateFormat->setToolTip( helpText );
    m_ui->m_comboShortDateFormat->setWhatsThis( helpText );

    QString formats = ki18nc("some reasonable short date formats for the language",
              "YYYY-MM-DD\n"
              "dD.mM.YYYY\n"
              "DD.MM.YYYY").toString( m_kcmLocale );

    changeShortDateFormat( posixToUserDate( m_kcmSettings.readEntry( "DateFormatShort", QString() ) ) );

    m_ui->m_comboShortDateFormat->blockSignals( false );
}

void KCMLocale::defaultShortDateFormat()
{
    changeShortDateFormat( posixToUserDate( m_defaultSettings.readEntry( "DateFormatShort", QString() ) ) );
}

void KCMLocale::changeShortDateFormat( const QString &newValue )
{
    m_ui->m_comboShortDateFormat->blockSignals( true );

    setItem( "DateFormatShort", userToPosixDate( newValue ),
             m_ui->m_comboShortDateFormat, m_ui->m_buttonResetShortDateFormat );
    // Read the entry rather than use itemValue in case setItem didn't change the value, e.g. if immutable
    m_ui->m_comboShortDateFormat->setEditText( posixToUserDate( m_kcmSettings.readEntry( "DateFormatShort", QString() ) ) );

    m_ui->m_comboShortDateFormat->blockSignals( false );

    m_kcmLocale->setDateFormatShort( m_kcmSettings.readEntry( "DateFormatShort", QString() ) );
}

void KCMLocale::initMonthNamePossessive()
{
    m_ui->m_checkMonthNamePossessive->blockSignals( true );

    m_ui->m_labelMonthNamePossessive->setText( ki18n( "Possessive month names:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines whether possessive form of month names "
                              "should be used in dates.</p>" ).toString( m_kcmLocale );
    m_ui->m_checkMonthNamePossessive->setToolTip( helpText );
    m_ui->m_checkMonthNamePossessive->setWhatsThis( helpText );

    m_ui->m_checkMonthNamePossessive->setChecked( m_kcmLocale->dateMonthNamePossessive() );
    setCheckItem( "DateMonthNamePossessive", m_kcmSettings.readEntry( "DateMonthNamePossessive", false ),
                  m_ui->m_checkMonthNamePossessive, m_ui->m_buttonResetMonthNamePossessive );

    changeMonthNamePossessive( m_kcmSettings.readEntry( "DateMonthNamePossessive", false ) );

    // Hide the option as it's not usable without ordinal day numbers
    m_ui->m_labelMonthNamePossessive->setHidden( true );
    m_ui->m_checkMonthNamePossessive->setHidden( true );
    m_ui->m_buttonResetMonthNamePossessive->setHidden( true );

    m_ui->m_checkMonthNamePossessive->blockSignals( false );
}

void KCMLocale::defaultMonthNamePossessive()
{
    changeMonthNamePossessive( m_defaultSettings.readEntry( "DateMonthNamePossessive", false ) );
}

void KCMLocale::changeMonthNamePossessive( bool newValue )
{
    setCheckItem( "DateMonthNamePossessive", newValue,
                  m_ui->m_checkMonthNamePossessive, m_ui->m_buttonResetMonthNamePossessive );
    m_kcmLocale->setDateMonthNamePossessive( m_kcmSettings.readEntry( "DateMonthNamePossessive", 0 ) );
}

void KCMLocale::initDateTimeDigitSet()
{
    m_ui->m_comboDateTimeDigitSet->blockSignals( true );

    m_ui->m_labelDateTimeDigitSet->setText( ki18n( "Digit set:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the set of digits used to display dates and "
                              "times.  If digits other than Arabic are selected, they will appear "
                              "only if used in the language of the application or the piece of "
                              "text where the date or time is shown.</p><p>Note that the set of "
                              "digits used to display numeric and monetary values have to be set "
                              "separately (see the 'Number' or 'Money' tabs).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboDateTimeDigitSet->setToolTip( helpText );
    m_ui->m_comboDateTimeDigitSet->setWhatsThis( helpText );

    initDigitSetCombo( m_ui->m_comboDateTimeDigitSet );

    changeDateTimeDigitSet( m_kcmSettings.readEntry( "DateTimeDigitSet", 0 ) );

    m_ui->m_comboDateTimeDigitSet->blockSignals( false );
}

void KCMLocale::defaultDateTimeDigitSet()
{
    changeDateTimeDigitSet( m_defaultSettings.readEntry( "DateTimeDigitSet", 0 ) );
}

void KCMLocale::changedDateTimeDigitSetIndex( int index )
{
    changeDateTimeDigitSet( m_ui->m_comboDateTimeDigitSet->itemData( index ).toInt() );
}

void KCMLocale::changeDateTimeDigitSet( int newValue )
{
    setComboItem( "DateTimeDigitSet", newValue,
                  m_ui->m_comboDateTimeDigitSet, m_ui->m_buttonResetDateTimeDigitSet );
    m_kcmLocale->setDateTimeDigitSet( (KLocale::DigitSet) m_kcmSettings.readEntry( "DateTimeDigitSet", 0 ) );
}

void KCMLocale::initPageSize()
{
    m_ui->m_comboPageSize->blockSignals( true );

    m_ui->m_labelPageSize->setText( ki18n( "Page size:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the default page size to be used in new "
                              "documents.</p><p>Note that this setting has no effect on printer "
                              "paper size.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboPageSize->setToolTip( helpText );
    m_ui->m_comboPageSize->setWhatsThis( helpText );

    m_ui->m_comboPageSize->clear();

    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A4").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A4 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "US Letter").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Letter ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A0").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A0 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A1").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A1 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A2").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A2 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A3").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A3 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A4").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A4 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A5").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A5 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A6").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A6 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A7").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A7 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A8").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A8 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "A9").toString( m_kcmLocale ),
                                    QVariant( QPrinter::A9 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B0").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B0 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B1").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B1 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B2").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B2 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B3").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B3 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B4").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B4 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B5").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B5 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B6").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B6 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B7").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B7 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B8").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B8 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B9").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B9 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "B10").toString( m_kcmLocale ),
                                    QVariant( QPrinter::B10 ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "C5 Envelope").toString( m_kcmLocale ),
                                    QVariant( QPrinter::C5E ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "US Common 10 Envelope").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Comm10E ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "DLE Envelope").toString( m_kcmLocale ),
                                    QVariant( QPrinter::DLE ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "Executive").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Executive ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "Folio").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Folio ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "Ledger").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Ledger ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "US Legal").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Legal ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "US Letter").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Letter ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "Tabloid").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Tabloid ) );
    m_ui->m_comboPageSize->addItem( ki18nc("Page size", "Custom").toString( m_kcmLocale ),
                                    QVariant( QPrinter::Custom ) );

    changePageSize( m_kcmSettings.readEntry( "PageSize", 0 ) );

    m_ui->m_comboPageSize->blockSignals( false );
}

void KCMLocale::defaultPageSize()
{
    changePageSize( m_defaultSettings.readEntry( "PageSize", 0 ) );
}

void KCMLocale::changedPageSizeIndex( int index )
{
    changePageSize( m_ui->m_comboPageSize->itemData( index ).toInt() );
}

void KCMLocale::changePageSize( int newValue )
{
    setComboItem( "PageSize", newValue,
                  m_ui->m_comboPageSize, m_ui->m_buttonResetPageSize );
    m_kcmLocale->setPageSize( m_kcmSettings.readEntry( "PageSize", 0 ) );
}

void KCMLocale::initMeasureSystem()
{
    m_ui->m_comboMeasureSystem->blockSignals( true );

    m_ui->m_labelMeasureSystem->setText( ki18n( "Measurement system:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the measurement system to use.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMeasureSystem->setToolTip( helpText );
    m_ui->m_comboMeasureSystem->setWhatsThis( helpText );

    m_ui->m_comboMeasureSystem->clear();

    m_ui->m_comboMeasureSystem->addItem( ki18n("Metric System").toString( m_kcmLocale ), (int) KLocale::Metric );
    m_ui->m_comboMeasureSystem->addItem( ki18n("Imperial System").toString( m_kcmLocale ), (int) KLocale::Imperial );

    changeMeasureSystem( m_kcmSettings.readEntry( "MeasureSystem", 0 ) );

    m_ui->m_comboMeasureSystem->blockSignals( false );
}

void KCMLocale::defaultMeasureSystem()
{
    changeMeasureSystem( m_defaultSettings.readEntry( "MeasureSystem", 0 ) );
}

void KCMLocale::changedMeasureSystemIndex( int index )
{
    changeMeasureSystem( m_ui->m_comboMeasureSystem->itemData( index ).toInt() );
}

void KCMLocale::changeMeasureSystem( int newValue )
{
    setComboItem( "MeasureSystem", newValue,
                  m_ui->m_comboMeasureSystem, m_ui->m_buttonResetMeasureSystem );
    m_kcmLocale->setMeasureSystem( (KLocale::MeasureSystem) m_kcmSettings.readEntry( "MeasureSystem", 0 ) );
}

void KCMLocale::initBinaryUnitDialect()
{
    m_ui->m_comboBinaryUnitDialect->blockSignals( true );

    m_ui->m_labelBinaryUnitDialect->setText( ki18n( "Byte size units:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This changes the units used by most KDE programs to display "
                              "numbers counted in bytes. Traditionally \"kilobytes\" meant units "
                              "of 1024, instead of the metric 1000, for most (but not all) byte "
                              "sizes."
                              "<ul>"
                              "<li>To reduce confusion you can use the recently standardized IEC "
                                  "units which are always in multiples of 1024.</li>"
                              "<li>You can also select metric, which is always in units of 1000.</li>"
                              "<li>Selecting JEDEC restores the older-style units used in KDE 3.5 "
                                  "and some other operating systems.</li>"
                              "</p>" ).toString( m_kcmLocale );
    m_ui->m_comboBinaryUnitDialect->setToolTip( helpText );
    m_ui->m_comboBinaryUnitDialect->setWhatsThis( helpText );

    m_ui->m_comboBinaryUnitDialect->addItem( ki18nc("Unit of binary measurement", "IEC Units (KiB, MiB, etc)").toString( m_kcmLocale ),
                                             QVariant( KLocale::IECBinaryDialect ) );
    m_ui->m_comboBinaryUnitDialect->addItem( ki18nc("Unit of binary measurement", "JEDEC Units (KB, MB, etc)").toString( m_kcmLocale ),
                                             QVariant( KLocale::JEDECBinaryDialect ) );
    m_ui->m_comboBinaryUnitDialect->addItem( ki18nc("Unit of binary measurement", "Metric Units (kB, MB, etc)").toString( m_kcmLocale ),
                                             QVariant( KLocale::MetricBinaryDialect ) );

    changeBinaryUnitDialect( m_kcmSettings.readEntry( "BinaryUnitDialect", 0 ) );

    m_ui->m_comboBinaryUnitDialect->blockSignals( false );
}

void KCMLocale::defaultBinaryUnitDialect()
{
    changeBinaryUnitDialect( m_defaultSettings.readEntry( "BinaryUnitDialect", 0 ) );
}

void KCMLocale::changedBinaryUnitDialectIndex( int index )
{
    changeBinaryUnitDialect( m_ui->m_comboBinaryUnitDialect->itemData( index ).toInt() );
}

void KCMLocale::changeBinaryUnitDialect( int newValue )
{
    setComboItem( "BinaryUnitDialect", newValue,
                  m_ui->m_comboBinaryUnitDialect, m_ui->m_buttonResetBinaryUnitDialect );
    m_kcmLocale->setBinaryUnitDialect( (KLocale::BinaryUnitDialect)
                                       m_kcmSettings.readEntry( "BinaryUnitDialect", 0 ) );
    m_ui->m_labelBinaryUnitSample->setText( ki18nc("Example test for binary unit dialect",
                                                   "Example: 2000 bytes equals %1")
                                                  .subs( m_kcmLocale->formatByteSize( 2000, 2 ) )
                                                  .toString( m_kcmLocale ) );
}

QString KCMLocale::posixToUserTime( const QString &posixFormat ) const
{
    return posixToUser( posixFormat, m_timeFormatMap );
}

QString KCMLocale::posixToUserDate( const QString &posixFormat ) const
{
    return posixToUser( posixFormat, m_dateFormatMap );
}

QString KCMLocale::posixToUser( const QString &posixFormat, const QMap<QString, QString> &map ) const
{
    QString result;

    bool escaped = false;
    for ( int pos = 0; pos < posixFormat.length(); ++pos ) {
        QChar c = posixFormat.at( pos );
        if ( escaped ) {
            QString key = c;
            if ( c == 'E' ) {
                key += posixFormat.at( ++pos );
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

QString KCMLocale::userToPosixTime( const QString &userFormat ) const
{
    return userToPosix( userFormat, m_timeFormatMap );
}

QString KCMLocale::userToPosixDate( const QString &userFormat ) const
{
    return userToPosix( userFormat, m_dateFormatMap );
}

QString KCMLocale::userToPosix( const QString &userFormat, const QMap<QString, QString> &map ) const
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

#include "kcmlocale.moc"
