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

#include "widgetrectitem.h"
#include <QtCore/QEvent>
#include <QtGui/QGraphicsProxyWidget>
#include "pathitem.h"

//X WidgetRectItem::WidgetRectItem(QGraphicsItem *parent)
//X     : QGraphicsRectItem(parent)
//X {
//X }

WidgetRectItem::WidgetRectItem(const QPointF &pos, const QColor &brush, const QString &title)
    : m_title(title)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setPos(pos);
    setBrush(brush);
}

WidgetRectItem::~WidgetRectItem()
{
}

void WidgetRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QSizeF size;
    QGraphicsProxyWidget *proxy = 0;
    if (childItems().size() == 1) {
        proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(
                childItems().first());
        QWidget *w = proxy->widget();
        if (w) {
            size = w->sizeHint();
        }
    }
    if (!size.isValid()) {
        size = childrenBoundingRect().size();
    }
    setRect(0.0, 0.0, size.width() + 32.0, size.height() + 30.0);
    QGraphicsRectItem::paint(painter, option, widget);
    painter->drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, m_title);

    if (size != m_lastSize) {
        if (proxy) {
            proxy->resize(size);
        }
        m_lastSize = size;
        foreach (PathItem *p, m_paths) {
            p->endPointMoved(this);
        }
    }

//X     const QPoint mapped = m_view->mapFromScene(pos());
//X     const QRectF frameGeometry(mapped.x() + 18, mapped.y() + 17, size.width(), size.height());
//X     if (childrenBoundingRect() != frameGeometry) {
//X         PathItem *pathItem = qgraphicsitem_cast<PathItem *>(parentItem());
//X         if (pathItem) {
//X             pathItem->updateChildrenPositions();
//X         }
//X     }
}

QVariant WidgetRectItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        foreach (PathItem *p, m_paths) {
            p->endPointMoved(this);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void WidgetRectItem::addPath(PathItem *p)
{
    m_paths.append(p);
}

void WidgetRectItem::removePath(PathItem *p)
{
    m_paths.removeAll(p);
}
