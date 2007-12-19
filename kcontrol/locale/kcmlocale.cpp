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

#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include <KDebug>
#include <KDialog>
#include <KIconLoader>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include "kcmlocale.h"
#include "kcmlocale.moc"
#include "toplevel.h"
#include "klanguagebutton.h"
#include "countryselectordialog.h"
#include "kcontrollocale.h"

KLocaleConfig::KLocaleConfig(KControlLocale *locale, QWidget *parent)
  : QWidget (parent),
    m_locale(locale)
{
    setupUi( this );

    m_crLabel->setObjectName(I18N_NOOP("Country or Region:"));
    m_languagesLabel->setObjectName(I18N_NOOP("Languages:"));
    languageRemove->setObjectName(I18N_NOOP("Remove"));
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
    m_languageList = configGroup.readEntry("Language").split(":");
}

void KLocaleConfig::slotAddLanguage(const QString & code)
{
  int pos = m_languages->currentRow();
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
  int pos = m_languages->currentRow();

  if (pos != -1)
  {
    m_languageList.removeAt(pos);

    m_locale->setLanguage( m_languageList );

    emit localeChanged();
    if ( pos == 0 )
      emit languageChanged();
  }
}

void KLocaleConfig::slotLanguageUp()
{
  int pos = m_languages->currentRow();

  QStringList::Iterator it1 = m_languageList.begin() + pos - 1;
  QStringList::Iterator it2 = m_languageList.begin() + pos;

  if ( it1 != m_languageList.end() && it2 != m_languageList.end()  )
  {
    QString str = *it1;
    *it1 = *it2;
    *it2 = str;

    m_locale->setLanguage( m_languageList );

    emit localeChanged();
    if ( pos == 1 ) // at the lang before the top
      emit languageChanged();
  }
}

void KLocaleConfig::slotLanguageDown()
{
  int pos = m_languages->currentRow();

  QStringList::Iterator it1 = m_languageList.begin() + pos;
  QStringList::Iterator it2 = m_languageList.begin() + pos + 1;

  if ( it1 != m_languageList.end() && it2 != m_languageList.end()  )
    {
      QString str = *it1;
      *it1 = *it2;
      *it2 = str;

      m_locale->setLanguage( m_languageList );

      emit localeChanged();
      if ( pos == 0 ) // at the top
        emit languageChanged();
    }
}

void KLocaleConfig::changeCountry()
{
  CountrySelectorDialog *csd = new CountrySelectorDialog(this);
  if (csd->editCountry(m_locale)) emit localeChanged();
  delete csd;
}

void KLocaleConfig::readLocale(const QString &path, QString &name,
                               const QString &sub) const
{
  // read the name
  QString filepath = QString::fromLatin1("%1%2/entry.desktop")
    .arg(sub)
    .arg(path);

  KConfig entry(KStandardDirs::locate("locale", filepath));
  entry.setLocale(m_locale->language());
  KConfigGroup entryGroup = entry.group("KCM Locale");
  name = entryGroup.readEntry("Name");
}

void KLocaleConfig::save()
{
  KSharedConfigPtr config = KGlobal::config();

  KConfigGroup configGroup = config->group("Locale");

  configGroup.writeEntry("Country", m_locale->country(), KConfig::Persistent|KConfig::Global);
  configGroup.writeEntry("Language",
                         m_languageList.join(":"), KConfig::Persistent|KConfig::Global);

  config->sync();
}

void KLocaleConfig::slotCheckButtons()
{
  languageRemove->setEnabled( m_languages->currentRow() != -1 && m_languages->count() > 1 );
  m_upButton->setEnabled( m_languages->currentRow() > 0 );
  m_downButton->setEnabled( m_languages->currentRow() != -1 &&
                            m_languages->currentRow() < (signed)(m_languages->count() - 1) );
}

void KLocaleConfig::slotLocaleChanged()
{
  // update language widget
  m_languages->clear();
  for ( QStringList::Iterator it = m_languageList.begin();
        it != m_languageList.end();
        ++it )
  {
    QString name;
    readLocale(*it, name, QString());

    m_languages->addItem(name);
  }
  slotCheckButtons();

  QString country = m_locale->countryCodeToName(m_locale->country());
  if (country.isEmpty()) country = i18nc("@item:intext Country", "Default");
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

QStringList KLocaleConfig::languageList() const
{
  QString fileName = KStandardDirs::locate("locale",
                            QString::fromLatin1("l10n/%1/entry.desktop")
                            .arg(m_locale->country()));

  KConfig _entry( fileName, KConfig::SimpleConfig );
  KConfigGroup entry(&_entry, "KCM Locale");

  return entry.readEntry("Languages", QStringList());
}

void KLocaleConfig::changedCountry(const QString & code)
{
  m_locale->setCountry(code);

  // change to the preferred languages in that country, installed only
  QStringList languages = languageList();
  QStringList newLanguageList;
  for ( QStringList::Iterator it = languages.begin();
        it != languages.end();
        ++it )
  {
    QString name;
    readLocale(*it, name, QString());

    if (!name.isEmpty())
      newLanguageList += *it;
  }
  m_locale->setLanguage( newLanguageList );

  emit localeChanged();
  emit languageChanged();
}
