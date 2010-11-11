/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MERGECONFLICTDELEGATE_H
#define MERGECONFLICTDELEGATE_H

#include <kwidgetitemdelegate.h>

class KPushButton;
class QPainter;

class MergeConflictDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

public:
    MergeConflictDelegate( QAbstractItemView* view, QObject* parent );
    ~MergeConflictDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QList<QWidget*> createItemWidgets() const;
    void updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const;

Q_SIGNALS:
    void requestResourceResolve(const QUrl& resource);
    void requestResourceDiscard(const QUrl& resource);

private Q_SLOTS:
    void slotResolveButtonClicked();
    void slotDiscardButtonClicked();

private:
    QString labelText(const QModelIndex& index) const;
    QString typeText(const QModelIndex& index) const;
    QString statusText(const QModelIndex& index, QPainter* painter = 0) const;
    QRect itemRect(const QStyleOptionViewItem &option, const QModelIndex& index) const;

    KPushButton* m_sizeHelperResolveButton;
    KPushButton* m_sizeHelperDiscardButton;
};

#endif // MERGECONFLICTDELEGATE_H
