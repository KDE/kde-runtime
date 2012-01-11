/* This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KLISTDEBUGDIALOG__H
#define KLISTDEBUGDIALOG__H

#include "kabstractdebugdialog.h"
#include <QCheckBox>

class KTreeWidgetSearchLineWidget;
class QTreeWidget;

/**
 * Control debug output of KDE applications
 * This dialog offers a reduced functionality compared to the full KDebugDialog
 * class, but allows to set debug output on or off to several areas much more easily.
 *
 * @author David Faure <faure@kde.org>
 */
class KListDebugDialog : public KAbstractDebugDialog
{
    Q_OBJECT

public:
    explicit KListDebugDialog(const AreaMap& areaMap, QWidget *parent = 0);
    virtual ~KListDebugDialog() {}

    void activateArea( const QByteArray& area, bool activate );

protected Q_SLOTS:
    void selectAll();
    void deSelectAll();
    void disableAllClicked();

protected:
    virtual void doSave();
    virtual void doLoad();

private:
    QTreeWidget* m_areaWidget;
    KTreeWidgetSearchLineWidget *m_incrSearch;
    QWidget* m_buttonContainer;
};

#endif
