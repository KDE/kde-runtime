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

#include "emoticons.h"
#include <QFile>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <KDebug>
#include <KStandardDirs>
#include <KArchive>
#include <KProgressDialog>
#include <KMessageBox>
#include <KMimeType>
#include <KZip>
#include <KTar>
#include <kio/netaccess.h>
#include <klocalizedstring.h>


Emoticons::Emoticons(QWidget *parent, const QString &theme)
{
    parent = parent;
    themeName = theme;
    loadTheme();
    modified = false;
}

void Emoticons::loadTheme()
{
    fileName = "emoticons.xml";
    QFile fp(KGlobal::dirs()->findResource( "emoticons",  themeName + '/' +  "emoticons.xml"));

    if( !fp.exists() ) {
        fp.setFileName(KGlobal::dirs()->findResource( "emoticons",  themeName + '/' +  "icondef.xml"));
        if( !fp.exists() ) {
            kWarning() << themeName << "doesn't have an emoticons.xml or icondef.xml!";
            return;
        }
        fileName = "icondef.xml";
    }

    if(!fp.open( QIODevice::ReadOnly )) {
        kWarning() << fp.fileName() << " can't open ReadOnly!";
        return;
    }

    QString error;
    int eli, eco;
    if(!themeXml.setContent(&fp, &error, &eli, &eco)) {
        kWarning() << fp.fileName() << " can't copy to xml!";
        kWarning() << error << " " << eli << " " << eco;
        fp.close();
        return;
    }

    fp.close();
    if(fileName == "emoticons.xml") {
        loadThemeEmoticons();
    } else {
        loadThemeIcondef();
    }
}

void Emoticons::loadThemeEmoticons()
{
    QDomElement fce = themeXml.firstChildElement("messaging-emoticon-map");

    if(fce.isNull())
        return;

    QDomNodeList nl = fce.childNodes();

    themeMap.clear();

    for(uint i = 0; i < nl.length(); i++) {
        QDomElement de = nl.item(i).toElement();

        if(!de.isNull() && de.tagName() == "emoticon") {
            QDomNodeList snl = de.childNodes();
            QStringList sl;

            for(uint k = 0; k < snl.length(); k++) {
                QDomElement sde = snl.item(k).toElement();

                if(!sde.isNull() && sde.tagName() == "string") {
                    sl << sde.text();
                }
            }

            QString emo = KGlobal::dirs()->findResource( "emoticons", themeName + '/' + de.attribute("file"));

            if( emo.isNull() )
                emo = KGlobal::dirs()->findResource( "emoticons", themeName + '/' + de.attribute("file") + ".mng" );
            if ( emo.isNull() )
                emo = KGlobal::dirs()->findResource( "emoticons", themeName + '/' + de.attribute("file") + ".png" );
            if ( emo.isNull() )
                emo = KGlobal::dirs()->findResource( "emoticons", themeName + '/' + de.attribute("file") + ".gif" );
            if ( emo.isNull() )
                continue;

            themeMap[emo] = sl;
        }
    }
    
}

void Emoticons::loadThemeIcondef()
{
    QDomElement fce = themeXml.firstChildElement("icondef");

    if(fce.isNull())
        return;

    QDomNodeList nl = fce.childNodes();

    themeMap.clear();

    for(uint i = 0; i < nl.length(); i++) {
        QDomElement de = nl.item(i).toElement();

        if(!de.isNull() && de.tagName() == "icon") {
            QDomNodeList snl = de.childNodes();
            QStringList sl;
            QString emo;
            QStringList mime;
            mime << "image/png" << "image/gif" << "image/bmp" << "image/jpeg";
            
            for(uint k = 0; k < snl.length(); k++) {
                QDomElement sde = snl.item(k).toElement();

                if(!sde.isNull() && sde.tagName() == "text") {
                    sl << sde.text();
                } else if(!sde.isNull() && sde.tagName() == "object" && mime.contains(sde.attribute("mime"))) {
                    emo = sde.text();
                }
            }

            emo = KGlobal::dirs()->findResource( "emoticons", themeName + '/' + emo );

            if ( emo.isNull() )
                continue;

            themeMap[emo] = sl;
        }
    }
    
}

void Emoticons::save()
{
    if(!modified)
        return;

    QFile fp(KGlobal::dirs()->saveLocation( "emoticons",  themeName, false ) + '/' + fileName);

    if( !fp.exists() ) {
        kWarning() << fp.fileName() << "doesn't exist!";
        return;
    }

    if(!fp.open( QIODevice::WriteOnly )) {
        kWarning() << fp.fileName() << "can't open WriteOnly!";
        return;
    }

    QTextStream emoStream(&fp);
    emoStream << themeXml.toString(4);
    fp.close();
}

