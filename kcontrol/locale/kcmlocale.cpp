/*
 * locale.cpp
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

#include "kcmlocale.h"

#include <QPushButton>

#include <KDebug>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include "kcmlocale.moc"
#include "toplevel.h"
#include "klanguagebutton.h"
#include "countryselectordialog.h"
#include "kcontrollocale.h"

static QStringList languageList(const QString& country)
{
  QString fileName = KStandardDirs::locate("locale",
                            QString::fromLatin1("l10n/%1/entry.desktop")
                            .arg(country));

  KConfig _entry( fileName, KConfig::SimpleConfig );
  KConfigGroup entry(&_entry, "KCM Locale");

  return entry.readEntry("Languages", QStringList());
}

static QString readLocale(const QString &language, const QString &path, const QString &sub=QString())
{
  // read the name
  QString filepath = QString::fromLatin1("%1%2/entry.desktop")
    .arg(sub)
    .arg(path);

  KConfig entry(KStandardDirs::locate("locale", filepath));
  entry.setLocale(language);
  KConfigGroup entryGroup = entry.group("KCM Locale");
  return entryGroup.readEntry("Name");
}



KLocaleConfig::KLocaleConfig(KControlLocale *locale, QWidget *parent)
  : QWidget (parent),
    m_locale(locale)
{
    setupUi( this );

    m_crLabel->setObjectName( QLatin1String(I18N_NOOP("Country or region:" )));
    m_languagesLabel->setObjectName( QLatin1String(I18N_NOOP("Languages:" )));
    languageRemove->setObjectName( QLatin1String(I18N_NOOP("Remove" )));
    m_upButton->setObjectName(QString());
    m_downButton->setObjectName(QString());
    m_selectedCountryLabel->setObjectName(QString());

    connect(languageRemove, SIGNAL(clicked()),
          SLOT(slotRemoveLanguage()));
    connect(m_upButton, SIGNAL(clicked()),
            SLOT(slotLanguageUp()));
    connect(m_downButton, SIGNAL(clicked()),
            SLOT(slotLanguageDown()));

    connect(m_selectedCountryLabel, SIGNAL(linkActivated(const QString &)), SLOT(changeCountry()));
    connect(languageAdd, SIGNAL(activated(const QString &)), SLOT(slotAddLanguage(const QString &)));
    connect(m_languages, SIGNAL(itemSelectionChanged()), SLOT(slotCheckButtons()));

    m_upButton->setIcon(KIcon("arrow-up"));
    m_downButton->setIcon(KIcon("arrow-down"));

    languageAdd->loadAllLanguages();
    
    KConfigGroup configGroup = KGlobal::config()->group("Locale");
    m_languageList = configGroup.readEntry("Language").split(':',QString::SkipEmptyParts);
}

int KLocaleConfig::selectedRow() const
{
    QList<QListWidgetItem *>selectedItems = m_languages->selectedItems();

    if ( selectedItems.isEmpty() ) {
        return -1;
    }
    return m_languages->row( selectedItems.at(0) );
}

void KLocaleConfig::slotAddLanguage(const QString & code)
{
  int pos = selectedRow();
  if ( pos < 0 )
    pos = 0;

  // If it's already in list, just move it (delete the old, then insert a new)
  int oldPos = m_languageList.indexOf( code );
  if ( oldPos != -1 )
    m_languageList.removeAll( code );

  if ( oldPos != -1 && oldPos < pos )
    --pos;

  m_languageList.insert( pos, code );

  m_locale->setLanguage( m_languageList );

  emit localeChanged();
  if ( pos == 0 )
    emit languageChanged();
}

void KLocaleConfig::slotRemoveLanguage()
{
  int pos = selectedRow();

  if (pos != -1)
  {
    m_languageList.removeAt(pos);

    m_locale->setLanguage( m_languageList );

    emit localeChanged();
    if ( pos == 0 )
      emit languageChanged();
  }
}

void KLocaleConfig::languageMove(Direction direcition)
{
  int pos = selectedRow();

  QStringList::Iterator it1 = m_languageList.begin() + pos - 1*(direcition==Up);
  QStringList::Iterator it2 = m_languageList.begin() + pos + 1*(direcition==Down);

  if ( it1 != m_languageList.end() && it2 != m_languageList.end()  )
  {
    QString str = *it1;
    *it1 = *it2;
    *it2 = str;

    m_locale->setLanguage( m_languageList );

    emit localeChanged();
    if ( pos == 1*(direcition==Up) ) //Up: at the top, Down: at the lang before the top
      emit languageChanged();
  }
}

void KLocaleConfig::slotLanguageUp()
{
  languageMove(Up);
}

void KLocaleConfig::slotLanguageDown()
{
  languageMove(Down);
}

void KLocaleConfig::changeCountry()
{
  CountrySelectorDialog *csd = new CountrySelectorDialog(this);
  if (csd->editCountry(m_locale)) emit localeChanged();
  delete csd;
}


void KLocaleConfig::save()
{
  KSharedConfigPtr config = KGlobal::config();

  KConfigGroup configGroup(config, "Locale");

  configGroup.writeEntry("Country", m_locale->country(), KConfig::Persistent|KConfig::Global);
  configGroup.writeEntry("Language",
                         m_languageList.join(":"), KConfig::Persistent|KConfig::Global);

  config->sync();
}

void KLocaleConfig::slotCheckButtons()
{
  languageRemove->setEnabled( selectedRow() != -1 && m_languages->count() > 1 );
  m_upButton->setEnabled( selectedRow() > 0 );
  m_downButton->setEnabled( selectedRow() != -1 &&
                            selectedRow() < (signed)(m_languages->count() - 1) );
}

void KLocaleConfig::slotLocaleChanged()
{
  // update language widget
  m_languages->clear();
  foreach (const QString& langCode, m_languageList)
    m_languages->addItem(readLocale(m_locale->language(),langCode));
  slotCheckButtons();

  QString country = m_locale->countryCodeToName(m_locale->country());
  if (country.isEmpty()) country = i18nc("@item:intext Country", "Not set (Generic English)");
  m_selectedCountryLabel->setText(i18nc("@info %1 is country name", "<html>%1 (<a href=\"changeCountry\">change...</a>)</html>", country));
}

void KLocaleConfig::slotTranslate()
{
  languageAdd->setText( ki18n("Add Language").toString(m_locale) );

  m_selectedCountryLabel->setToolTip( ki18n
        ( "This is where you live. KDE will use the defaults for "
          "this country or region.").toString(m_locale) );
  languageAdd->setToolTip( ki18n
        ( "This will add a language to the list. If the language is already "
          "in the list, the old one will be moved instead." ).toString(m_locale) );

  languageRemove->setToolTip( ki18n
        ( "This will remove the highlighted language from the list." ).toString(m_locale) );

  m_languages->setToolTip( ki18n
        ( "KDE programs will be displayed in the first available language in "
          "this list.\nIf none of the languages are available, US English "
          "will be used.").toString(m_locale) );

  QString str;

  str = ki18n
    ( "Here you can choose your country or region. The settings "
      "for languages, numbers etc. will automatically switch to the "
      "corresponding values." ).toString(m_locale);
  m_crLabel->setWhatsThis( str );
  m_selectedCountryLabel->setWhatsThis( str );

  str = ki18n
    ( "<p>Here you can choose the languages that will be used by KDE. If the "
      "first language in the list is not available, the second will be used, "
      "etc. If only US English is available, no translations "
      "have been installed. You can get translation packages for many "
      "languages from the place you got KDE from.</p><p>"
      "Note that some applications may not be translated to your languages; "
      "in this case, they will automatically fall back to US English.</p>" ).toString(m_locale);
  m_languagesLabel->setWhatsThis( str );
  m_languages->setWhatsThis( str );
  languageAdd->setWhatsThis( str );
  languageRemove->setWhatsThis( str );
}


void KLocaleConfig::changedCountry(const QString & code)
{
  m_locale->setCountry(code);

  // change to the preferred languages in that country, installed only
  QStringList languages = languageList(m_locale->country());
  QStringList newLanguageList;
  foreach (const QString& langCode, languages)
  {
    if (!readLocale(m_locale->language(),langCode).isEmpty())
      newLanguageList += langCode;
  }
  m_locale->setLanguage( newLanguageList );

  emit localeChanged();
  emit languageChanged();
}
