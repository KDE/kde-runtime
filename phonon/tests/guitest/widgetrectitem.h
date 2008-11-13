/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef WIDGETRECTITEM_H
#define WIDGETRECTITEM_H

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QPoint>
#include <QtGui/QGraphicsRectItem>
#include <QtGui/QGraphicsView>
class PathItem;

class WidgetRectItem : public QGraphicsRectItem
{
    public:
        //WidgetRectItem(QGraphicsItem *parent);
        WidgetRectItem(const QPointF &pos, const QColor &color, const QString &title);

        ~WidgetRectItem();

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        enum { Type = UserType + 1 };
        int type() const { return Type; }

        void addPath(PathItem *p);
        void removePath(PathItem *p);

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:
        QString m_title;
        QList<PathItem *> m_paths;
        QSizeF m_lastSize;
};

#endif // WIDGETRECTITEM_H
