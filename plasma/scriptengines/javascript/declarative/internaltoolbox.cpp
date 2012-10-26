/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
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

#include "internaltoolbox.h"

#include <QAction>
#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QRadialGradient>

#include <KColorScheme>
#include <KConfigGroup>
#include <KIconLoader>
#include <KDebug>

#include <Plasma/Corona>
#include <Plasma/Theme>
#include <Plasma/IconWidget>


InternalToolBox::InternalToolBox(Plasma::Containment *parent)
    : AbstractToolBox(parent),
      m_containment(parent),
      m_corner(InternalToolBox::TopRight),
      m_size(KIconLoader::SizeSmallMedium),
      m_iconSize(KIconLoader::SizeSmall, KIconLoader::SizeSmall),
      m_hidden(false),
      m_showing(false),
      m_movable(false),
      m_dragging(false),
      m_userMoved(false),
      m_iconic(true)
{
    init();
}

InternalToolBox::InternalToolBox(QObject *parent, const QVariantList &args)
    : AbstractToolBox(parent, args),
      m_containment(qobject_cast<Plasma::Containment *>(parent)),
      m_corner(InternalToolBox::TopRight),
      m_size(KIconLoader::SizeSmallMedium),
      m_iconSize(KIconLoader::SizeSmall, KIconLoader::SizeSmall),
      m_hidden(false),
      m_showing(false),
      m_movable(false),
      m_dragging(false),
      m_userMoved(false),
      m_iconic(true)
{
    init();
}

InternalToolBox::~InternalToolBox()
{
}

void InternalToolBox::init()
{
    if (m_containment) {
        connect(m_containment, SIGNAL(immutabilityChanged(Plasma::ImmutabilityType)),
                this, SLOT(immutabilityChanged(Plasma::ImmutabilityType)));
    }

    setAcceptsHoverEvents(true);
}

Plasma::Containment *InternalToolBox::containment()
{
    return m_containment;
}

QList<QAction *> InternalToolBox::actions() const
{
    return m_actions;
}

void InternalToolBox::addTool(QAction *action)
{
    if (!action) {
        return;
    }

    if (m_actions.contains(action)) {
        return;
    }

    connect(action, SIGNAL(destroyed(QObject*)), this, SLOT(actionDestroyed(QObject*)));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(toolTriggered(bool)));
    m_actions.append(action);
}

void InternalToolBox::removeTool(QAction *action)
{
    disconnect(action, 0, this, 0);
    m_actions.removeAll(action);
}

void InternalToolBox::actionDestroyed(QObject *object)
{
    m_actions.removeAll(static_cast<QAction*>(object));
}

bool InternalToolBox::isEmpty() const
{
    return m_actions.isEmpty();
}

void InternalToolBox::toolTriggered(bool)
{
}

int InternalToolBox::size() const
{
    return  m_size;
}

void InternalToolBox::setSize(const int newSize)
{
    m_size = newSize;
}

QSize InternalToolBox::iconSize() const
{
    return m_iconSize;
}

void InternalToolBox::setIconSize(const QSize newSize)
{
    m_iconSize = newSize;
}

bool InternalToolBox::isShowing() const
{
    return m_showing;
}

void InternalToolBox::setShowing(const bool show)
{
    if (show) {
        showToolBox();
    } else {
        hideToolBox();
    }
    m_showing = show;
}

QSize InternalToolBox::cornerSize() const
{
    return boundingRect().size().toSize();
}

QSize InternalToolBox::fullWidth() const
{
    return boundingRect().size().toSize();
}

QSize  InternalToolBox::fullHeight() const
{
    return boundingRect().size().toSize();
}


bool InternalToolBox::isMovable() const
{
    return m_movable;
}

void InternalToolBox::setIsMovable(bool movable)
{
    m_movable = movable;
}

void InternalToolBox::setCorner(const Corner corner)
{
    m_corner = corner;
}

InternalToolBox::Corner InternalToolBox::corner() const
{
    return m_corner;
}

void InternalToolBox::save(KConfigGroup &cg) const
{
    if (!m_movable) {
        return;
    }

    KConfigGroup group(&cg, "ToolBox");
    if (!m_userMoved) {
        group.deleteGroup();
        return;
    }

    int offset = 0;
    if (corner() == InternalToolBox::Left ||
        corner() == InternalToolBox::Right) {
        offset = y();
    } else if (corner() == InternalToolBox::Top ||
               corner() == InternalToolBox::Bottom) {
        offset = x();
    }

    group.writeEntry("corner", int(corner()));
    group.writeEntry("offset", offset);
}

void InternalToolBox::restore(const KConfigGroup &containmentGroup)
{
    KConfigGroup group = KConfigGroup(&containmentGroup, "ToolBox");

    if (!group.hasKey("corner")) {
        return;
    }

    m_userMoved = true;
    setCorner(Corner(group.readEntry("corner", int(corner()))));

    const int offset = group.readEntry("offset", 0);
    const int w = boundingRect().width();
    const int h = boundingRect().height();
    const int maxW = m_containment ? m_containment->geometry().width() - w : offset;
    const int maxH = m_containment ? m_containment->geometry().height() - h : offset;
    switch (corner()) {
        case InternalToolBox::TopLeft:
            setPos(0, 0);
            break;
        case InternalToolBox::Top:
            setPos(qMin(offset, maxW), 0);
            break;
        case InternalToolBox::TopRight:
            setPos(m_containment->size().width() - boundingRect().width(), 0);
            break;
        case InternalToolBox::Right:
            setPos(m_containment->size().width() - boundingRect().width(), qMin(offset, maxH));
            break;
        case InternalToolBox::BottomRight:
            setPos(m_containment->size().width() - boundingRect().width(), m_containment->size().height() - boundingRect().height());
            break;
        case InternalToolBox::Bottom:
            setPos(qMin(offset, maxW), m_containment->size().height() - boundingRect().height());
            break;
        case InternalToolBox::BottomLeft:
            setPos(0, m_containment->size().height() - boundingRect().height());
            break;
        case InternalToolBox::Left:
            setPos(0, qMin(offset, maxH));
            break;
    }
    //kDebug() << "marked as user moved" << pos()
    //         << (m_containment->containmentType() == Containment::PanelContainment);
}

void InternalToolBox::immutabilityChanged(Plasma::ImmutabilityType immutability)
{
    const bool unlocked = immutability == (Plasma::Mutable);

    if (containment() &&
        (containment()->type() == Plasma::Containment::PanelContainment ||
         containment()->type() == Plasma::Containment::CustomPanelContainment)) {
        setVisible(unlocked);
    } else {
        setIsMovable(unlocked);
    }
}

#include "internaltoolbox.moc"

