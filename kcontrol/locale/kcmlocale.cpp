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

//TODO create <> include in kdelibs!!!
#include "kcurrencycode.h"

#include "ui_kcmlocalewidget.h"

K_PLUGIN_FACTORY( KCMLocaleFactory, registerPlugin<KCMLocale>(); )
K_EXPORT_PLUGIN( KCMLocaleFactory( "kcmlocale" ) )

KCMLocale::KCMLocale( QWidget *parent, const QVariantList &args )
         : KCModule( KCMLocaleFactory::componentData(), parent, args ),
           m_config( KSharedConfig::openConfig( QString(), KConfig::FullConfig ) ),
           m_settings( m_config, "Locale" ),
           m_userConfig( KSharedConfig::openConfig( QString(), KConfig::NoCascade ) ),
           m_userSettings( m_userConfig, "Locale" ),
           m_ui( new Ui::KCMLocaleWidget )
{
    KAboutData *about = new KAboutData( "kcmlocale", 0, ki18n( "Localization options for KDE applications" ),
                                        0, KLocalizedString(), KAboutData::License_GPL,
                                        ki18n( "Copyright 2010 John Layt" ) );

    about->addAuthor( ki18n( "John Layt" ), ki18n( "Maintainer" ), "john@layt.net" );

    setAboutData( about );

    m_ui->setupUi( this );

    // Country tab
    connect( m_ui->m_comboCountry,       SIGNAL( activated( int ) ),
             this,                       SLOT( changeCountry( int ) ) );
    connect( m_ui->m_buttonResetCountry, SIGNAL( clicked() ),
             this,                       SLOT( defaultCountry() ) );

    connect( m_ui->m_comboCountryDivision,       SIGNAL( activated( int ) ),
             this,                               SLOT( changeCountryDivision( int ) ) );
    connect( m_ui->m_buttonResetCountryDivision, SIGNAL( clicked() ),
             this,                               SLOT( defaultCountryDivision() ) );

    // Langauges tab

    m_ui->m_selectorTranslations->setButtonsEnabled();

    connect( m_ui->m_selectorTranslations, SIGNAL( added( QListWidgetItem * ) ),
             this,                         SLOT( changeTranslationsAdded( QListWidgetItem * ) ) );
    connect( m_ui->m_selectorTranslations, SIGNAL( removed( QListWidgetItem * ) ),
             this,                         SLOT( changeTranslationsRemoved( QListWidgetItem * ) ) );
    connect( m_ui->m_selectorTranslations, SIGNAL( movedDown( QListWidgetItem * ) ),
             this,                         SLOT( changeTranslationsMoved( QListWidgetItem * ) ) );
    connect( m_ui->m_selectorTranslations, SIGNAL( movedUp( QListWidgetItem * ) ),
             this,                         SLOT( changeTranslationsMoved( QListWidgetItem * ) ) );

    // Numbers tab

    connect( m_ui->m_comboThousandsSeparator,       SIGNAL( editTextChanged( const QString & ) ),
             this,                                  SLOT( changeThousandsSeparator( const QString & ) ) );
    connect( m_ui->m_buttonResetThousandsSeparator, SIGNAL( clicked() ),
             this,                                  SLOT( defaultThousandsSeparator() ) );

    connect( m_ui->m_comboDecimalSymbol,       SIGNAL( editTextChanged( const QString & ) ),
             this,                             SLOT( changeDecimalSymbol( const QString & ) ) );
    connect( m_ui->m_buttonResetDecimalSymbol, SIGNAL( clicked() ),
             this,                             SLOT( defaultDecimalSymbol() ) );

    connect( m_ui->m_intDecimalPlaces,         SIGNAL( valueChanged( int ) ),
             this,                             SLOT( changeDecimalPlaces( int ) ) );
    connect( m_ui->m_buttonResetDecimalPlaces, SIGNAL( clicked() ),
             this,                             SLOT( defaultDecimalPlaces() ) );

    connect( m_ui->m_comboPositiveSign,       SIGNAL( editTextChanged( const QString & ) ),
             this,                            SLOT( changePositiveSign( const QString & ) ) );
    connect( m_ui->m_buttonResetPositiveSign, SIGNAL( clicked() ),
             this,                            SLOT( defaultPositiveSign() ) );

    connect( m_ui->m_comboNegativeSign,       SIGNAL( editTextChanged( const QString & ) ),
             this,                            SLOT( changeNegativeSign( const QString & ) ) );
    connect( m_ui->m_buttonResetNegativeSign, SIGNAL( clicked() ),
             this,                            SLOT( defaultNegativeSign() ) );

    connect( m_ui->m_comboDigitSet,       SIGNAL( activated( int ) ),
             this,                        SLOT( changeDigitSet( int ) ) );
    connect( m_ui->m_buttonResetDigitSet, SIGNAL( clicked() ),
             this,                        SLOT( defaultDigitSet() ) );

    // Money tab

    connect( m_ui->m_comboCurrencyCode,       SIGNAL( activated( int ) ),
             this,                              SLOT( changeCurrencyCode( int ) ) );
    connect( m_ui->m_buttonResetCurrencyCode, SIGNAL( clicked() ),
             this,                              SLOT( defaultCurrencyCode() ) );

    connect( m_ui->m_comboCurrencySymbol,       SIGNAL( activated( int ) ),
             this,                              SLOT( changeCurrencySymbol( int ) ) );
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

    connect( m_ui->m_comboMonetaryPositiveFormat,       SIGNAL( activated( int ) ),
             this,                                      SLOT( changeMonetaryPositiveFormat( int ) ) );
    connect( m_ui->m_buttonResetMonetaryPositiveFormat, SIGNAL( clicked() ),
             this,                                      SLOT( defaultMonetaryPositiveFormat() ) );

    connect( m_ui->m_comboMonetaryNegativeFormat,       SIGNAL( activated( int ) ),
             this,                                      SLOT( changeMonetaryNegativeFormat( int ) ) );
    connect( m_ui->m_buttonResetMonetaryNegativeFormat, SIGNAL( clicked() ),
             this,                                      SLOT( defaultMonetaryNegativeFormat() ) );

    connect( m_ui->m_comboMonetaryDigitSet,       SIGNAL( activated( int ) ),
             this,                                SLOT( changeMonetaryDigitSet( int ) ) );
    connect( m_ui->m_buttonResetMonetaryDigitSet, SIGNAL( clicked() ),
             this,                                SLOT( defaultMonetaryDigitSet() ) );

    // Calendar tab

    connect( m_ui->m_comboCalendarSystem,       SIGNAL( activated( int ) ),
             this,                              SLOT( changeCalendarSystem( int ) ) );
    connect( m_ui->m_buttonResetCalendarSystem, SIGNAL( clicked() ),
             this,                              SLOT( defaultCalendarSystem() ) );

    connect( m_ui->m_checkCalendarGregorianUseCommonEra,       SIGNAL( clicked() ),
             this,                                             SLOT( changeUseCommonEra() ) );
    connect( m_ui->m_buttonResetCalendarGregorianUseCommonEra, SIGNAL( clicked() ),
             this,                                             SLOT( defaultUseCommonEra() ) );

    connect( m_ui->m_intShortYearWindowStartYear, SIGNAL( valueChanged( int ) ),
             this,                                SLOT( changeShortYearWindow( int ) ) );
    connect( m_ui->m_buttonResetShortYearWindow,  SIGNAL( clicked() ),
             this,                                SLOT( defaultUseCommonEra() ) );

    connect( m_ui->m_comboWeekStartDay,       SIGNAL( activated( int ) ),
             this,                            SLOT( changeWeekStartDay( int ) ) );
    connect( m_ui->m_buttonResetWeekStartDay, SIGNAL( clicked() ),
             this,                            SLOT( defaultWeekStartDay() ) );

    connect( m_ui->m_comboWorkingWeekStartDay,       SIGNAL( activated( int ) ),
             this,                                   SLOT( changeWorkingWeekStartDay( int ) ) );
    connect( m_ui->m_buttonResetWorkingWeekStartDay, SIGNAL( clicked() ),
             this,                                   SLOT( defaultWorkingWeekStartDay() ) );

    connect( m_ui->m_comboWorkingWeekEndDay,       SIGNAL( activated( int ) ),
             this,                                 SLOT( changeWorkingWeekEndDay( int ) ) );
    connect( m_ui->m_buttonResetWorkingWeekEndDay, SIGNAL( clicked() ),
             this,                                 SLOT( defaultWorkingWeekEndDay() ) );

    connect( m_ui->m_comboWeekDayOfPray,       SIGNAL( activated( int ) ),
             this,                             SLOT( changeWeekDayOfPray( int ) ) );
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

    connect( m_ui->m_checkMonthNamePossessive,       SIGNAL( clicked() ),
             this,                                   SLOT( changeMonthNamePossessive() ) );
    connect( m_ui->m_buttonResetMonthNamePossessive, SIGNAL( clicked() ),
             this,                                   SLOT( defaultMonthNamePossessive() ) );

    connect( m_ui->m_comboDateTimeDigitSet,       SIGNAL( activated( int ) ),
             this,                                SLOT( changeDateTimeDigitSet( int ) ) );
    connect( m_ui->m_buttonResetDateTimeDigitSet, SIGNAL( clicked() ),
             this,                                SLOT( defaultDateTimeDigitSet() ) );

    // Other tab

    connect( m_ui->m_comboPageSize,       SIGNAL( activated( int ) ),
             this,                        SLOT( changePageSize( int ) ) );
    connect( m_ui->m_buttonResetPageSize, SIGNAL( clicked() ),
             this,                        SLOT( defaultPageSize() ) );

    connect( m_ui->m_comboMeasureSystem,       SIGNAL( activated( int ) ),
             this,                             SLOT( changeMeasureSystem( int ) ) );
    connect( m_ui->m_buttonResetMeasureSystem, SIGNAL( clicked() ),
             this,                             SLOT( defaultPageSize() ) );

    connect( m_ui->m_comboBinaryUnitDialect,       SIGNAL( activated( int ) ),
             this,                                 SLOT( changeBinaryUnitDialect( int ) ) );
    connect( m_ui->m_buttonResetBinaryUnitDialect, SIGNAL( clicked() ),
             this,                                 SLOT( defaultPageSize() ) );

    load();
    initAllWidgets();
}

