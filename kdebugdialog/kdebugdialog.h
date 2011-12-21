/* This file is part of the KDE libraries
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)

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
#ifndef _KDEBUGDIALOG
#define _KDEBUGDIALOG

#include "kabstractdebugdialog.h"

#include "ui_kdebugdialog.h"

class QLineEdit;
class QComboBox;
class QLabel;
class QGroupBox;
class QCheckBox;

/**
 * Control debug/warning/error/fatal output of KDE applications
 *
 * This dialog allows control of debugging output for all KDE apps.
 *
 * @author Kalle Dalheimer (kalle@kde.org)
 */
class KDebugDialog : public KAbstractDebugDialog, public Ui_KDebugDialog
{
    Q_OBJECT

public:
    explicit KDebugDialog(const AreaMap& areaMap, QWidget *parent = 0);
    virtual ~KDebugDialog();

    void doLoad() {}
    void doSave();

protected Q_SLOTS:
    void slotDebugAreaChanged(QTreeWidgetItem*);
    void slotDestinationChanged();
    void disableAllClicked();

private:
    void showArea(const QString& areaName);

    QString mCurrentDebugArea;

private:
    // Disallow assignment and copy-construction
    KDebugDialog( const KDebugDialog& );
    KDebugDialog& operator= ( const KDebugDialog& );
};

#endif
