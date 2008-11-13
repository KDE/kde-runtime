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

#include "pathitem.h"
#include <QtGui/QLinearGradient>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

PathItem::PathItem(WidgetRectItem *start, WidgetRectItem *end, const Path &_path)
    : m_path(_path), m_startItem(start), m_endItem(end)
{
    m_startPos.setX(start->sceneBoundingRect().right());
    m_startPos.setY(start->sceneBoundingRect().center().y());
    m_endPos.setX(end->sceneBoundingRect().left());
    m_endPos.setY(end->sceneBoundingRect().center().y());
    updatePainterPath();

    setZValue(-1.0);
    start->addPath(this);
    end->addPath(this);
}

void PathItem::endPointMoved(const WidgetRectItem *item)
{
    if (item == m_startItem) {
        m_startPos.setX(item->sceneBoundingRect().right());
        m_startPos.setY(item->sceneBoundingRect().center().y());
    } else if (item == m_endItem) {
        m_endPos.setX(item->sceneBoundingRect().left());
        m_endPos.setY(item->sceneBoundingRect().center().y());
    }
    updatePainterPath();
}

void PathItem::updatePainterPath()
{
    QPainterPath path;
    path.moveTo(m_startPos);
    path.cubicTo(m_startPos + QPointF(150.0, 0.0),
            m_endPos - QPointF(150.0, 0.0),
            m_endPos);
    setPath(path);

    QLinearGradient gradient(m_startPos, m_endPos);
    gradient.setColorAt(0, QColor(64, 0, 0, 200));
    gradient.setColorAt(1, QColor(0, 64, 0, 200));
    setPen(QPen(gradient, 3.0));

    updateChildrenPositions();
}

void PathItem::updateChildrenPositions()
{
    const qreal divisor = QGraphicsItem::children().count() + 1;
    int positionOnLine = 0;
    foreach (QGraphicsItem *item, QGraphicsItem::children()) {
        item->setPos(QGraphicsPathItem::path().pointAtPercent(++positionOnLine / divisor) - item->boundingRect().center());
    }
}
