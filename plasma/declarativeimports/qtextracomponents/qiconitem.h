/***************************************************************************
 *   Copyright 2011 Marco Martin <mart@kde.org>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
#ifndef QICONITEM_H
#define QICONITEM_H

#include <QDeclarativeItem>
#include <QPixmap>
#include <QVariant>

class QIconItem : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(QVariant icon READ icon WRITE setIcon)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth)
    Q_PROPERTY(int implicitWidth READ implicitWidth NOTIFY implicitWidthChanged)
    Q_PROPERTY(int implicitHeight READ implicitHeight NOTIFY implicitHeightChanged)
    Q_PROPERTY(Group group READ group WRITE setGroup NOTIFY groupChanged)
    Q_ENUMS(Group)

public:
    enum Group {
        NoGroup, ///No group.
        Desktop, ///Desktop icons.
        FirstGroup, ///First group.
        Toolbar, ///Toolbar icons.
        MainToolbar, ///Main toolbar icons.
        Small, ///Small icons, e.g. for buttons.
        Panel, ///Panel (Plasma Taskbar) icons.
        Dialog, ///Icons for use in dialog titles, page lists, etc.
        LastGroup, ///Last group.
        User ///User icons.
    };

    QIconItem(QDeclarativeItem *parent=0);
    ~QIconItem();

    void setIcon(const QVariant &icon);
    QIcon icon() const;

    void setGroup(Group group);
    Group group() const;

    int implicitWidth() const;
    int implicitHeight() const;

    void setSmooth(const bool smooth);
    bool smooth() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

Q_SIGNALS:
    void implicitWidthChanged(int implicitWidth);
    void implicitHeightChanged(int implicitHeight);
    void groupChanged(Group group);

private:
    QIcon m_icon;
    bool m_smooth;
    Group m_group;
};

#endif