bool Emoticons::removeEmoticon(const QString &emo)
{
    if(fileName == "emoticons.xml") {
        return removeEmoticonEmoticons(emo);
    } else {
        return removeEmoticonIcondef(emo);
    }
}

bool Emoticons::removeEmoticonEmoticons(const QString &emo)
{
    QString emoticon = QFileInfo(themeMap.key(emo.split(" "))).fileName();
    QDomElement fce = themeXml.firstChildElement("messaging-emoticon-map");
    if(fce.isNull())
        return false;

    QDomNodeList nl = fce.childNodes();
    for(uint i = 0; i < nl.length(); i++) {
        QDomElement de = nl.item(i).toElement();
        if(!de.isNull() && de.tagName() == "emoticon" && (de.attribute("file") == emoticon || de.attribute("file") == QFileInfo(emoticon).baseName())) {
            fce.removeChild(de);
            themeMap.remove(themeMap.key(emo.split(" ")));
            modified = true;
            return true;
        }
    }
    return false;
}

bool Emoticons::removeEmoticonIcondef(const QString &emo)
{
    QString emoticon = QFileInfo(themeMap.key(emo.split(" "))).fileName();
    QDomElement fce = themeXml.firstChildElement("icondef");
    if(fce.isNull())
        return false;

    QDomNodeList nl = fce.childNodes();
    for(uint i = 0; i < nl.length(); i++) {
        QDomElement de = nl.item(i).toElement();
        if(!de.isNull() && de.tagName() == "icon") {
            QDomNodeList snl = de.childNodes();
            QStringList sl;
            QString emo;
            QStringList mime;
            
            for(uint k = 0; k < snl.length(); k++) {
                QDomElement sde = snl.item(k).toElement();

                if(!sde.isNull() && sde.tagName() == "object" && sde.text() == emoticon) {
                    fce.removeChild(de);
                    themeMap.remove(themeMap.key(emo.split(" ")));
                    modified = true;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Emoticons::addEmoticon(const QString &emo, const QString &text, bool copy)
{
    if(copy) {
        KIO::NetAccess::dircopy(KUrl(emo), KUrl(KGlobal::dirs()->saveLocation( "emoticons",  themeName, false )));
    }
    
    if(fileName == "emoticons.xml") {
        return addEmoticonEmoticons(emo, text);
    } else {
        return addEmoticonIcondef(emo, text);
    }
}

bool Emoticons::addEmoticonEmoticons(const QString &emo, const QString &text)
{
    QStringList splitted = text.split(" ");
    QDomElement fce = themeXml.firstChildElement("messaging-emoticon-map");
    if(fce.isNull())
        return false;

    QDomElement emoticon = themeXml.createElement("emoticon");
    emoticon.setAttribute("file", QFileInfo(emo).baseName());
    fce.appendChild(emoticon);
    QStringList::const_iterator constIterator;
    for(constIterator = splitted.begin(); constIterator != splitted.end(); constIterator++)
    {
        QDomElement emotext = themeXml.createElement("string");
        QDomText txt = themeXml.createTextNode((*constIterator).trimmed());
        emotext.appendChild(txt);
        emoticon.appendChild(emotext);
    }
    themeMap[emo] = splitted;
    modified = true;
    return true;
}

bool Emoticons::addEmoticonIcondef(const QString &emo, const QString &text)
{
    QStringList splitted = text.split(" ");
    QDomElement fce = themeXml.firstChildElement("icondef");
    if(fce.isNull())
        return false;

    QDomElement emoticon = themeXml.createElement("icon");
    fce.appendChild(emoticon);
    QStringList::const_iterator constIterator;
    for(constIterator = splitted.begin(); constIterator != splitted.end(); constIterator++)
    {
        QDomElement emotext = themeXml.createElement("text");
        QDomText txt = themeXml.createTextNode((*constIterator).trimmed());
        emotext.appendChild(txt);
        emoticon.appendChild(emotext);
    }
    QDomElement emoElement = themeXml.createElement("object");
    KMimeType::Ptr mimePtr = KMimeType::findByPath(emo, 0, true);
    emoElement.setAttribute("mime", mimePtr->name());
    QDomText txt = themeXml.createTextNode(QFileInfo(emo).fileName());

    emoElement.appendChild(txt);
    emoticon.appendChild(emoElement);
    
    themeMap[emo] = splitted;
    modified = true;
    return true;
}

void Emoticons::newTheme(const QString &name)
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\""));
    doc.appendChild(doc.createElement("messaging-emoticon-map"));

    QFile *fp =  new QFile(KGlobal::dirs()->saveLocation( "emoticons",  name, false ) + '/' + "emoticons.xml");

    if(!fp->open( QIODevice::WriteOnly )) {
        kWarning() << "Emoticons::newTheme() " << fp->fileName() << " can't open WriteOnly!";
        delete fp;
        return;
    }

    QTextStream emoStream(fp);
    emoStream << doc.toString(4);
    fp->close();
    delete fp;
}

QStringList Emoticons::installEmoticonTheme(QWidget *parent, const QString &archiveName)
{
    QStringList foundThemes;
    KArchiveEntry *currentEntry = 0L;
    KArchiveDirectory* currentDir = 0L;
    KProgressDialog *progressDlg = 0L;
    KArchive *archive = 0L;

    QString localThemesDir(KStandardDirs::locateLocal("emoticons", QString()) );

    if(localThemesDir.isEmpty())
    {
        KMessageBox::error(parent, i18n("Could not find suitable place to install emoticon theme into."));
        return QStringList();
    }

    progressDlg = new KProgressDialog(0, i18n("Installing Emoticon Theme..."));
    progressDlg->setModal(true);
    progressDlg->progressBar()->setMaximum(foundThemes.count());
    progressDlg->show();
    qApp->processEvents();

    QString currentBundleMimeType = KMimeType::findByPath(archiveName, 0, false)->name();
    if( currentBundleMimeType == QLatin1String("application/zip") ||
        currentBundleMimeType == QLatin1String("application/x-zip") ||
        currentBundleMimeType ==  QLatin1String("application/x-zip-compressed"))
        archive = new KZip(archiveName);
    else if( currentBundleMimeType == QLatin1String("application/x-compressed-tar") ||
                currentBundleMimeType == QLatin1String("application/x-bzip-compressed-tar") ||
                currentBundleMimeType == QLatin1String("application/x-gzip") ||
                currentBundleMimeType == QLatin1String("application/x-bzip") )
        archive = new KTar(archiveName);
    else if(archiveName.endsWith(QLatin1String("jisp")) || archiveName.endsWith(QLatin1String("zip")) )
        archive = new KZip(archiveName);
    else
        archive = new KTar(archiveName);

    if ( !archive || !archive->open(QIODevice::ReadOnly) )
    {
        KMessageBox::error(parent, i18n("Could not open \"%1\" for unpacking.", archiveName));
        delete archive;
        delete progressDlg;
        return QStringList();
    }

    const KArchiveDirectory* rootDir = archive->directory();

    // iterate all the dirs looking for an emoticons.xml file
    QStringList entries = rootDir->entries();
    for (QStringList::Iterator it = entries.begin(); it != entries.end(); ++it)
    {
        currentEntry = const_cast<KArchiveEntry*>(rootDir->entry(*it));
        if (currentEntry->isDirectory())
        {
            currentDir = dynamic_cast<KArchiveDirectory*>( currentEntry );
            if (currentDir && ( currentDir->entry(QLatin1String("emoticons.xml")) != NULL ||
                                currentDir->entry(QLatin1String("icondef.xml")) != NULL ) )
                foundThemes.append(currentDir->name());
        }
    }

    if (foundThemes.isEmpty())
    {
        KMessageBox::error(parent, i18n("<qt>The file \"%1\" is not a valid emoticon theme archive.</qt>", archiveName));
        archive->close();
        delete archive;
        delete progressDlg;
        return QStringList();
    }

    for (int themeIndex = 0; themeIndex < foundThemes.size(); ++themeIndex)
    {
        const QString &theme = foundThemes[themeIndex];

        progressDlg->setLabelText(i18n("<qt>Installing <strong>%1</strong> emoticon theme</qt>", theme));
        progressDlg->progressBar()->setValue(themeIndex);
        progressDlg->resize(progressDlg->sizeHint());
        qApp->processEvents();

        if (progressDlg->wasCancelled())
            break;

        currentEntry = const_cast<KArchiveEntry *>(rootDir->entry(theme));
        if (currentEntry == 0)
        {
            kDebug() << "couldn't get next archive entry";
            continue;
        }

        if(currentEntry->isDirectory())
        {
            currentDir = dynamic_cast<KArchiveDirectory*>(currentEntry);
            if (currentDir == 0)
            {
                kDebug() <<
                    "couldn't cast archive entry to KArchiveDirectory";
                continue;
            }
            if(QFile::exists(localThemesDir + theme)) {
                int ret = KMessageBox::warningYesNo(parent, i18n("A theme called %1 is already installed, are you sure you want to replace it?", theme));
                if(ret != KMessageBox::Yes) {
                    foundThemes.removeAt(themeIndex);
                    break;
                }
            }
            currentDir->copyTo(localThemesDir + theme);
        }
    }

    archive->close();
    delete archive;

    // check if all steps were done, if there are skipped ones then we didn't
    // succeed copying all dirs from the tarball
    if (progressDlg->progressBar()->maximum() > progressDlg->progressBar()->value())
    {
        KMessageBox::error(parent,i18n("<qt>A problem occurred during the installation process. "
            "However, some of the emoticon themes in the archive may have been "
            "installed.</qt>"));
    }

    delete progressDlg;
    return foundThemes;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
