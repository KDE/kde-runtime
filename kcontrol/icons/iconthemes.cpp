/**
 *  Copyright (c) 2000 Antonio Larrosa <larrosa@kde.org>
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

#include "iconthemes.h"

#include <config-runtime.h>

#include <stdlib.h>
#include <unistd.h>

#include <QFileInfo>
#include <QLabel>
//Added by qt3to4:
#include <QPixmap>
#include <QVBoxLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QPainter>

#include <kdebug.h>
#include <kapplication.h>
#include <kbuildsycocaprogressdialog.h>
#include <klocale.h>
#include <kiconcache.h>
#include <kpushbutton.h>
#include <ksvgrenderer.h>
#include <kstandarddirs.h>
#include <kservice.h>
#include <kconfig.h>
#include <knewstuff3/downloaddialog.h>

#undef Unsorted

#include <kurlrequesterdialog.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <k3icon_p.h>  // this private header is only installed for us!
#include <kprogressdialog.h>
#include <kio/job.h>
#include <kio/deletejob.h>
#include <kio/netaccess.h>
#include <ktar.h>
#include <kglobalsettings.h>

static const int ThemeNameRole = Qt::UserRole + 1;

IconThemesConfig::IconThemesConfig(const KComponentData &inst, QWidget *parent)
  : KCModule(inst, parent)
{
  QVBoxLayout *topLayout = new QVBoxLayout(this);

  QFrame *m_preview=new QFrame(this);
  m_preview->setMinimumHeight(80);

  QHBoxLayout *lh2=new QHBoxLayout( m_preview );
  lh2->setSpacing(0);
  m_previewExec=new QLabel(m_preview);
  m_previewExec->setPixmap(DesktopIcon("system-run"));
  m_previewFolder=new QLabel(m_preview);
  m_previewFolder->setPixmap(DesktopIcon("folder"));
  m_previewDocument=new QLabel(m_preview);
  m_previewDocument->setPixmap(DesktopIcon("document"));

  lh2->addStretch(10);
  lh2->addWidget(m_previewExec);
  lh2->addStretch(1);
  lh2->addWidget(m_previewFolder);
  lh2->addStretch(1);
  lh2->addWidget(m_previewDocument);
  lh2->addStretch(10);


  m_iconThemes=new QTreeWidget(this/*"IconThemeList"*/);
  QStringList columns;
  columns.append(i18n("Name"));
  columns.append(i18n("Description"));
  m_iconThemes->setHeaderLabels(columns);
  m_iconThemes->setAllColumnsShowFocus( true );
  m_iconThemes->setRootIsDecorated(false);
  m_iconThemes->setSortingEnabled(true);
  connect(m_iconThemes,SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		SLOT(themeSelected(QTreeWidgetItem *)));

  KPushButton *installButton=new KPushButton( KIcon("document-import"), i18n("Install Theme File..."), this);
  installButton->setObjectName("InstallNewTheme");
  installButton->setToolTip(i18n("Install a theme archive file you already have locally"));
  installButton->setWhatsThis(i18n("If you already have a theme archive locally, this button will unpack it and make it available for KDE applications"));
  connect(installButton,SIGNAL(clicked()),SLOT(installNewTheme()));

  KPushButton *newButton=new KPushButton( KIcon("get-hot-new-stuff"), i18n("Get New Themes..."), this);
  newButton->setObjectName("GetNewTheme");
  newButton->setToolTip(i18n("Get new themes from the Internet"));
  newButton->setWhatsThis(i18n("You need to be connected to the Internet to use this action. A dialog will display a list of themes from the http://www.kde.org website. Clicking the Install button associated with a theme will install this theme locally."));
  connect(newButton,SIGNAL(clicked()),SLOT(getNewTheme()));

  m_removeButton=new KPushButton( KIcon("edit-delete"), i18n("Remove Theme"), this);
  m_removeButton->setObjectName("RemoveTheme");
  m_removeButton->setToolTip(i18n("Remove the selected theme from your disk"));
  m_removeButton->setWhatsThis(i18n("This will remove the selected theme from your disk."));
  connect(m_removeButton,SIGNAL(clicked()),SLOT(removeSelectedTheme()));

  topLayout->addWidget(
	new QLabel(i18n("Select the icon theme you want to use:"), this));
  topLayout->addWidget(m_preview);
  topLayout->addWidget(m_iconThemes);
  QHBoxLayout *lg = new QHBoxLayout();
  lg->addWidget(newButton);
  lg->addStretch(0);
  lg->addWidget(installButton);
  lg->addWidget(m_removeButton);
  topLayout->addLayout(lg);

  loadThemes();

  m_defaultTheme=iconThemeItem(KIconTheme::current());
  if (m_defaultTheme)
    m_iconThemes->setCurrentItem(m_defaultTheme);
  updateRemoveButton();

  m_iconThemes->setFocus();
}