KCMLocale::~KCMLocale()
{
    delete m_ui;
    delete m_kcmLocale;
}

void KCMLocale::load()
{
    m_kcmLocale = KGlobal::locale();

    m_countryConfig = KSharedConfig::openConfig( KStandardDirs::locate( "locale",
                                                 QString::fromLatin1("l10n/%1/entry.desktop")
                                                 .arg( m_kcmLocale->country() ) ) );
    m_countrySettings= KConfigGroup( m_countryConfig, "KCM Locale" );

    m_defaultConfig = KSharedConfig::openConfig( KStandardDirs::locate( "locale",
                                                 QString::fromLatin1("l10n/C/entry.desktop") ) );
    m_defaultSettings= KConfigGroup( m_defaultConfig, "KCM Locale" );
}

void KCMLocale::save()
{
    KMessageBox::information(this, ki18n("Changed language settings apply only to "
                                         "newly started applications.\nTo change the "
                                         "language of all programs, you will have to "
                                         "logout first.").toString(m_kcmLocale),
                             ki18n("Applying Language Settings").toString(m_kcmLocale),
                             QLatin1String("LanguageChangesApplyOnlyToNewlyStartedPrograms"));

    bool langChanged;
    // rebuild the date base if language was changed
    if (langChanged)
    {
        KBuildSycocaProgressDialog::rebuildKSycoca(this);
    }
}

