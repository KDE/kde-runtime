/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "qpixmapitem.h"

#include <QPainter>


QPixmapItem::QPixmapItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
      m_smooth(false),
      m_fillMode(QPixmapItem::Stretch)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}


QPixmapItem::~QPixmapItem()
{
}

void QPixmapItem::setPixmap(const QPixmap &pixmap)
{
    bool oldPixmapNull = m_pixmap.isNull();
    m_pixmap = pixmap;
    update();
    emit nativeWidthChanged();
    emit nativeHeightChanged();
    emit pixmapChanged();
    if (oldPixmapNull != m_pixmap.isNull()) {
        emit nullChanged();
    }
}

QPixmap QPixmapItem::pixmap() const
{
    return m_pixmap;
}

void QPixmapItem::setSmooth(const bool smooth)
{
    if (smooth == m_smooth) {
        return;
    }
    m_smooth = smooth;
    update();
}

bool QPixmapItem::smooth() const
{
    return m_smooth;
}

int QPixmapItem::nativeWidth() const
{
    return m_pixmap.size().width();
}

int QPixmapItem::nativeHeight() const
{
    return m_pixmap.size().height();
}

QPixmapItem::FillMode QPixmapItem::fillMode() const
{
    return m_fillMode;
}

void QPixmapItem::setFillMode(QPixmapItem::FillMode mode)
{
    if (mode == m_fillMode) {
        return;
    }

    m_fillMode = mode;
    update();
    emit fillModeChanged();
}

void QPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_pixmap.isNull()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, m_smooth);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    QRect destRect;
    switch (m_fillMode) {
    case PreserveAspectFit: {
        QSize scaled = m_pixmap.size();

        scaled.scale(boundingRect().size().toSize(), Qt::KeepAspectRatio);
        destRect = QRect(QPoint(0, 0), scaled);
        break;
    }
    case PreserveAspectCrop: {
        painter->setClipRect(boundingRect(), Qt::IntersectClip);
        QSize scaled = m_pixmap.size();
        scaled.scale(boundingRect().size().toSize(), Qt::KeepAspectRatioByExpanding);
        destRect = QRect(QPoint(0, 0), scaled);
        break;
    }
    case TileVertically: {
        painter->scale(width()/(qreal)m_pixmap.width(), 1);
        destRect = boundingRect().toRect();
        destRect.setWidth(destRect.width() / (width()/(qreal)m_pixmap.width()));
        break;
    }
    case TileHorizontally: {
        painter->scale(1, height()/(qreal)m_pixmap.height());
        destRect = boundingRect().toRect();
        destRect.setHeight(destRect.height() / (height()/(qreal)m_pixmap.height()));
        break;
    }
    case Stretch:
    case Tile:
    default:
        destRect = boundingRect().toRect();
    }

    if (m_fillMode >= Tile) {
        painter->drawTiledPixmap(destRect, m_pixmap);
    } else {
        painter->drawPixmap(destRect, m_pixmap, m_pixmap.rect());
    }

    painter->restore();
}

bool QPixmapItem::isNull() const
{
    return m_pixmap.isNull();
}

#include "qpixmapitem.moc"