IconThemesConfig::~IconThemesConfig()
{
}

QTreeWidgetItem *IconThemesConfig::iconThemeItem(const QString &name)
{
  for (int i = 0; i < m_iconThemes->topLevelItemCount(); ++i)
    if (m_iconThemes->topLevelItem(i)->data(0, ThemeNameRole).toString()==name)
      return m_iconThemes->topLevelItem(i);

  return 0L;
}

void IconThemesConfig::loadThemes()
{
  m_iconThemes->clear();
  const QStringList themelist(KIconTheme::list());
  QString name;
  QString tname;
  QStringList::const_iterator it;
  QMap <QString, QString> themeNames;
  for (it=themelist.constBegin(); it != themelist.constEnd(); ++it)
  {
    KIconTheme icontheme(*it);
    if (!icontheme.isValid()) kDebug() << "notvalid\n";
    if (icontheme.isHidden()) continue;

    name=icontheme.name();
    tname=name;

 //  Just in case we have duplicated icon theme names on separate directories
    for (int i = 2; themeNames.find(tname) != themeNames.end(); ++i)
        tname=QString("%1-%2").arg(name).arg(i);

    QTreeWidgetItem *newitem = new QTreeWidgetItem();
    newitem->setText(0, name);
    newitem->setText(1, icontheme.description());
    newitem->setData(0, ThemeNameRole, *it);
    m_iconThemes->addTopLevelItem(newitem);

    themeNames.insert(name, *it);
  }
}

void IconThemesConfig::installNewTheme()
{
  KUrl themeURL = KUrlRequesterDialog::getUrl(QString(), this,
                                           i18n("Drag or Type Theme URL"));

  if (themeURL.url().isEmpty()) return;

  kDebug() << themeURL.prettyUrl();
  QString themeTmpFile;
  // themeTmpFile contains the name of the downloaded file

  if (!KIO::NetAccess::download(themeURL, themeTmpFile, this)) {
    QString sorryText;
    if (themeURL.isLocalFile())
       sorryText = i18n("Unable to find the icon theme archive %1.",
                        themeURL.prettyUrl());
    else
       sorryText = i18n("Unable to download the icon theme archive;\n"
                        "please check that address %1 is correct.",
                        themeURL.prettyUrl());
    KMessageBox::sorry(this, sorryText);
    return;
  }

  QStringList themesNames = findThemeDirs(themeTmpFile);
  if (themesNames.isEmpty()) {
    QString invalidArch(i18n("The file is not a valid icon theme archive."));
    KMessageBox::error(this, invalidArch);

    KIO::NetAccess::removeTempFile(themeTmpFile);
    return;
  }

  if (!installThemes(themesNames, themeTmpFile)) {
    //FIXME: make me able to know what is wrong....
    // QStringList instead of bool?
    QString somethingWrong =
        i18n("A problem occurred during the installation process; "
             "however, most of the themes in the archive have been installed");
    KMessageBox::error(this, somethingWrong);
  }

  KIO::NetAccess::removeTempFile(themeTmpFile);

  KIconLoader::global()->newIconLoader();
  loadThemes();

  QTreeWidgetItem *item=iconThemeItem(KIconTheme::current());
  if (item)
    m_iconThemes->setCurrentItem(item);
  updateRemoveButton();
}