void KCMLocale::defaults()
{
    load();
    initAllWidgets();
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

void KCMLocale::setItemEnabled( const QString itemKey, QWidget *itemWidget, KPushButton *itemReset )
{
    if ( m_settings.isEntryImmutable( itemKey ) ) {
            itemWidget->setEnabled( false );
            itemReset->setEnabled( false );
    } else {
        itemWidget->setEnabled( true );
        if ( m_userSettings.hasKey( itemKey ) ) {
            itemReset->setEnabled( true );
        } else {
            itemReset->setEnabled( false );
        }
    }
}

void KCMLocale::initAllWidgets()
{
    //Initialise the widgets with the default values
    //Needs re-running whenever the language changes!

    //The kcm uses the language currently chosen in the kcm for translations.
    //This is for usability so when using for the first time the user can
    //understand.  However it means we have to translate everything here rather
    //than in the .ui file.

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

    //Numeric tab
    initNumericThousandsSeparator();
    initNumericDecimalSymbol();
    initNumericDecimalPlaces();
    initNumericPositiveSign();
    initNumericNegativeSign();
    initNumericDigitSet();

    //Monetary tab
    initCurrencyCode();
    initCurrencySymbol();
    initMonetaryThousandsSeparator();
    initMonetaryDecimalSymbol();
    initMonetaryDecimalPlaces();
    initMonetaryPositiveFormat();
    initMonetaryNegativeFormat();
    initMonetaryDigitSet();

    //Calendar tab
    initCalendarSystem();
    initUseCommonEra();
    initShortYearWindow();
    initWeekStartDay();
    initWorkingWeekStartDay();
    initWorkingWeekEndDay();
    initWeekDayOfPray();

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
}

void KCMLocale::initResetButtons()
{
    KGuiItem defaultItem( QString(), "document-revert", ki18n( "Reset item to its default value" ).toString( m_kcmLocale ) );

    //Country tab
    m_ui->m_buttonResetCountry->setGuiItem( defaultItem );
    m_ui->m_buttonResetCountryDivision->setGuiItem( defaultItem );

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

void KCMLocale::loadCombo( const QString &key, const QString &loadValue, KComboBox *combo, KPushButton *resetButton )
{
    combo->setCurrentIndex( combo->findData( loadValue ) );
    setItemEnabled( key, combo, resetButton );
}

void KCMLocale::loadCombo( const QString &key, int loadValue, KComboBox *combo, KPushButton *resetButton )
{
    combo->setCurrentIndex( (int) combo->findData( loadValue ) );
    setItemEnabled( key, combo, resetButton );
}

void KCMLocale::loadEditCombo( const QString &key, const QString &loadValue, KComboBox *combo, KPushButton *resetButton )
{
    combo->setEditText( loadValue );
    setItemEnabled( key, combo, resetButton );
}

void KCMLocale::saveValue( const QString &key, const QString &saveValue )
{
    QString defaultValue = m_defaultSettings.readEntry( key, QString() );
    QString countryValue = m_countrySettings.readEntry( key, defaultValue );
    m_settings.deleteEntry( key, KConfig::Persistent | KConfig::Global );
    if ( saveValue != countryValue ) {
        m_settings.writeEntry( key, saveValue, KConfig::Persistent | KConfig::Global );
    }
}

void KCMLocale::saveValue( const QString &key, int saveValue )
{
    int defaultValue = m_defaultSettings.readEntry( key, 0 );
    int countryValue = m_countrySettings.readEntry( key, defaultValue );
    m_settings.deleteEntry( key, KConfig::Persistent | KConfig::Global );
    if ( saveValue != countryValue ) {
        m_settings.writeEntry( key, saveValue, KConfig::Persistent | KConfig::Global );
    }
}

void KCMLocale::defaultCombo( const QString &key, const QString &type, KComboBox *combo )
{
    Q_UNUSED( type );
    QString defaultValue = m_defaultSettings.readEntry( key, QString() );
    QString countryValue = m_countrySettings.readEntry( key, defaultValue );
    combo->setCurrentIndex( combo->findData( countryValue ) );
}

void KCMLocale::defaultCombo( const QString &key, int type, KComboBox *combo )
{
    Q_UNUSED( type );
    int defaultValue = m_defaultSettings.readEntry( key, 0 );
    int countryValue = m_countrySettings.readEntry( key, defaultValue );
    combo->setCurrentIndex( combo->findData( countryValue ) );
}

void KCMLocale::defaultEditCombo( const QString &key, const QString &type, KComboBox *combo )
{
    Q_UNUSED( type );
    QString defaultValue = m_defaultSettings.readEntry( key, QString() );
    QString countryValue = m_countrySettings.readEntry( key, defaultValue );
    combo->setEditText( countryValue );
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

    separatorCombo->addItem( ki18nc( "No seperator symbol" , "None" ).toString( m_kcmLocale ), QString() );
    separatorCombo->addItem( QString(','), QString(',') );
    separatorCombo->addItem( QString('.'), QString('.') );
    separatorCombo->addItem( ki18nc( "Space seperator symbol", "Single Space" ).toString( m_kcmLocale ), ' ' );
}

void KCMLocale::initPositiveCombo( KComboBox *positiveCombo )
{
    positiveCombo->clear();

    positiveCombo->addItem( ki18nc( "No positive symbol", "None" ).toString( m_kcmLocale ), QString() );
    positiveCombo->addItem( QString('+'), QString('+') );
}

void KCMLocale::initNegativeCombo( KComboBox *negativeCombo )
{
    negativeCombo->clear();

    negativeCombo->addItem( ki18nc("No negative symbol", "None" ).toString( m_kcmLocale ), QString() );
    negativeCombo->addItem( QString('-'), QString('-') );
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
    m_ui->m_textMoneyNegativeSample->setText( m_kcmLocale->formatMoney( 123456.78 ) );

    KDateTime dateTime = KDateTime::currentLocalDateTime();
    m_ui->m_textDateSample->setText( m_kcmLocale->formatDate( dateTime.date(), KLocale::LongDate ) );
    m_ui->m_textShortDateSample->setText( m_kcmLocale->formatDate( dateTime.date(), KLocale::ShortDate ) );
    m_ui->m_textTimeSample->setText( m_kcmLocale->formatTime( dateTime.time(), true ) );
}

void KCMLocale::initCountry()
{
    m_ui->m_labelCountry->setText( ki18n( "Country:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This is the country where you live.  The KDE Workspace will use "
                              "the settings for this country or region.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCountry->setToolTip( helpText );
    m_ui->m_comboCountry->setWhatsThis( helpText );

    m_ui->m_comboCountry->clear();

    QStringList countryCodes = m_kcmLocale->allCountriesList();
    countryCodes.removeDuplicates();
    QMap<QString, QString> countryNames;

    foreach ( const QString &countryCode, countryCodes )
    {
        countryNames.insert( m_kcmLocale->countryCodeToName( countryCode ), countryCode );
    }

    QString systemCountryName;
    QString systemCountry = ki18nc( "%1 is the system country name", "System Country (%1)" ).subs( systemCountryName ).toString( m_kcmLocale );
    m_ui->m_comboCountry->addItem( systemCountry , "system" );

    QString defaultLocale = ki18n( "No Country (Default Settings)" ).toString( m_kcmLocale );
    QMapIterator<QString, QString> it( countryNames );
    while ( it.hasNext() )
    {
        it.next();
        KIcon flag( KStandardDirs::locate( "locale", QString::fromLatin1( "l10n/%1/flag.png" ).arg( it.value() ) ) );
        m_ui->m_comboCountry->addItem( flag, it.key(), it.value() );
    }

    loadCountry();
}

void KCMLocale::loadCountry()
{
    loadCombo( "Country", m_kcmLocale->country(),
               m_ui->m_comboCountry, m_ui->m_buttonResetCountry );
}

void KCMLocale::saveCountry()
{
    saveValue( "Country", m_kcmLocale->country() );
}

void KCMLocale::defaultCountry()
{
    defaultCombo( "Country", QString(), m_ui->m_comboCountry );
    saveCountry();
    setItemEnabled( "Country", m_ui->m_comboCountry, m_ui->m_buttonResetCountry );
}

void KCMLocale::changeCountry( int activated )
{
    m_kcmLocale->setCountry( m_ui->m_comboCountry->itemData( activated ).toString(), m_config.data() );
    saveCountry();
    setItemEnabled( "Country", m_ui->m_comboCountry, m_ui->m_buttonResetCountry );
}

void KCMLocale::initCountryDivision()
{
    m_ui->m_labelCountryDivision->setText( ki18n( "Subdivision:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "" ).toString( m_kcmLocale );
    m_ui->m_comboCountryDivision->setToolTip( helpText );
    m_ui->m_comboCountryDivision->setWhatsThis( helpText );
    loadCountry();
}

void KCMLocale::loadCountryDivision()
{
    loadCombo( "CountryDivisionCode", m_kcmLocale->countryDivisionCode(),
               m_ui->m_comboCountryDivision, m_ui->m_buttonResetCountryDivision );
}

void KCMLocale::saveCountryDivision()
{
    saveValue( "CountryDivisionCode", m_kcmLocale->countryDivisionCode() );
}

void KCMLocale::defaultCountryDivision()
{
    defaultCombo( "CountryDivisionCode", QString(), m_ui->m_comboCountryDivision );
    saveCountryDivision();
    setItemEnabled( "CountryDivisionCode", m_ui->m_comboCountryDivision, m_ui->m_buttonResetCountryDivision );
}

void KCMLocale::changeCountryDivision( int activated )
{
    m_kcmLocale->setCountryDivisionCode( m_ui->m_comboCountryDivision->itemData( activated ).toString() );
    saveCountryDivision();
    setItemEnabled( "CountryDivisionCode", m_ui->m_comboCountryDivision, m_ui->m_buttonResetCountryDivision );
}

void KCMLocale::initTranslations()
{
    m_ui->m_selectorTranslations->setAvailableLabel( ki18n( "Available Languages:" ).toString( m_kcmLocale ) );
    QString availableHelp = ki18n( "<p>This is the list of installed KDE Workspace language "
                                   "translations not currently being used.  To use a language "
                                   "translation move it to the 'Preferred Languages' list in "
                                   "the order of preference.  If no suitable languages are "
                                   "listed, then you may need to install more language packages "
                                   "using your usual installation method.</p>" ).toString( m_kcmLocale );
    m_ui->m_selectorTranslations->availableListWidget()->setToolTip( availableHelp );
    m_ui->m_selectorTranslations->availableListWidget()->setWhatsThis( availableHelp );

    m_ui->m_selectorTranslations->setSelectedLabel( ki18n( "Preferred Langauges:" ).toString( m_kcmLocale ) );
    QString selectedHelp = ki18n( "<p>This is the list of installed KDE Workspace language "
                                  "translations currently being used, listed in order of "
                                  "preference.  If a translation is not available for the "
                                  "first language in the list, the next language will be used.  "
                                  "If no other translations are available then US English will "
                                  "be used.</p>").toString( m_kcmLocale );
    m_ui->m_selectorTranslations->selectedListWidget()->setToolTip( selectedHelp );
    m_ui->m_selectorTranslations->selectedListWidget()->setWhatsThis( selectedHelp );

    QString enUS;
    QString defaultLang = ki18nc( "%1 = default language name", "%1 (Default)" ).subs( enUS ).toString( m_kcmLocale );

    loadTranslations();
}

void KCMLocale::loadTranslations()
{
    m_installedLanguages.clear();
    m_installedLanguages = m_kcmLocale->installedLanguages();
    m_languages.clear();
    m_languages = m_settings.readEntry( "Language" ).split( ':', QString::SkipEmptyParts );
    m_kcmLocale->setLanguage( m_languages );

    QStringList missingLanguages;
    foreach ( const QString &languageCode, m_languages ) {
        if ( !m_installedLanguages.contains( languageCode ) ) {
            missingLanguages.append( languageCode );
            m_languages.removeAll( languageCode );
        }
    }

    foreach ( const QString &languageCode, m_languages ) {
        QListWidgetItem *listItem = new QListWidgetItem( m_ui->m_selectorTranslations->selectedListWidget() );
        listItem->setText( m_kcmLocale->languageCodeToName( languageCode ) );
        listItem->setData( Qt::UserRole, languageCode );
    }

    foreach ( const QString &languageCode, m_installedLanguages ) {
        if ( !m_languages.contains( languageCode ) ) {
            QListWidgetItem *listItem = new QListWidgetItem( m_ui->m_selectorTranslations->availableListWidget() );
            listItem->setText( m_kcmLocale->languageCodeToName( languageCode ) );
            listItem->setData( Qt::UserRole, languageCode );
        }
    }

    foreach ( const QString &languageCode, missingLanguages ) {
        KMessageBox::information(this, ki18n("You have the language with code '%1' in your list "
                                             "of languages to use for translation but the "
                                             "localization files for it could not be found. The "
                                             "language has been removed from your configuration. "
                                             "If you want to add it again please install the "
                                             "localization files for it and add the language again.")
                                             .subs( languageCode ).toString( m_kcmLocale ) );
    }

}

void KCMLocale::saveTranslations()
{
    saveValue( "Language", m_languages.join(":") );
}

void KCMLocale::changeTranslations( QListWidgetItem *item )
{
    Q_UNUSED( item );

    m_languages.clear();

    foreach( QListWidgetItem *item, m_ui->m_selectorTranslations->selectedListWidget()->selectedItems() ) {
        m_languages.append( item->data( Qt::UserRole ).toString() );
    }

    saveTranslations();
    initAllWidgets();
}

void KCMLocale::initTranslationsInstall()
{
    m_ui->m_buttonTranslationInstall->setText( ki18n( "Install more languages" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Click here to install more langauges</p>" ).toString( m_kcmLocale );
    m_ui->m_buttonTranslationInstall->setToolTip( helpText );
    m_ui->m_buttonTranslationInstall->setWhatsThis( helpText );
}

void KCMLocale::initNumericThousandsSeparator()
{
    m_ui->m_labelThousandsSeparator->setText( ki18n( "Group separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the digit group separator used to display "
                              "numbers.</p><p>Note that the digit group separator used to display "
                              "monetary values has to be set separately (see the 'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboThousandsSeparator->setToolTip( helpText );
    m_ui->m_comboThousandsSeparator->setWhatsThis( helpText );

    loadNumericThousandsSeparator();
}

void KCMLocale::loadNumericThousandsSeparator()
{
    loadEditCombo( "ThousandsSeparator", m_kcmLocale->thousandsSeparator(),
                   m_ui->m_comboThousandsSeparator, m_ui->m_buttonResetThousandsSeparator );
}

void KCMLocale::saveNumericThousandsSeparator()
{
    saveValue( "ThousandsSeparator", m_kcmLocale->thousandsSeparator() );
}

void KCMLocale::defaultNumericThousandsSeparator()
{
    defaultEditCombo( "ThousandsSeparator", QString(), m_ui->m_comboThousandsSeparator );
    saveNumericThousandsSeparator();
    setItemEnabled( "ThousandsSeparator", m_ui->m_comboThousandsSeparator, m_ui->m_buttonResetThousandsSeparator );
}

void KCMLocale::changeNumericThousandsSeparator( const QString &newValue )
{
    m_kcmLocale->setThousandsSeparator( newValue );
    saveNumericThousandsSeparator();
    setItemEnabled( "ThousandsSeparator", m_ui->m_comboThousandsSeparator, m_ui->m_buttonResetThousandsSeparator );
}

void KCMLocale::initNumericDecimalSymbol()
{
    m_ui->m_labelDecimalSymbol->setText( ki18n( "Decimal separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the decimal separator used to display "
                              "numbers (i.e. a dot or a comma in most countries).</p><p>Note "
                              "that the decimal separator used to display monetary values has "
                              "to be set separately (see the 'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboDecimalSymbol->setToolTip( helpText );
    m_ui->m_comboDecimalSymbol->setWhatsThis( helpText );

    loadNumericDecimalSymbol();
}

void KCMLocale::loadNumericDecimalSymbol()
{
    loadEditCombo( "DecimalSymbol", m_kcmLocale->decimalSymbol(),
                   m_ui->m_comboDecimalSymbol, m_ui->m_buttonResetDecimalSymbol );
}

void KCMLocale::saveNumericDecimalSymbol()
{
    saveValue( "DecimalSymbol", m_kcmLocale->decimalSymbol() );
}

void KCMLocale::defaultNumericDecimalSymbol()
{
    defaultEditCombo( "DecimalSymbol", QString(), m_ui->m_comboDecimalSymbol );
    saveNumericDecimalSymbol();
    setItemEnabled( "DecimalSymbol", m_ui->m_comboDecimalSymbol, m_ui->m_buttonResetDecimalSymbol );
}

void KCMLocale::changeNumericDecimalSymbol( const QString &newValue )
{
    m_kcmLocale->setDecimalSymbol( newValue );
    saveNumericDecimalSymbol();
    setItemEnabled( "DecimalSymbol", m_ui->m_comboDecimalSymbol, m_ui->m_buttonResetDecimalSymbol );
}

void KCMLocale::initNumericDecimalPlaces()
{
    m_ui->m_labelDecimalPlaces->setText( ki18n( "Decimal places:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the number of decimal places displayed for "
                              "numeric values, i.e. the number of digits <em>after</em> the "
                              "decimal separator.</p><p>Note that the decimal places used "
                              "to display monetary values has to be set separately (see the "
                              "'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_intDecimalPlaces->setToolTip( helpText );
    m_ui->m_intDecimalPlaces->setWhatsThis( helpText );
}

void KCMLocale::initNumericPositiveSign()
{
    m_ui->m_labelPositiveFormat->setText( ki18n( "Positive sign:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can specify text used to prefix positive numbers. "
                              "Most locales leave this blank.</p><p>Note that the positive sign "
                              "used to display monetary values has to be set separately (see the "
                              "'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboPositiveSign->setToolTip( helpText );
    m_ui->m_comboPositiveSign->setWhatsThis( helpText );

    loadNumericPositiveSign();
}

void KCMLocale::loadNumericPositiveSign()
{
    loadEditCombo( "PositiveSign", m_kcmLocale->positiveSign(),
                   m_ui->m_comboPositiveSign, m_ui->m_buttonResetPositiveSign );
}

void KCMLocale::saveNumericPositiveSign()
{
    saveValue( "PositiveSign", m_kcmLocale->positiveSign() );
}

void KCMLocale::defaultNumericPositiveSign()
{
    defaultEditCombo( "PositiveSign", QString(), m_ui->m_comboPositiveSign );
    saveNumericPositiveSign();
    setItemEnabled( "PositiveSign", m_ui->m_comboPositiveSign, m_ui->m_buttonResetPositiveSign );
}

void KCMLocale::changeNumericPositiveSign( const QString &newValue )
{
    m_kcmLocale->setPositiveSign( newValue );
    saveNumericPositiveSign();
    setItemEnabled( "PositiveSign", m_ui->m_comboPositiveSign, m_ui->m_buttonResetPositiveSign );
}

void KCMLocale::initNumericNegativeSign()
{
    m_ui->m_labelNegativeFormat->setText( ki18n( "Negative sign:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can specify text used to prefix negative numbers. "
                              "This should not be empty, so you can distinguish positive and "
                              "negative numbers. It is normally set to minus (-).</p><p>Note "
                              "that the negative sign used to display monetary values has to "
                              "be set separately (see the 'Money' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboNegativeSign->setToolTip( helpText );
    m_ui->m_comboNegativeSign->setWhatsThis( helpText );

    loadNumericNegativeSign();
}

void KCMLocale::loadNumericNegativeSign()
{
    loadEditCombo( "NegativeSign", m_kcmLocale->negativeSign(),
                   m_ui->m_comboNegativeSign, m_ui->m_buttonResetNegativeSign );
}

void KCMLocale::saveNumericNegativeSign()
{
    saveValue( "NegativeSign", m_kcmLocale->negativeSign() );
}

void KCMLocale::defaultNumericNegativeSign()
{
    defaultEditCombo( "NegativeSign", QString(), m_ui->m_comboNegativeSign );
    saveNumericNegativeSign();
    setItemEnabled( "NegativeSign", m_ui->m_comboNegativeSign, m_ui->m_buttonResetNegativeSign );
}

void KCMLocale::changeNumericNegativeSign( const QString &newValue )
{
    m_kcmLocale->setNegativeSign( newValue );
    saveNumericNegativeSign();
    setItemEnabled( "NegativeSign", m_ui->m_comboNegativeSign, m_ui->m_buttonResetNegativeSign );
}

void KCMLocale::initNumericDigitSet()
{
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

    loadNumericDigitSet();
}

void KCMLocale::loadNumericDigitSet()
{
    loadCombo( "DigitSet", m_kcmLocale->digitSet(),
               m_ui->m_comboDigitSet, m_ui->m_buttonResetDigitSet );
}

void KCMLocale::saveNumericDigitSet()
{
    saveValue( "DigitSet", m_kcmLocale->digitSet() );
}

void KCMLocale::defaultNumericDigitSet()
{
    defaultCombo( "DigitSet", QString(), m_ui->m_comboDigitSet );
    saveNumericDigitSet();
    setItemEnabled( "DigitSet", m_ui->m_comboDigitSet, m_ui->m_buttonResetDigitSet );
}

void KCMLocale::changeNumericDigitSet( int activated )
{
    m_kcmLocale->setDigitSet( (KLocale::DigitSet) m_ui->m_comboDigitSet->itemData( activated ).toInt() );
    saveNumericDigitSet();
    setItemEnabled( "DigitSet", m_ui->m_comboDigitSet, m_ui->m_buttonResetDigitSet );
}

void KCMLocale::initCurrencyCode()
{
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
        .subs( m_kcmLocale->currency()->currencyCodeToName( currencyCode ) ) .subs( currencyCode ).toString( m_kcmLocale );
        m_ui->m_comboCurrencyCode->addItem( text, QVariant( currencyCode ) );
    }
    //Next put all currencies available in alphabetical name order
    m_ui->m_comboCurrencyCode->insertSeparator(m_ui->m_comboCurrencyCode->count());
    currencyCodeList = m_kcmLocale->currency()->allCurrencyCodesList();
    QStringList currencyNameList;
    foreach ( const QString &currencyCode, currencyCodeList ) {
        currencyNameList.append( ki18nc( "@item currency name and currency code", "%1 (%2)").subs(
        m_kcmLocale->currency()->currencyCodeToName( currencyCode ) ).subs( currencyCode ).toString( m_kcmLocale ) );
    }
    currencyNameList.sort();
    foreach ( const QString &name, currencyNameList ) {
        m_ui->m_comboCurrencyCode->addItem( name, QVariant( name.mid( name.length()-4, 3 ) ) );
    }

    loadCurrencyCode();
}

void KCMLocale::loadCurrencyCode()
{
    loadCombo( "CurrencyCode", m_kcmLocale->currencyCode(),
               m_ui->m_comboCurrencyCode, m_ui->m_buttonResetCurrencyCode );
}

void KCMLocale::saveCurrencyCode()
{
    saveValue( "CurrencyCode", m_kcmLocale->currencyCode() );
}

void KCMLocale::defaultCurrencyCode()
{
    defaultCombo( "CurrencyCode", QString(), m_ui->m_comboCurrencyCode );
    saveCurrencyCode();
    setItemEnabled( "CurrencyCode", m_ui->m_comboCurrencyCode, m_ui->m_buttonResetCurrencyCode );
}

void KCMLocale::changeCurrencyCode( int activated )
{
    m_kcmLocale->setCurrencyCode( m_ui->m_comboCurrencyCode->itemData( activated ).toString() );
    saveCurrencyCode();
    setItemEnabled( "CurrencyCode", m_ui->m_comboCurrencyCode, m_ui->m_buttonResetCurrencyCode );
}

void KCMLocale::initCurrencySymbol()
{
    m_ui->m_labelCurrencySymbol->setText( ki18n( "Currency symbol:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can choose the symbol to be used when displaying "
                              "monetary values, e.g. $, US$ or USD.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCurrencySymbol->setToolTip( helpText );
    m_ui->m_comboCurrencySymbol->setWhatsThis( helpText );

    //Create the list of Currency Symbols for the selected Currency Code
    m_ui->m_comboCurrencySymbol->clear();
    QStringList currencySymbolList = m_kcmLocale->currency()->symbolList();
    foreach ( const QString &currencySymbol, currencySymbolList ) {
        m_ui->m_comboCurrencySymbol->addItem( currencySymbol, QVariant( currencySymbol ) );
    }

    loadCurrencySymbol();
}

void KCMLocale::loadCurrencySymbol()
{
    loadCombo( "CurrencySymbol", m_kcmLocale->currencySymbol(),
               m_ui->m_comboCurrencySymbol, m_ui->m_buttonResetCurrencySymbol );
}

void KCMLocale::saveCurrencySymbol()
{
    saveValue( "CurrencySymbol", m_kcmLocale->currencySymbol() );
}

void KCMLocale::defaultCurrencySymbol()
{
    defaultCombo( "CurrencySymbol", QString(), m_ui->m_comboCurrencySymbol );
    saveCurrencySymbol();
    setItemEnabled( "CurrencySymbol", m_ui->m_comboCurrencySymbol, m_ui->m_buttonResetCurrencySymbol );
}

void KCMLocale::changeCurrencySymbol( int activated )
{
    m_kcmLocale->setCurrencySymbol( m_ui->m_comboCurrencySymbol->itemData( activated ).toString() );
    saveCurrencySymbol();
    setItemEnabled( "CurrencySymbol", m_ui->m_comboCurrencySymbol, m_ui->m_buttonResetCurrencySymbol );
}

void KCMLocale::initMonetaryThousandsSeparator()
{
    m_ui->m_labelMonetaryThousandsSeparator->setText( ki18n( "Group separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the group separator used to display monetary "
                              "values.</p><p>Note that the thousands separator used to display "
                              "other numbers has to be defined separately (see the 'Numbers' tab)."
                              "</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryThousandsSeparator->setToolTip( helpText );
    m_ui->m_comboMonetaryThousandsSeparator->setWhatsThis( helpText );

    loadMonetaryThousandsSeparator();
}

void KCMLocale::loadMonetaryThousandsSeparator()
{
    loadEditCombo( "MonetaryThousandsSeparator", m_kcmLocale->monetaryThousandsSeparator(),
                   m_ui->m_comboMonetaryThousandsSeparator, m_ui->m_buttonResetMonetaryThousandsSeparator );
}

void KCMLocale::saveMonetaryThousandsSeparator()
{
    saveValue( "MonetaryThousandsSeparator", m_kcmLocale->monetaryThousandsSeparator() );
}

void KCMLocale::defaultMonetaryThousandsSeparator()
{
    defaultEditCombo( "MonetaryThousandsSeparator", QString(), m_ui->m_comboMonetaryThousandsSeparator );
    saveMonetaryThousandsSeparator();
    setItemEnabled( "MonetaryThousandsSeparator", m_ui->m_comboMonetaryThousandsSeparator, m_ui->m_buttonResetMonetaryThousandsSeparator );
}

void KCMLocale::changeMonetaryThousandsSeparator( const QString &newValue )
{
    m_kcmLocale->setMonetaryThousandsSeparator( newValue );
    saveMonetaryThousandsSeparator();
    setItemEnabled( "MonetaryThousandsSeparator", m_ui->m_comboMonetaryThousandsSeparator, m_ui->m_buttonResetMonetaryThousandsSeparator );
}

void KCMLocale::initMonetaryDecimalSymbol()
{
    m_ui->m_labelMonetaryDecimalSymbol->setText( ki18n( "Decimal separator:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the decimal separator used to display "
                              "monetary values.</p><p>Note that the thousands separator used to "
                              "display other numbers has to be defined separately (see the "
                              "'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryDecimalSymbol->setToolTip( helpText );
    m_ui->m_comboMonetaryDecimalSymbol->setWhatsThis( helpText );
}

void KCMLocale::loadMonetaryDecimalSymbol()
{
    loadEditCombo( "MonetaryDecimalSymbol", m_kcmLocale->monetaryDecimalSymbol(),
                   m_ui->m_comboMonetaryDecimalSymbol, m_ui->m_buttonResetMonetaryDecimalSymbol );
}

void KCMLocale::saveMonetaryDecimalSymbol()
{
    saveValue( "MonetaryDecimalSymbol", m_kcmLocale->monetaryDecimalSymbol() );
}

void KCMLocale::defaultMonetaryDecimalSymbol()
{
    defaultEditCombo( "MonetaryDecimalSymbol", QString(), m_ui->m_comboMonetaryDecimalSymbol );
    saveMonetaryDecimalSymbol();
    setItemEnabled( "MonetaryDecimalSymbol", m_ui->m_comboMonetaryDecimalSymbol, m_ui->m_buttonResetMonetaryDecimalSymbol );
}

void KCMLocale::changeMonetaryDecimalSymbol( const QString &newValue )
{
    m_kcmLocale->setMonetaryDecimalSymbol( newValue );
    saveMonetaryDecimalSymbol();
    setItemEnabled( "MonetaryDecimalSymbol", m_ui->m_comboMonetaryDecimalSymbol, m_ui->m_buttonResetMonetaryDecimalSymbol );
}

void KCMLocale::initMonetaryDecimalPlaces()
{
    m_ui->m_labelMonetaryDecimalPlaces->setText( ki18n( "Decimal places:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the number of decimal places displayed for "
                              "monetary values, i.e. the number of digits <em>after</em> the "
                              "decimal separator.</p><p>Note that the decimal places used to "
                              "display other numbers has to be defined separately (see the "
                              "'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_intMonetaryDecimalPlaces->setToolTip( helpText );
    m_ui->m_intMonetaryDecimalPlaces->setWhatsThis( helpText );
}

void KCMLocale::loadMonetaryDecimalPlaces()
{
}

void KCMLocale::saveMonetaryDecimalPlaces()
{
}

void KCMLocale::defaultMonetaryDecimalPlaces()
{
}

void KCMLocale::changeMonetaryDecimalPlaces( int newValue )
{
}

void KCMLocale::initMonetaryPositiveFormat()
{
    m_ui->m_labelMonetaryPositiveFormat->setText( ki18n( "Positive format:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the format of positive monetary values.</p>"
                              "<p>Note that the positive sign used to display other numbers has "
                              "to be defined separately (see the 'Numbers' tab).</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMonetaryPositiveFormat->setToolTip( helpText );
    m_ui->m_comboMonetaryPositiveFormat->setWhatsThis( helpText );

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
}

void KCMLocale::loadMonetaryPositiveFormat()
{
}

void KCMLocale::saveMonetaryPositiveFormat()
{
}

void KCMLocale::defaultMonetaryPositiveFormat()
{
}

void KCMLocale::changeMonetaryPositiveFormat( int activated )
{
}

void KCMLocale::initMonetaryNegativeFormat()
{
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
}

void KCMLocale::loadMonetaryNegativeFormat()
{
}

void KCMLocale::saveMonetaryNegativeFormat()
{
}

void KCMLocale::defaultMonetaryNegativeFormat()
{
}

void KCMLocale::changeMonetaryNegativeFormat( int activated )
{
}

void KCMLocale::initMonetaryDigitSet()
{
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

    loadMonetaryDigitSet();
}

void KCMLocale::loadMonetaryDigitSet()
{
    loadCombo( "MonetaryDigitSet", m_kcmLocale->monetaryDigitSet(),
               m_ui->m_comboMonetaryDigitSet, m_ui->m_buttonResetMonetaryDigitSet );
}

void KCMLocale::saveMonetaryDigitSet()
{
    saveValue( "MonetaryDigitSet", m_kcmLocale->monetaryDigitSet() );
}

void KCMLocale::defaultMonetaryDigitSet()
{
    defaultCombo( "MonetaryDigitSet", QString(), m_ui->m_comboMonetaryDigitSet );
    saveMonetaryDigitSet();
    setItemEnabled( "MonetaryDigitSet", m_ui->m_comboMonetaryDigitSet, m_ui->m_buttonResetMonetaryDigitSet );
}

void KCMLocale::changeMonetaryDigitSet( int activated )
{
    m_kcmLocale->setMonetaryDigitSet( (KLocale::DigitSet) m_ui->m_comboMonetaryDigitSet->itemData( activated ).toInt() );
    saveMonetaryDigitSet();
    setItemEnabled( "MonetaryDigitSet", m_ui->m_comboMonetaryDigitSet, m_ui->m_buttonResetMonetaryDigitSet );
}

void KCMLocale::initCalendarSystem()
{
    m_ui->m_labelCalendarSystem->setText( ki18n( "Calendar system:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the Calendar System to use to display dates.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboCalendarSystem->setToolTip( helpText );
    m_ui->m_comboCalendarSystem->setWhatsThis( helpText );

    m_ui->m_comboCalendarSystem->clear();

    QStringList calendarSystems = KCalendarSystem::calendarSystems();

    foreach ( const QString &calendarType, calendarSystems )
    {
        m_ui->m_comboCalendarSystem->addItem( KCalendarSystem::calendarLabel( calendarTypeToCalendarSystem( calendarType ), m_kcmLocale ), QVariant( calendarType ) );
    }

    loadCalendarSystem();
}

void KCMLocale::loadCalendarSystem()
{
    loadCombo( "CalendarSystem", m_kcmLocale->calendarType(),
               m_ui->m_comboCalendarSystem, m_ui->m_buttonResetCalendarSystem );
}

void KCMLocale::saveCalendarSystem()
{
    saveValue( "CalendarSystem", m_kcmLocale->calendarType() );
}

void KCMLocale::defaultCalendarSystem()
{
    defaultCombo( "CalendarSystem", 0, m_ui->m_comboCalendarSystem );
    saveCalendarSystem();
    setItemEnabled( "CalendarSystem", m_ui->m_comboCalendarSystem, m_ui->m_buttonResetCalendarSystem );
}

void KCMLocale::changeCalendarSystem( int activated )
{
    m_kcmLocale->setCalendar( m_ui->m_comboCalendarSystem->itemData( activated ).toString() );
    saveCalendarSystem();
    setItemEnabled( "CalendarSystem", m_ui->m_comboCalendarSystem, m_ui->m_buttonResetCalendarSystem );
}

void KCMLocale::initUseCommonEra()
{
    m_ui->m_checkCalendarGregorianUseCommonEra->setText( ki18n( "Use Common Era" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines if the Common Era (CE/BCE) should be used "
                              "instead of the Christian Era (AD/BC).</p>" ).toString( m_kcmLocale );
    m_ui->m_checkCalendarGregorianUseCommonEra->setToolTip( helpText );
    m_ui->m_checkCalendarGregorianUseCommonEra->setWhatsThis( helpText );

    loadUseCommonEra();
}

void KCMLocale::loadUseCommonEra()
{
    KConfigGroup calendarGroup( m_config, QString( "KCalendarSystem %1" ).arg( m_kcmLocale->calendarType() ) );
    m_ui->m_checkCalendarGregorianUseCommonEra->setChecked( calendarGroup.readEntry( "UseCommonEra", false ) );
    setItemEnabled( "UseCommonEra", m_ui->m_checkCalendarGregorianUseCommonEra, m_ui->m_buttonResetCalendarGregorianUseCommonEra );
}

void KCMLocale::initShortYearWindow()
{
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

    loadShortYearWindow();
}

void KCMLocale::loadShortYearWindow()
{
    m_ui->m_intShortYearWindowStartYear->setValue( m_kcmLocale->calendar()->shortYearWindowStartYear() );
    m_ui->m_spinShortYearWindowEndYear->setValue( m_kcmLocale->calendar()->shortYearWindowStartYear() + 99 );
}

void KCMLocale::initWeekStartDay()
{
    m_ui->m_labelWeekStartDay->setText( ki18n( "First day of week:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day will be considered as the first "
                              "one of the week.</p> " ).toString( m_kcmLocale );
    m_ui->m_comboWeekStartDay->setToolTip( helpText );
    m_ui->m_comboWeekStartDay->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWeekStartDay );

    loadWeekStartDay();
}

void KCMLocale::loadWeekStartDay()
{
    loadCombo( "WeekStartDay", m_kcmLocale->weekStartDay(),
               m_ui->m_comboWeekStartDay, m_ui->m_buttonResetWeekStartDay );
}

void KCMLocale::saveWeekStartDay()
{
    saveValue( "WeekStartDay", m_kcmLocale->weekStartDay() );
}

void KCMLocale::defaultWeekStartDay()
{
    defaultCombo( "WeekStartDay", 0, m_ui->m_comboWeekStartDay );
    savePageSize();
    setItemEnabled( "WeekStartDay", m_ui->m_comboWeekStartDay, m_ui->m_buttonResetWeekStartDay );
}

void KCMLocale::changeWeekStartDay( int activated )
{
    m_kcmLocale->setWeekStartDay( m_ui->m_comboWeekStartDay->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "WeekStartDay", m_ui->m_comboWeekStartDay, m_ui->m_buttonResetWeekStartDay );
}

void KCMLocale::initWorkingWeekStartDay()
{
    m_ui->m_labelWorkingWeekStartDay->setText( ki18n( "First working day of week:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day will be considered as the first "
                              "working day of the week.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboWorkingWeekStartDay->setToolTip( helpText );
    m_ui->m_comboWorkingWeekStartDay->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWorkingWeekStartDay );

    loadWorkingWeekStartDay();
}

void KCMLocale::loadWorkingWeekStartDay()
{
    loadCombo( "WorkingWeekStartDay", m_kcmLocale->workingWeekStartDay(),
               m_ui->m_comboWorkingWeekStartDay, m_ui->m_buttonResetWorkingWeekStartDay );
}

void KCMLocale::saveWorkingWeekStartDay()
{
    saveValue( "WorkingWeekStartDay", m_kcmLocale->workingWeekStartDay() );
}

void KCMLocale::defaultWorkingWeekStartDay()
{
    defaultCombo( "WorkingWeekStartDay", 0, m_ui->m_comboWorkingWeekStartDay );
    savePageSize();
    setItemEnabled( "WorkingWeekStartDay", m_ui->m_comboWorkingWeekStartDay, m_ui->m_buttonResetWorkingWeekStartDay );
}

void KCMLocale::changeWorkingWeekStartDay( int activated )
{
    m_kcmLocale->setWorkingWeekStartDay( m_ui->m_comboWorkingWeekStartDay->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "WorkingWeekStartDay", m_ui->m_comboWorkingWeekStartDay, m_ui->m_buttonResetWorkingWeekStartDay );
}

void KCMLocale::initWorkingWeekEndDay()
{
    m_ui->m_labelWorkingWeekEndDay->setText( ki18n( "Last working day of week:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day will be considered as the last "
                              "working day of the week.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboWorkingWeekEndDay->setToolTip( helpText );
    m_ui->m_comboWorkingWeekEndDay->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWorkingWeekEndDay );

    loadWorkingWeekEndDay();
}

void KCMLocale::loadWorkingWeekEndDay()
{
    loadCombo( "WorkingWeekEndDay", m_kcmLocale->workingWeekEndDay(),
               m_ui->m_comboWorkingWeekEndDay, m_ui->m_buttonResetWorkingWeekEndDay );
}

void KCMLocale::saveWorkingWeekEndDay()
{
    saveValue( "WorkingWeekEndDay", m_kcmLocale->workingWeekEndDay() );
}

void KCMLocale::defaultWorkingWeekEndDay()
{
    defaultCombo( "WorkingWeekEndDay", 0, m_ui->m_comboWorkingWeekEndDay );
    savePageSize();
    setItemEnabled( "WorkingWeekEndDay", m_ui->m_comboWorkingWeekEndDay, m_ui->m_buttonResetWorkingWeekEndDay );
}

void KCMLocale::changeWorkingWeekEndDay( int activated )
{
    m_kcmLocale->setWorkingWeekEndDay( m_ui->m_comboWorkingWeekEndDay->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "WorkingWeekEndDay", m_ui->m_comboWorkingWeekEndDay, m_ui->m_buttonResetWorkingWeekEndDay );
}

void KCMLocale::initWeekDayOfPray()
{
    m_ui->m_labelWeekDayOfPray->setText( ki18n( "Week day for special religious observance:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines which day if any will be considered as "
                              "the day of the week for special religious observance.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboWeekDayOfPray->setToolTip( helpText );
    m_ui->m_comboWeekDayOfPray->setWhatsThis( helpText );

    initWeekDayCombo( m_ui->m_comboWeekDayOfPray );
    m_ui->m_comboWeekDayOfPray->insertItem( 0, ki18nc( "Day name list, option for no special day of religious observance", "None / None in particular" ).toString( m_kcmLocale ) );

    loadWeekDayOfPray();
}

void KCMLocale::loadWeekDayOfPray()
{
    loadCombo( "WeekDayOfPray", (int) m_kcmLocale->weekDayOfPray(),
               m_ui->m_comboWeekDayOfPray, m_ui->m_buttonResetWeekDayOfPray );
}

void KCMLocale::saveWeekDayOfPray()
{
    saveValue( "WeekDayOfPray", (int) m_kcmLocale->weekDayOfPray() );
}

void KCMLocale::defaultWeekDayOfPray()
{
    defaultCombo( "WeekDayOfPray", 0, m_ui->m_comboWeekDayOfPray );
    savePageSize();
    setItemEnabled( "WeekDayOfPray", m_ui->m_comboWeekDayOfPray, m_ui->m_buttonResetWeekDayOfPray );
}

void KCMLocale::changeWeekDayOfPray( int activated )
{
    m_kcmLocale->setWeekDayOfPray( m_ui->m_comboWeekDayOfPray->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "WeekDayOfPray", m_ui->m_comboWeekDayOfPray, m_ui->m_buttonResetWeekDayOfPray );
}

void KCMLocale::initTimeFormat()
{
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

    loadTimeFormat();
}

void KCMLocale::loadTimeFormat()
{
    m_ui->m_comboTimeFormat->setEditText( posixToUserTime( m_kcmLocale->timeFormat() ) );
    setItemEnabled( "TimeFormat", m_ui->m_comboTimeFormat, m_ui->m_buttonResetTimeFormat );
}

void KCMLocale::initAmSymbol()
{
    m_ui->m_labelAmSymbol->setText( ki18n( "AM symbol:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the text to be displayed for AM.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboAmSymbol->setToolTip( helpText );
    m_ui->m_comboAmSymbol->setWhatsThis( helpText );

    loadAmSymbol();
}

void KCMLocale::loadAmSymbol()
{
}

void KCMLocale::initPmSymbol()
{
    m_ui->m_labelPmSymbol->setText( ki18n( "PM symbol:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can set the text to be displayed for PM.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboPmSymbol->setToolTip( helpText );
    m_ui->m_comboPmSymbol->setWhatsThis( helpText );

    loadPmSymbol();
}

void KCMLocale::loadPmSymbol()
{
}

void KCMLocale::initDateFormat()
{
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

    loadDateFormat();
}

void KCMLocale::loadDateFormat()
{
    m_ui->m_comboDateFormat->setEditText( posixToUserDate( m_kcmLocale->dateFormat() ) );
    setItemEnabled( "DateFormat", m_ui->m_comboDateFormat, m_ui->m_buttonResetDateFormat );
}

void KCMLocale::initShortDateFormat()
{
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

    loadShortDateFormat();
}

void KCMLocale::loadShortDateFormat()
{
    m_ui->m_comboShortDateFormat->setEditText( posixToUserDate( m_kcmLocale->dateFormatShort() ) );
    setItemEnabled( "DateFormatShort", m_ui->m_comboShortDateFormat, m_ui->m_buttonResetShortDateFormat );
}

void KCMLocale::initMonthNamePossessive()
{
    m_ui->m_labelMonthNamePossessive->setText( ki18n( "Possessive month names:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>This option determines whether possessive form of month names "
                              "should be used in dates.</p>" ).toString( m_kcmLocale );
    m_ui->m_checkMonthNamePossessive->setToolTip( helpText );
    m_ui->m_checkMonthNamePossessive->setWhatsThis( helpText );

    loadMonthNamePossessive();
}

void KCMLocale::loadMonthNamePossessive()
{
    m_ui->m_checkMonthNamePossessive->setChecked( m_kcmLocale->dateMonthNamePossessive() );
    setItemEnabled( "DateMonthNamePossessive", m_ui->m_checkMonthNamePossessive, m_ui->m_buttonResetMonthNamePossessive );
}

void KCMLocale::initDateTimeDigitSet()
{
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

    loadDateTimeDigitSet();
}

void KCMLocale::loadDateTimeDigitSet()
{
    loadCombo( "DateTimeDigitSet", (int) m_kcmLocale->dateTimeDigitSet(),
               m_ui->m_comboDateTimeDigitSet, m_ui->m_buttonResetDateTimeDigitSet );
}

void KCMLocale::saveDateTimeDigitSet()
{
    saveValue( "DateTimeDigitSet", (int) m_kcmLocale->dateTimeDigitSet() );
}

void KCMLocale::defaultDateTimeDigitSet()
{
    defaultCombo( "DateTimeDigitSet", 0, m_ui->m_comboDateTimeDigitSet );
    savePageSize();
    setItemEnabled( "DateTimeDigitSet", m_ui->m_comboDateTimeDigitSet, m_ui->m_buttonResetDateTimeDigitSet );
}

void KCMLocale::changeDateTimeDigitSet( int activated )
{
    m_kcmLocale->setDateTimeDigitSet( (KLocale::DigitSet) m_ui->m_comboCalendarSystem->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "DateTimeDigitSet", m_ui->m_comboDateTimeDigitSet, m_ui->m_buttonResetDateTimeDigitSet );
}

void KCMLocale::initPageSize()
{
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

    loadPageSize();
}

void KCMLocale::loadPageSize()
{
    loadCombo( "PageSize", (int) m_kcmLocale->pageSize(),
               m_ui->m_comboPageSize, m_ui->m_buttonResetPageSize );
}

void KCMLocale::savePageSize()
{
    saveValue( "PageSize", (int) m_kcmLocale->pageSize() );
}

void KCMLocale::defaultPageSize()
{
    defaultCombo( "PageSize", 0, m_ui->m_comboPageSize );
    savePageSize();
    setItemEnabled( "PageSize", m_ui->m_comboPageSize, m_ui->m_buttonResetPageSize );
}

void KCMLocale::changePageSize( int activated )
{
    m_kcmLocale->setPageSize( m_ui->m_comboCalendarSystem->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "PageSize", m_ui->m_comboPageSize, m_ui->m_buttonResetPageSize );
}

void KCMLocale::initMeasureSystem()
{
    m_ui->m_labelMeasureSystem->setText( ki18n( "Measurement system:" ).toString( m_kcmLocale ) );
    QString helpText = ki18n( "<p>Here you can define the measurement system to use.</p>" ).toString( m_kcmLocale );
    m_ui->m_comboMeasureSystem->setToolTip( helpText );
    m_ui->m_comboMeasureSystem->setWhatsThis( helpText );

    m_ui->m_comboMeasureSystem->clear();
    m_ui->m_comboMeasureSystem->addItem( ki18n("Metric System").toString( m_kcmLocale ), (int) KLocale::Metric );
    m_ui->m_comboMeasureSystem->addItem( ki18n("Imperial System").toString( m_kcmLocale ), (int) KLocale::Imperial );

    loadMeasureSystem();
}

void KCMLocale::loadMeasureSystem()
{
    loadCombo( "MeasureSystem", (int) m_kcmLocale->measureSystem(),
               m_ui->m_comboMeasureSystem, m_ui->m_buttonResetMeasureSystem );
}

void KCMLocale::saveMeasureSystem()
{
    saveValue( "MeasureSystem", (int) m_kcmLocale->measureSystem() );
}

void KCMLocale::defaultMeasureSystem()
{
    defaultCombo( "MeasureSystem", 0, m_ui->m_comboMeasureSystem );
    saveMeasureSystem();
    setItemEnabled( "MeasureSystem", m_ui->m_comboMeasureSystem, m_ui->m_buttonResetMeasureSystem );
}

void KCMLocale::changeMeasureSystem( int activated )
{
    m_kcmLocale->setMeasureSystem( (KLocale::MeasureSystem) m_ui->m_comboMeasureSystem->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "MeasureSystem", m_ui->m_comboMeasureSystem, m_ui->m_buttonResetMeasureSystem );
}

void KCMLocale::initBinaryUnitDialect()
{
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

    loadBinaryUnitDialect();
}

void KCMLocale::loadBinaryUnitDialect()
{
    loadCombo( "BinaryUnitDialect", (int) m_kcmLocale->binaryUnitDialect(),
               m_ui->m_comboBinaryUnitDialect, m_ui->m_buttonResetBinaryUnitDialect );
}

void KCMLocale::saveBinaryUnitDialect()
{
    saveValue( "BinaryUnitDialect", (int) m_kcmLocale->binaryUnitDialect() );
}

void KCMLocale::defaultBinaryUnitDialect()
{
    defaultCombo( "BinaryUnitDialect", 0, m_ui->m_comboBinaryUnitDialect );
    saveMeasureSystem();
    setItemEnabled( "BinaryUnitDialect", m_ui->m_comboBinaryUnitDialect, m_ui->m_buttonResetBinaryUnitDialect );
}

void KCMLocale::changeBinaryUnitDialect( int activated )
{
    m_kcmLocale->setBinaryUnitDialect( (KLocale::BinaryUnitDialect) m_ui->m_comboBinaryUnitDialect->itemData( activated ).toInt() );
    savePageSize();
    setItemEnabled( "BinaryUnitDialect", m_ui->m_comboBinaryUnitDialect, m_ui->m_buttonResetBinaryUnitDialect );
}

KLocale::CalendarSystem KCMLocale::calendarTypeToCalendarSystem(const QString &calendarType) const
{
    if (calendarType == "coptic") {
        return KLocale::CopticCalendar;
    } else if (calendarType == "ethiopian") {
        return KLocale::EthiopianCalendar;
    } else if (calendarType == "gregorian") {
        return KLocale::QDateCalendar;
    } else if (calendarType == "gregorian-proleptic") {
        return KLocale::GregorianCalendar;
    } else if (calendarType == "hebrew") {
        return KLocale::HebrewCalendar;
    } else if (calendarType == "hijri") {
        return KLocale::IslamicCivilCalendar;
    } else if (calendarType == "indian-national") {
        return KLocale::IndianNationalCalendar;
    } else if (calendarType == "jalali") {
        return KLocale::JalaliCalendar;
    } else if (calendarType == "japanese") {
        return KLocale::JapaneseCalendar;
    } else if (calendarType == "julian") {
        return KLocale::JulianCalendar;
    } else if (calendarType == "minguo") {
        return KLocale::MinguoCalendar;
    } else if (calendarType == "thai") {
        return KLocale::ThaiCalendar;
    } else {
        return KLocale::QDateCalendar;
    }
}

QString KCMLocale::calendarSystemToCalendarType(KLocale::CalendarSystem calendarSystem) const
{
    switch (calendarSystem) {
    case KLocale::QDateCalendar:
        return "gregorian";
    case KLocale::CopticCalendar:
        return "coptic";
    case KLocale::EthiopianCalendar:
        return "ethiopian";
    case KLocale::GregorianCalendar:
        return "gregorian-proleptic";
    case KLocale::HebrewCalendar:
        return "hebrew";
    case KLocale::IslamicCivilCalendar:
        return "hijri";
    case KLocale::IndianNationalCalendar:
        return "indian-national";
    case KLocale::JalaliCalendar:
        return "jalali";
    case KLocale::JapaneseCalendar:
        return "japanese";
    case KLocale::JulianCalendar:
        return "julian";
    case KLocale::MinguoCalendar:
        return "minguo";
    case KLocale::ThaiCalendar:
        return "thai";
    default:
        return "gregorian";
    }
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
