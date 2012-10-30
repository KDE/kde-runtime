/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
 *   Copyright 2012 by Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.

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

#ifndef PLASMA_INTERNALTOOLBOX_P_H
#define PLASMA_INTERNALTOOLBOX_P_H

#include <Plasma/Plasma>
#include <QDeclarativeListProperty>

#include <Plasma/AbstractToolBox>

class QAction;

class InternalToolBoxPrivate;

class InternalToolBox : public Plasma::AbstractToolBox
{
    Q_OBJECT
    Q_PROPERTY(bool immutable READ immutable WRITE setImmutable NOTIFY immutableChanged)
    Q_PROPERTY(QDeclarativeListProperty<QAction> actions READ actions NOTIFY actionsChanged)
    Q_PROPERTY(bool showing READ isShowing WRITE setShowing NOTIFY showingChanged)

public:
    explicit InternalToolBox(Plasma::Containment *parent);
    explicit InternalToolBox(QObject *parent = 0, const QVariantList &args = QVariantList());
    ~InternalToolBox();

    bool immutable() const;
    void setImmutable(bool immutable);
    bool isShowing() const; // satisfy badly named API
    bool showing() const;

    QDeclarativeListProperty<QAction> actions();

public Q_SLOTS:
    Q_INVOKABLE void setShowing(const bool show);

Q_SIGNALS:
    void actionsChanged();
    void immutableChanged();
    void showingChanged();

private Q_SLOTS:
    void actionDestroyed(QObject *object);
    void immutabilityChanged(Plasma::ImmutabilityType immutability);

private:
    void init();
    /**
     * create a toolbox tool from the given action
     * @p action the action to associate hte tool with
     */
    void addTool(QAction *action);
    /**
     * remove the tool associated with this action
     */
    void removeTool(QAction *action);

    InternalToolBoxPrivate* d;
};

#endif // PLASMA_INTERNALTOOLBOX_P_H