bool IconThemesConfig::installThemes(const QStringList &themes, const QString &archiveName)
{
  bool everythingOk = true;
  QString localThemesDir(KStandardDirs::locateLocal("icon", "./"));

  KProgressDialog progressDiag(this,
                               i18n("Installing icon themes"),
                               QString());
  progressDiag.setModal(true);
  progressDiag.setAutoClose(true);
  QProgressBar* progressBar = progressDiag.progressBar();
  progressBar->setMaximum(themes.count());
  progressDiag.show();

  KTar archive(archiveName);
  archive.open(QIODevice::ReadOnly);
  kapp->processEvents();

  const KArchiveDirectory* rootDir = archive.directory();

  KArchiveDirectory* currentTheme;
  for (QStringList::ConstIterator it = themes.begin();
       it != themes.end();
       ++it) {
    progressDiag.setLabelText(
        i18n("<qt>Installing <strong>%1</strong> theme</qt>",
         *it));
    kapp->processEvents();

    if (progressDiag.wasCancelled())
      break;

    currentTheme = dynamic_cast<KArchiveDirectory*>(
                     const_cast<KArchiveEntry*>(
                       rootDir->entry(*it)));
    if (currentTheme == NULL) {
      // we tell back that something went wrong, but try to install as much
      // as possible
      everythingOk = false;
      continue;
    }

    currentTheme->copyTo(localThemesDir + *it);
    progressBar->setValue(progressBar->value()+1);
  }

  archive.close();
  return everythingOk;
}

QStringList IconThemesConfig::findThemeDirs(const QString &archiveName)
{
  QStringList foundThemes;

  KTar archive(archiveName);
  archive.open(QIODevice::ReadOnly);
  const KArchiveDirectory* themeDir = archive.directory();

  KArchiveEntry* possibleDir = 0L;
  KArchiveDirectory* subDir = 0L;

  // iterate all the dirs looking for an index.theme or index.desktop file
  QStringList entries = themeDir->entries();
  for (QStringList::const_iterator it = entries.constBegin();
       it != entries.constEnd();
       ++it) {
    possibleDir = const_cast<KArchiveEntry*>(themeDir->entry(*it));
    if (possibleDir->isDirectory()) {
      subDir = dynamic_cast<KArchiveDirectory*>( possibleDir );
      if (subDir && (subDir->entry("index.theme") != NULL ||
                     subDir->entry("index.desktop") != NULL))
        foundThemes.append(subDir->name());
    }
  }

  archive.close();
  return foundThemes;
}

void IconThemesConfig::getNewTheme()
{
  KNS3::DownloadDialog dialog("icons.knsrc", this);
  dialog.exec();
  if (!dialog.changedEntries().isEmpty()) {
    for(int i = 0; i < dialog.changedEntries().size(); i ++) {
      if(dialog.changedEntries().at(i).status() == KNS3::Entry::Installed
         && !dialog.changedEntries().at(i).installedFiles().isEmpty()) {
          const QString themeTmpFile = dialog.changedEntries().at(i).installedFiles().at(0);
          const QString name = dialog.changedEntries().at(i).installedFiles().at(0).section('/', -2, -2);
          kDebug()<<"IconThemesConfig::getNewTheme() themeTmpFile="<<themeTmpFile<<"name="<<name;
          QStringList themeNames = findThemeDirs(themeTmpFile);
          if (themeNames.isEmpty()) {
              //dialog.changedEntries().at(i)->setStatus(KNS3::Entry::Invalid);
          }
          else if (! installThemes(themeNames, themeTmpFile)) {
              //dialog.changedEntries().at(i)->setStatus(KNS3::Entry::Invalid);
          }
      }
    }

    // reload the display icontheme items
    KIconLoader::global()->newIconLoader();
    loadThemes();
    QTreeWidgetItem *item=iconThemeItem(KIconTheme::current());
    if (item)
        m_iconThemes->setCurrentItem(item);
    updateRemoveButton();
    load();
  }
}

void IconThemesConfig::removeSelectedTheme()
{
  QTreeWidgetItem *selected = m_iconThemes->currentItem();
  if (!selected)
     return;

  QString question=i18n("<qt>Are you sure you want to remove the "
        "<strong>%1</strong> icon theme?<br />"
        "<br />"
        "This will delete the files installed by this theme.</qt>",
	selected->text(0));

  bool deletingCurrentTheme=(selected==iconThemeItem(KIconTheme::current()));

  int r=KMessageBox::warningContinueCancel(this,question,i18n("Confirmation"),KStandardGuiItem::del());
  if (r!=KMessageBox::Continue) return;

  KIconTheme icontheme(selected->data(0, ThemeNameRole).toString());

  // delete the index file before the async KIO::del so loadThemes() will
  // ignore that dir.
  unlink(QFile::encodeName(icontheme.dir()+"/index.theme").data());
  unlink(QFile::encodeName(icontheme.dir()+"/index.desktop").data());
  KIO::del(KUrl( icontheme.dir() ));

  KIconLoader::global()->newIconLoader();

  loadThemes();

  QTreeWidgetItem *item=0L;
  //Fallback to the default if we've deleted the current theme
  if (!deletingCurrentTheme)
     item=iconThemeItem(KIconTheme::current());
  if (!item)
     item=iconThemeItem(KIconTheme::defaultThemeName());

  if (item)
    m_iconThemes->setCurrentItem(item);
  updateRemoveButton();

  if (deletingCurrentTheme) // Change the configuration
    save();
}

