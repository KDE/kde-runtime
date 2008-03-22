/***************************************************************************
 *   Copyright (C) 2007 by Carlo Segato <brandon.ml@gmail.com>             *
 *   Copyright (C) 2008 Montel Laurent <montel@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "emoticonslist.h"
#include <QString>
#include <QDir>
#include <QIcon>
#include <QLabel>
#include <QListWidgetItem>
#include <KMessageBox>
#include <KIcon>
#include <KAboutData>
#include <KStandardDirs>
#include <KFileDialog>
#include <KInputDialog>
#include <KUrlRequesterDialog>
#include <kdecore_export.h>
#include <kio/netaccess.h>
#include <kgenericfactory.h>
#include <knewstuff2/engine.h>

EditDialog::EditDialog(QWidget *parent, const QString &name)
   : KDialog(parent)
{
    setCaption(name);
    setupDlg();
}

EditDialog::EditDialog(QWidget *parent, const QString &name, QListWidgetItem *itm, const QString &file)
    : KDialog(parent)
{
    setCaption(name);
    emoticon = file;
    setupDlg();
    leText->setText(itm->text());
    btnIcon->setIcon(itm->icon());
}

void EditDialog::setupDlg()
{
    wdg = new QWidget(this);
    QVBoxLayout *vl = new QVBoxLayout;
    QHBoxLayout *hb = new QHBoxLayout;
    leText = new KLineEdit(this);
    btnIcon = new QPushButton(this);
    btnIcon->setFixedSize(QSize(64, 64));

    QLabel *lab = new QLabel(wdg);
    lab->setWordWrap(true);
    vl->addWidget(lab);
    hb->addWidget(btnIcon);
    hb->addWidget(leText);
    vl->addLayout(hb);
    wdg->setLayout(vl);
    setMainWidget(wdg);
    connect(btnIcon, SIGNAL(clicked()), this, SLOT(btnIconClicked()));
    connect( leText, SIGNAL( textChanged( const QString & ) ), this, SLOT( updateOkButton() ) );
    updateOkButton();
    leText->setFocus();
}

void EditDialog::btnIconClicked()
{
    KUrl url =  KFileDialog::getImageOpenUrl();

    if(!url.isLocalFile())
        return;

    emoticon = url.path();

    if(emoticon.isEmpty())
        return;

    btnIcon->setIcon(QPixmap(emoticon));
    updateOkButton();
}

void EditDialog::updateOkButton()
{
    enableButtonOk( !leText->text().isEmpty() && !emoticon.isEmpty() );
}

K_PLUGIN_FACTORY(EmoticonsFactory, registerPlugin<EmoticonList>();)
K_EXPORT_PLUGIN(EmoticonsFactory("emoticons"))

EmoticonList::EmoticonList(QWidget *parent, const QVariantList &args)
    : KCModule(EmoticonsFactory::componentData(), parent, args)
{
    KAboutData *about = new KAboutData("kcm_emoticons", 0, ki18n("Emoticons"),"1.0");
    setAboutData(about);
    setButtons(Apply|Help);
    setupUi(this);
    btAdd->setIcon(KIcon("list-add"));
    btEdit->setIcon(KIcon("edit-rename"));
    btRemoveEmoticon->setIcon(KIcon("edit-delete"));
    btNew->setIcon(KIcon("document-new"));
    btGetNew->setIcon(KIcon("get-hot-new-stuff"));
    btInstall->setIcon(KIcon("document-import"));
    btRemoveTheme->setIcon(KIcon("edit-delete"));

    load();

    connect(themeList, SIGNAL(itemSelectionChanged()), this, SLOT(selectTheme()));
    connect(themeList, SIGNAL(itemSelectionChanged()), this, SLOT(updateButton()));
    connect(btRemoveTheme, SIGNAL(clicked()), this, SLOT(btRemoveThemeClicked()));
    connect(btInstall, SIGNAL(clicked()), this, SLOT(installEmoticonTheme()));
    connect(btNew, SIGNAL(clicked()), this, SLOT(newTheme()));
    connect(btGetNew, SIGNAL(clicked()), this, SLOT(getNewStuff()));

    connect(btAdd, SIGNAL(clicked()), this, SLOT(addEmoticon()));
    connect(btEdit, SIGNAL(clicked()), this, SLOT(editEmoticon()));
    connect(btRemoveEmoticon, SIGNAL(clicked()), this, SLOT(btRemoveEmoticonClicked()));
    connect(emoList, SIGNAL(itemSelectionChanged()), this, SLOT(updateButton()));
    connect(emoList, SIGNAL( itemDoubleClicked ( QListWidgetItem *) ), this, SLOT( editEmoticon()));
}

EmoticonList::~EmoticonList()
{
    QMapIterator<QString, Emoticons*> it(emoMap);
    while (it.hasNext()) {
        it.next();
        delete it.value();
    }
    emoMap.clear();
}


void EmoticonList::load()
{
    KStandardDirs dir;

    delFiles.clear();
    themeList->clear();
    QMapIterator<QString, Emoticons*> it(emoMap);
    while (it.hasNext()) {
        it.next();
        delete it.value();
    }
    emoMap.clear();
    emoList->clear();

    QStringList themeDirs = KGlobal::dirs()->findDirs("emoticons", "");

    for(int i = 0; i < themeDirs.count(); i++)
    {
        QDir themeQDir(themeDirs[i]);
        themeQDir.setFilter( QDir::Dirs );
        themeQDir.setSorting( QDir::Name );

        for(uint y = 0; y < themeQDir.count(); y++)
        {
            if ( themeQDir[y] != "." && themeQDir[y] != ".." )
            {
                loadTheme(themeQDir[y]);
            }
        }
    }
    emit changed(false);
}

void EmoticonList::save()
{
    for(int i = 0; i < delFiles.size(); i++)
    {
        KIO::NetAccess::del(delFiles.at(i), this);
    }

    QMapIterator<QString, Emoticons*> it(emoMap);
    while (it.hasNext()) {
        it.next();
        it.value()->save();
    }
}

void EmoticonList::updateButton()
{
    btRemoveEmoticon->setEnabled( themeList->currentItem() && emoList->selectedItems().size() );
    btRemoveTheme->setEnabled( themeList->selectedItems().size() );
    btEdit->setEnabled( themeList->currentItem() && emoList->selectedItems().size() );
    btAdd->setEnabled( themeList->currentItem() );
}

void EmoticonList::selectTheme()
{
    updateButton();
    if ( !themeList->currentItem() )
    {
        emoList->clear();
        return;
    }

    if(!themeList->currentItem()) {
        themeList->currentItem()->setSelected(true);
        return;
    }
    emoList->clear();

    Emoticons *em = emoMap.value(themeList->currentItem()->text());
    QMapIterator <QString, QStringList> it(em->getThemeMap());

    while(it.hasNext()) {
        it.next();
        QString text;
        if(it.value().size()) {
            text = it.value().at(0);
            for(int i = 1; i < it.value().size(); i++) {
                text += ' ' + it.value().at(i);
            }
        }
//         kDebug() << "icon: "<<it.key()<<"text: "<<text<<"\n\r";
        new QListWidgetItem(QIcon(it.key()), text, emoList);
    }
}

void EmoticonList::btRemoveThemeClicked()
{
    if(!themeList->currentItem())
        return;

    QString name = themeList->currentItem()->text();

//    int ret = KMessageBox::warningYesNo(this, i18n("Are you sure you want to remove %1 emoticon theme?", name));
  //  if(ret == KMessageBox::Yes) {
        delFiles.append(KGlobal::dirs()->findResource("emoticons", name + QDir::separator()));
        delete themeList->currentItem();
        emoMap.remove(name);
        emit changed();
    //}
}

void EmoticonList::installEmoticonTheme()
{
    KUrl themeURL = KUrlRequesterDialog::getUrl(QString(), this, i18n("Drag or Type Emoticon Theme URL"));
    if ( themeURL.isEmpty() )
        return;

    if ( !themeURL.isLocalFile() )
    {
        KMessageBox::queuedMessageBox( this, KMessageBox::Error, i18n("Sorry, emoticon themes must be installed from local files."),
        i18n("Could Not Install Emoticon Theme") );
        return;
    }
    QStringList installed = Emoticons::installEmoticonTheme(this, themeURL.path() );
    for(int i = 0; i < installed.size(); i ++)
        loadTheme(installed.at(i));
}

void EmoticonList::btRemoveEmoticonClicked()
{
    if(!emoList->currentItem())
        return;

    QListWidgetItem *itm = emoList->currentItem();
    if(emoMap.value(themeList->currentItem()->text())->removeEmoticon(itm->text())) {
        delete itm;
        themeList->currentItem()->setIcon(QIcon(emoMap.value(themeList->currentItem()->text())->getThemeMap().keys().value(0)));
        emit changed();
    }
}

void EmoticonList::addEmoticon()
{
    if(!themeList->currentItem())
        return;

    EditDialog *dlg = new EditDialog(this, i18n("Add emoticon"));

    if(dlg->exec() == QDialog::Rejected )
    {
        delete dlg;
        return;
    }
    if(emoMap.value(themeList->currentItem()->text())->addEmoticon(dlg->getEmoticon(), dlg->getText(), true)) {
        new QListWidgetItem(QPixmap(dlg->getEmoticon()), dlg->getText(), emoList);
        themeList->currentItem()->setIcon(QIcon(emoMap.value(themeList->currentItem()->text())->getThemeMap().keys().value(0)));
        emit changed();
    }
    delete dlg;
}

void EmoticonList::editEmoticon()
{
    if(!themeList->currentItem() || !emoList->currentItem())
        return;

    QString path = emoMap.value(themeList->currentItem()->text())->getThemeMap().key(emoList->currentItem()->text().split(' '));
    QString f = QFileInfo(path).baseName();
    EditDialog *dlg = new EditDialog(this, i18n("Edit emoticon"), emoList->currentItem(), path);

    if(dlg->exec() == QDialog::Rejected)
    {
        delete dlg;
        return;
    }
    bool copy;
    QString emo = dlg->getEmoticon();
    if(path != dlg->getEmoticon()) {
        copy = true;
    } else {
        copy = false;

        KStandardDirs *dir = KGlobal::dirs();
        emo = dir->findResource( "emoticons", themeList->currentItem()->text() + QDir::separator() + f);

        if( emo.isNull() )
            emo = dir->findResource( "emoticons", themeList->currentItem()->text() + QDir::separator()  + f + ".mng" );
        if ( emo.isNull() )
            emo = dir->findResource( "emoticons", themeList->currentItem()->text() + QDir::separator()  + f + ".png" );
        if ( emo.isNull() )
            emo = dir->findResource( "emoticons", themeList->currentItem()->text() + QDir::separator()  + f + ".gif" );
        if ( emo.isNull() )
        {
            delete dlg;
            return;
        }
    }

    if(emoMap.value(themeList->currentItem()->text())->removeEmoticon(emoList->currentItem()->text())) {
        delete emoList->currentItem();
    }
    if(emoMap.value(themeList->currentItem()->text())->addEmoticon(emo, dlg->getText(), copy)) {
        new QListWidgetItem(QPixmap(emo), dlg->getText(), emoList);
    }
    emit changed();
    delete dlg;
}

void EmoticonList::newTheme()
{
    QString name = KInputDialog::getText(i18n("New Emoticon Theme"), i18n("Enter the name of the new emoticon theme"));
    if(name.isEmpty())
      return;
    QString path = KGlobal::dirs()->saveLocation("emoticons", name, false);

    if(KIO::NetAccess::exists(path, KIO::NetAccess::SourceSide, this)) {
        KMessageBox::error(this, i18n("%1 theme already exists", name));
    } else {
        KIO::NetAccess::mkdir(KUrl(path), this);
        Emoticons::newTheme(name);
        loadTheme(name);
    }
}

void EmoticonList::loadTheme(const QString &name)
{
    if(name.isEmpty())
        return;
    
    if(emoMap.contains(name)) {
        delete emoMap.value(name);
        emoMap.remove(name);
        QList<QListWidgetItem *>ls = themeList->findItems(name, Qt::MatchExactly);
        if(ls.size()) {
            delete ls.at(0);
        }
    }
    
    Emoticons *emo = new Emoticons(this, name);
    emoMap[name] = emo;
    QIcon previewIcon = QIcon(emo->getThemeMap().keys().value(0));
    new QListWidgetItem(previewIcon, name, themeList);
}

void EmoticonList::getNewStuff()
{
    KNS::Engine engine(this);
    if (engine.init("emoticons.knsrc")) {
        KNS::Entry::List entries = engine.downloadDialogModal(this);

        for(int i = 0; i < entries.size(); i ++) {
            if(entries.at(i)->status() == KNS::Entry::Installed) {
                QString name = entries.at(i)->installedFiles().at(0).section('/', -2, -2);
                loadTheme(name);
            }
        }
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
