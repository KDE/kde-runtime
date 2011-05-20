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

#include "dialog.h"
#include "declarativeitemcontainer_p.h"

#include <QDeclarativeItem>
#include <QGraphicsObject>
#include <QGraphicsWidget>
#include <QTimer>

#include <Plasma/Corona>
#include <Plasma/Dialog>


DialogProxy::DialogProxy(QObject *parent)
    : QObject(parent)
{
    m_dialog = new Plasma::Dialog();
    m_flags = m_dialog->windowFlags();
}

DialogProxy::~DialogProxy()
{
    delete m_dialog;
    delete m_declarativeItemContainer;
}

QGraphicsObject *DialogProxy::mainItem() const
{
    return m_dialog->graphicsWidget();
}

void DialogProxy::setMainItem(QGraphicsObject *mainItem)
{
    if (m_mainItem.data() != mainItem) {
        if (m_mainItem) {
            m_mainItem.data()->setParent(mainItem->parent());
        }
        m_mainItem = mainItem;
        mainItem->setParentItem(0);
        mainItem->setParent(this);

        //if this is called in Compenent.onCompleted we have to wait a loop the item is added to a scene
        QTimer::singleShot(0, this, SLOT(syncMainItem()));
        emit mainItemChanged();
    }
}

void DialogProxy::syncMainItem()
{
    if (!m_mainItem) {
        return;
    }

    //not have a scene? go up in the hyerarchy until we find something with a scene
    QGraphicsScene *scene = m_mainItem.data()->scene();
    if (!scene) {
        QObject *parent = m_mainItem.data();
        while ((parent = parent->parent())) {
            QGraphicsObject *qo = qobject_cast<QGraphicsObject *>(parent);
            if (qo) {
                scene = qo->scene();
                scene->addItem(m_mainItem.data());
                break;
            }
        }
    }

    //the parent of the qobject never changed, only the parentitem, so put it back what it was
    m_mainItem.data()->setParentItem(qobject_cast<QGraphicsObject *>(m_mainItem.data()->parent()));

    QGraphicsWidget *widget = qobject_cast<QGraphicsWidget *>(m_mainItem.data());
    if (widget) {
        m_declarativeItemContainer->deleteLater();
        m_declarativeItemContainer = 0;
    } else {
        QDeclarativeItem *di = qobject_cast<QDeclarativeItem *>(m_mainItem.data());
        if (di) {
            if (!m_declarativeItemContainer) {
                m_declarativeItemContainer = new DeclarativeItemContainer();
                scene->addItem(m_declarativeItemContainer);
            }
            m_declarativeItemContainer->setDeclarativeItem(di);
            widget = m_declarativeItemContainer;
        }
    }
    m_dialog->setGraphicsWidget(widget);
}

bool DialogProxy::isVisible() const
{
    return m_dialog->isVisible();
}

void DialogProxy::setVisible(const bool visible)
{
    if (m_dialog->isVisible() != visible) {
        m_dialog->setVisible(visible);
        if (visible) {
            m_dialog->setWindowFlags(m_flags);
            m_dialog->raise();
        }
        emit visibleChanged();
    }
}

QPoint DialogProxy::popupPosition(QGraphicsObject *item) const
{
    Plasma::Corona *corona = qobject_cast<Plasma::Corona *>(item->scene());
    if (corona) {
        return corona->popupPosition(item, m_dialog->size());
    } else {
        return QPoint();
    }
}


int DialogProxy::x() const
{
    return m_dialog->pos().x();
}

void DialogProxy::setX(int x)
{
    m_dialog->move(x, m_dialog->pos().y());
}

int DialogProxy::y() const
{
    return m_dialog->pos().y();
}

void DialogProxy::setY(int y)
{
    m_dialog->move(m_dialog->pos().x(), y);
}

int DialogProxy::windowFlags() const
{
    return (int)m_dialog->windowFlags();
}

void DialogProxy::setWindowFlags(const int flags)
{
    m_flags = (Qt::WindowFlags)flags;
    m_dialog->setWindowFlags((Qt::WindowFlags)flags);
}

bool DialogProxy::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_dialog && event->type() == QEvent::Move) {
        emit positionChanged();
    }
    return false;
}

void DialogProxy::setAttribute(int attribute, bool on)
{
    m_dialog->setAttribute((Qt::WidgetAttribute)attribute, on);
}

#include "dialog.moc"
