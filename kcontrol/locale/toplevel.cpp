/*
  toplevel.cpp - A KControl Application

  Copyright (c) 1998 Matthias Hoelzer <hoelzer@physik.uni-wuerzburg.de>
  Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  */

#include "toplevel.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>

#include <kaboutdata.h>
#include <kbuildsycocaprogressdialog.h>
#include <ksharedconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <kservice.h>

#include "localenum.h"
#include "localemon.h"
#include "localetime.h"
#include "localeother.h"
#include "klocalesample.h"
#include "kcmlocale.h"
#include "toplevel.moc"
#include <kconfiggroup.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include "kcontrollocale.h"

K_PLUGIN_FACTORY(KLocaleFactory,
        KLocale::setMainCatalog("kcmlocale");
        registerPlugin<KLocaleApplication>();
    )
K_EXPORT_PLUGIN(KLocaleFactory("kcmlocale"))

KLocaleApplication::KLocaleApplication(QWidget *parent,
                                       const QVariantList &args)
  : KCModule( KLocaleFactory::componentData(), parent, args)
{
  KAboutData* aboutData = new KAboutData("kcmlocale", 0,
        ki18n("KCMLocale"),
        "3.0",
        ki18n("Regional settings"),
        KAboutData::License_GPL,
        ki18n("(C) 1998 Matthias Hoelzer, "
        "(C) 1999-2003 Hans Petter Bieker"),
        KLocalizedString(), 0, "bieker@kde.org");
  setAboutData( aboutData );

  m_nullConfig = KSharedConfig::openConfig(QString(), KConfig::NoGlobals );
  m_globalConfig = KSharedConfig::openConfig(QString(), KConfig::IncludeGlobals );

  m_locale = new KControlLocale(QLatin1String("kcmlocale"), m_nullConfig);

  QVBoxLayout *l = new QVBoxLayout(this);
  l->setMargin(0);

  m_tab = new QTabWidget(this);
  l->addWidget(m_tab);

  m_localemain = new KLocaleConfig(m_locale, this);

  m_tab->addTab( m_localemain, QString());
  m_localenum = new KLocaleConfigNumber(m_locale, this);
  m_tab->addTab( m_localenum, QString() );
  m_localemon = new KLocaleConfigMoney(m_locale, this);
  m_tab->addTab( m_localemon, QString() );
  m_localetime = new KLocaleConfigTime(m_locale, m_nullConfig, this);
  m_tab->addTab( m_localetime, QString() );
  m_localeother = new KLocaleConfigOther(m_locale, this);
  m_tab->addTab( m_localeother, QString() );

  // Examples
  m_gbox = new QGroupBox(this);
  l->addWidget(m_gbox);
  QVBoxLayout *laygroup = new QVBoxLayout(m_gbox);
  m_sample = new KLocaleSample(m_locale, m_gbox);
  laygroup->addWidget( m_sample );

  // getting signals from children
  connect(m_localemain, SIGNAL(localeChanged()),
          this, SIGNAL(localeChanged()));
  connect(m_localemain, SIGNAL(languageChanged()),
          this, SIGNAL(languageChanged()));

  // run the slots on the children
  connect(this, SIGNAL(localeChanged()),
          m_localemain, SLOT(slotLocaleChanged()));
  connect(this, SIGNAL(localeChanged()),
          m_localenum, SLOT(slotLocaleChanged()));
  connect(this, SIGNAL(localeChanged()),
          m_localemon, SLOT(slotLocaleChanged()));
  connect(this, SIGNAL(localeChanged()),
          m_localetime, SLOT(slotLocaleChanged()));
  connect(this, SIGNAL(localeChanged()),
          m_localeother, SLOT(slotLocaleChanged()));

  // keep the example up to date
  // NOTE: this will make the sample be updated 6 times the first time
  // because combo boxes++ emits the change signal not only when the user changes
  // it, but also when it's changed by the program.
  connect(m_localenum, SIGNAL(localeChanged()),
          m_sample, SLOT(slotLocaleChanged()));
  connect(m_localemon, SIGNAL(localeChanged()),
          m_sample, SLOT(slotLocaleChanged()));
  connect(m_localetime, SIGNAL(localeChanged()),
          m_sample, SLOT(slotLocaleChanged()));
  // No examples for this yet
  //connect(m_localeother, SIGNAL(slotLocaleChanged()),
  //m_sample, SLOT(slotLocaleChanged()));
  connect(this, SIGNAL(localeChanged()),
          m_sample, SLOT(slotLocaleChanged()));

  // make sure we always have translated interface
  connect(this, SIGNAL(languageChanged()),
          this, SLOT(slotTranslate()));
  connect(this, SIGNAL(languageChanged()),
          m_localemain, SLOT(slotTranslate()));
  connect(this, SIGNAL(languageChanged()),
          m_localenum, SLOT(slotTranslate()));
  connect(this, SIGNAL(languageChanged()),
          m_localemon, SLOT(slotTranslate()));
  connect(this, SIGNAL(languageChanged()),
          m_localetime, SLOT(slotTranslate()));
  connect(this, SIGNAL(languageChanged()),
          m_localeother, SLOT(slotTranslate()));

  // mark it as changed when we change it.
  connect(m_localemain, SIGNAL(localeChanged()),
          SLOT(slotChanged()));
  connect(m_localenum, SIGNAL(localeChanged()),
          SLOT(slotChanged()));
  connect(m_localemon, SIGNAL(localeChanged()),
          SLOT(slotChanged()));
  connect(m_localetime, SIGNAL(localeChanged()),
          SLOT(slotChanged()));
  connect(m_localeother, SIGNAL(localeChanged()),
          SLOT(slotChanged()));

}

