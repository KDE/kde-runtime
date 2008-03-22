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

#ifndef EMOTICONS_H
#define EMOTICONS_H

#include <QDomDocument>
#include <QStringList>
#include <QMap>
class Emoticons
{   
    public:
        Emoticons(QWidget *parent, const QString &theme);
        QMap<QString, QStringList> getThemeMap() { return themeMap; }
        bool removeEmoticon(const QString &emo);
        bool addEmoticon(const QString &emo, const QString &text, bool copy);
        void save();
        static void newTheme(const QString &name);
        static QStringList installEmoticonTheme(QWidget *parent, const QString &archiveName);
        
    private:
        void loadTheme();
        void loadThemeEmoticons();
        void loadThemeIcondef();
        bool removeEmoticonEmoticons(const QString &emo);
        bool removeEmoticonIcondef(const QString &emo);
        bool addEmoticonEmoticons(const QString &emo, const QString &text);
        bool addEmoticonIcondef(const QString &emo, const QString &text);
        QString themeName;
        QMap<QString, QStringList> themeMap;
        QDomDocument themeXml;
        bool modified;
        QWidget *parent;
        QString fileName;
        
};

#endif /* EMOTICONS_H */

// kate: space-indent on; indent-width 4; replace-tabs on;