void IconThemesConfig::updateRemoveButton()
{
  QTreeWidgetItem *selected = m_iconThemes->currentItem();
  bool enabled = false;
  if (selected)
  {
    QString selectedtheme = selected->data(0, ThemeNameRole).toString();
    KIconTheme icontheme(selectedtheme);
    QFileInfo fi(icontheme.dir());
    enabled = fi.isWritable();
    // Don't let users remove the current theme.
    if (selectedtheme == KIconTheme::current() ||
        selectedtheme == KIconTheme::defaultThemeName())
      enabled = false;
  }
  m_removeButton->setEnabled(enabled);
}

void loadPreview(QLabel *label, KIconTheme& icontheme, const QStringList& iconnames)
{
    const int size = qMin(48, icontheme.defaultSize(KIconLoader::Desktop));
    KSvgRenderer renderer;
    foreach(const QString &name, iconnames) {
        K3Icon icon = icontheme.iconPath(QString("%1.png").arg(name), size, KIconLoader::MatchBest);
        if (icon.isValid()) {
            label->setPixmap(QPixmap(icon.path).scaled(size, size));
            return;
        }
        icon = icontheme.iconPath(QString("%1.svg").arg(name), size, KIconLoader::MatchBest);
        if( ! icon.isValid() ) {
            icon = icontheme.iconPath(QString("%1.svgz").arg(name), size, KIconLoader::MatchBest);
            if( ! icon.isValid() ) {
                continue;
            }
        }
        if (renderer.load(icon.path)) {
            QPixmap pix(size, size);
            pix.fill(label->palette().color(QPalette::Background));
            QPainter p(&pix);
            p.setViewport(0, 0, size, size);
            renderer.render(&p);
            label->setPixmap(pix.scaled(size, size));
            return;
        }
    }
    label->setPixmap(QPixmap());
}

void IconThemesConfig::themeSelected(QTreeWidgetItem *item)
{
  if (!item) return;

  QString dirName(item->data(0, ThemeNameRole).toString());
  KIconTheme icontheme(dirName);
  if (!icontheme.isValid()) kDebug() << "notvalid\n";

  updateRemoveButton();

  loadPreview(m_previewExec,     icontheme, QStringList() << "system-run" << "exec");
  loadPreview(m_previewFolder,   icontheme, QStringList() << "folder");
  loadPreview(m_previewDocument, icontheme, QStringList() << "document" << "text-x-generic");

  emit changed(true);
  m_bChanged = true;
}

void IconThemesConfig::load()
{
  m_defaultTheme=iconThemeItem(KIconTheme::current());
  if (m_defaultTheme)
    m_iconThemes->setCurrentItem(m_defaultTheme);
  emit changed(false);
  m_bChanged = false;
}

void IconThemesConfig::save()
{
  if (!m_bChanged)
     return;
  QTreeWidgetItem *selected = m_iconThemes->currentItem();
  if (!selected)
     return;

  KConfigGroup config(KSharedConfig::openConfig("kdeglobals", KConfig::SimpleConfig), "Icons");
  config.writeEntry("Theme", selected->data(0, ThemeNameRole).toString());
  config.sync();

  KIconTheme::reconfigure();
  emit changed(false);

  KIconCache::deleteCache();

  for (int i=0; i<KIconLoader::LastGroup; i++)
  {
    KGlobalSettings::self()->emitChange(KGlobalSettings::IconChanged, i);
  }

  KBuildSycocaProgressDialog::rebuildKSycoca(this);

  m_bChanged = false;
  m_removeButton->setEnabled(false);
}

void IconThemesConfig::defaults()
{
  if (m_iconThemes->currentItem()==m_defaultTheme) return;

  if (m_defaultTheme)
    m_iconThemes->setCurrentItem(m_defaultTheme);
  updateRemoveButton();

  emit changed(true);
  m_bChanged = true;
}

#include "iconthemes.moc"
