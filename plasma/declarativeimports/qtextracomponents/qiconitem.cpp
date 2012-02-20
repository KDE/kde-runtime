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

#include "qiconitem.h"

#include <kicon.h>
#include <kiconloader.h>
#include <QtGui/QPainter>


QIconItem::QIconItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
      m_smooth(false),
      m_group(NoGroup)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}


QIconItem::~QIconItem()
{
}

void QIconItem::setIcon(const QVariant &icon)
{
    if(icon.canConvert<QIcon>()) {
        m_icon = icon.value<QIcon>();
    }
    else if(icon.canConvert<QString>()) {
        m_icon = KIcon(icon.toString());
    }
    update();
}

QIcon QIconItem::icon() const
{
    return m_icon;
}

void QIconItem::setGroup(QIconItem::Group group)
{
    if (m_group == group) {
        return;
    }

    m_group = group;
    emit groupChanged(group);
    emit implicitWidthChanged(implicitWidth());
    emit implicitHeightChanged(implicitHeight());
}

QIconItem::Group QIconItem::group() const
{
    return m_group;
}

int QIconItem::implicitWidth() const
{
    return KIconLoader::global()->currentSize((KIconLoader::Group)m_group);
}

int QIconItem::implicitHeight() const
{
    return KIconLoader::global()->currentSize((KIconLoader::Group)m_group);
}

void QIconItem::setSmooth(const bool smooth)
{
    if (smooth == m_smooth) {
        return;
    }
    m_smooth = smooth;
    update();
}

bool QIconItem::smooth() const
{
    return m_smooth;
}

void QIconItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_icon.isNull()) {
        return;
    }
    //do without painter save, faster and the support can be compiled out
    const bool wasAntiAlias = painter->testRenderHint(QPainter::Antialiasing);
    const bool wasSmoothTransform = painter->testRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing, m_smooth);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    m_icon.paint(painter, boundingRect().toRect(), Qt::AlignCenter, isEnabled()?QIcon::Normal:QIcon::Disabled);
    painter->setRenderHint(QPainter::Antialiasing, wasAntiAlias);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, wasSmoothTransform);
}


#include "qiconitem.moc"