KLocaleApplication::~KLocaleApplication()
{
  delete m_locale;
}

void KLocaleApplication::load()
{
  m_globalConfig->reparseConfiguration();
  *m_locale = KControlLocale(QLatin1String("kcmlocale"), m_globalConfig);

  emit localeChanged();
  emit languageChanged();
  emit changed(false);
}

void KLocaleApplication::save()
{
  KMessageBox::information(this, ki18n
                           ("Changed language settings apply only to "
                            "newly started applications.\nTo change the "
                            "language of all programs, you will have to "
                            "logout first.").toString(m_locale),
                           ki18n("Applying Language Settings").toString(m_locale),
                           QLatin1String("LanguageChangesApplyOnlyToNewlyStartedPrograms"));

  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group(config, "Locale");

  // ##### this doesn't make sense
  bool langChanged = group.readEntry("Language")
    != m_locale->language();

  m_localemain->save();
  m_localenum->save();
  m_localemon->save();
  m_localetime->save();
  m_localeother->save();

  // rebuild the date base if language was changed
  if (langChanged)
  {
    KBuildSycocaProgressDialog::rebuildKSycoca(this);
  }

  emit changed(false);
}

void KLocaleApplication::defaults()
{
  *m_locale = KControlLocale(QLatin1String("kcmlocale"), m_nullConfig);

  kDebug() << "defaults: " << m_locale->languageList();

  emit localeChanged();
  emit languageChanged();
}

QString KLocaleApplication::quickHelp() const
{
  return ki18n("<h1>Country/Region & Language</h1>\n"
          "<p>From here you can configure language, numeric, and time \n"
          "settings for your particular region. In most cases it will be \n"
          "sufficient to choose the country you live in. For instance KDE \n"
          "will automatically choose \"German\" as language if you choose \n"
          "\"Germany\" from the list. It will also change the time format \n"
          "to use 24 hours and and use comma as decimal separator.</p>\n").toString(m_locale);
}

void KLocaleApplication::slotTranslate()
{
  // The untranslated string for QLabel are stored in
  // the name() so we use that when retranslating
  const QList<QWidget*> &list = findChildren<QWidget*>();
  foreach ( QObject* wc, list )
  {
    // unnamed labels will cause errors and should not be
    // retranslated. E.g. the example box should not be
    // retranslated from here.
    if (wc->objectName().isEmpty())
      continue;
    if (wc->objectName() == "unnamed")
      continue;

    if (::qstrcmp(wc->metaObject()->className(), "QLabel") == 0)
      ((QLabel *)wc)->setText( ki18n( wc->objectName().toLatin1() ).toString( m_locale ) );
    else if (::qstrcmp(wc->metaObject()->className(), "QGroupBox") == 0 ||
             ::qstrcmp(wc->metaObject()->className(), "QVGroupBox") == 0)
      ((QGroupBox *)wc)->setTitle( ki18n( wc->objectName().toLatin1() ).toString( m_locale ) );
    else if (::qstrcmp(wc->metaObject()->className(), "QPushButton") == 0 ||
             ::qstrcmp(wc->metaObject()->className(), "KMenuButton") == 0)
      ((QPushButton *)wc)->setText( ki18n( wc->objectName().toLatin1() ).toString( m_locale ) );
    else if (::qstrcmp(wc->metaObject()->className(), "QCheckBox") == 0)
      ((QCheckBox *)wc)->setText( ki18n( wc->objectName().toLatin1() ).toString( m_locale ) );
  }

  // Here we have the pointer
  m_gbox->setWindowTitle(ki18n("Examples").toString(m_locale));
  m_tab->setTabText(m_tab->indexOf(m_localemain), ki18n("&Locale").toString(m_locale));
  m_tab->setTabText(m_tab->indexOf(m_localenum), ki18n("&Numbers").toString(m_locale));
  m_tab->setTabText(m_tab->indexOf(m_localemon), ki18n("&Money").toString(m_locale));
  m_tab->setTabText(m_tab->indexOf(m_localetime), ki18n("&Time && Dates").toString(m_locale));
  m_tab->setTabText(m_tab->indexOf(m_localeother), ki18n("&Other").toString(m_locale));
  // FIXME: All widgets are done now. However, there are
  // still some problems. Popup menus from the QLabels are
  // not retranslated.
}

void KLocaleApplication::slotChanged()
{
  emit changed(true);
}

