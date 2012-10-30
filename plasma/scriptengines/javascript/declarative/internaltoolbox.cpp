/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
 *   Copyright 2012 by Sebastian Kügler <sebas@kde.org>
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
#include <QTimer>

#include <KAuthorized>
#include <KIcon>
#include <KDebug>

#include <Plasma/Corona>

class InternalToolBoxPrivate {
public:
    bool immutable: true;
    bool showing: 1;
    Plasma::Containment *containment;
    QList<QAction*> actions;
};

InternalToolBox::InternalToolBox(Plasma::Containment *parent)
    : AbstractToolBox(parent)
{
    kDebug() << "0New InternalToolBox";
    d = new InternalToolBoxPrivate;
    d->containment = parent;
    init();
}

InternalToolBox::InternalToolBox(QObject *parent, const QVariantList &args)
    : AbstractToolBox(parent, args)
{
    kDebug() << "1New InternalToolBox";
    d = new InternalToolBoxPrivate;
    d->containment = qobject_cast<Plasma::Containment *>(parent);
    init();
}

InternalToolBox::~InternalToolBox()
{
    delete d;
}

void InternalToolBox::init()
{
    if (d->containment) {
        connect(d->containment, SIGNAL(immutabilityChanged(Plasma::ImmutabilityType)),
                this, SLOT(immutabilityChanged(Plasma::ImmutabilityType)));
    }
    if (KAuthorized::authorizeKAction("logout")) {
        QAction *action = new QAction(i18n("Leave..."), this);
        action->setIcon(KIcon("system-shutdown"));
        action->setObjectName("logout");
        addTool(action);
    }

    if (KAuthorized::authorizeKAction("lock_screen")) {
        QAction *action = new QAction(i18n("Lock Screen"), this);
        action->setObjectName("lock_screen");
        action->setIcon(KIcon("system-lock-screen"));
        addTool(action);
    }
    foreach (QAction* a, d->actions) {
        addTool(a);
        kDebug() << "Loaded tb action: " << a->text();
    }
    if (d->containment) {
        foreach (QAction *action, d->containment->corona()->actions()) {
            kDebug() << " Action from Corona: " << action->text();
            addTool(action);
        }
    }
}

QDeclarativeListProperty<QAction> InternalToolBox::actions()
{
    kDebug() << " Returning " << d->actions.count() << " Actions";
    return QDeclarativeListProperty<QAction>(this, d->actions);
}

void InternalToolBox::addTool(QAction *action)
{
    if (!action) {
        return;
    }
    kDebug() << "Added action: " << action->text();

    if (d->actions.contains(action)) {
        return;
    }

    connect(action, SIGNAL(destroyed(QObject*)), this, SLOT(actionDestroyed(QObject*)));
    d->actions.append(action);
    actionsChanged();
}

void InternalToolBox::removeTool(QAction *action)
{
    disconnect(action, 0, this, 0);
    d->actions.removeAll(action);
    emit actionsChanged();
}

void InternalToolBox::actionDestroyed(QObject *object)
{
    d->actions.removeAll(static_cast<QAction*>(object));
}

bool InternalToolBox::showing() const
{
    return d->showing;
}

bool InternalToolBox::isShowing() const
{
    return showing();
}

void InternalToolBox::setShowing(const bool show)
{
    if (d->showing == show) {
        return;
    }
    d->showing = show;
    emit showingChanged();
}

void InternalToolBox::immutabilityChanged(Plasma::ImmutabilityType immutability)
{
    const bool unlocked = immutability == (Plasma::Mutable);

    d->immutable = !unlocked;
    emit immutableChanged();
}

bool InternalToolBox::immutable() const
{
    return d->immutable;
}

void InternalToolBox::setImmutable(bool immutable)
{
    d->immutable = immutable;
    emit immutableChanged();
}

#include "internaltoolbox.moc"

