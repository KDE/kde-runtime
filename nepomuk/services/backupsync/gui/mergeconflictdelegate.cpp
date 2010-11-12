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

#include "mergeconflictdelegate.h"
#include "identifiermodel.h"

#include <QtGui/QStyleOptionViewItemV4>
#include <QtGui/QPainter>
#include <QtGui/QTreeView>
#include <QFontMetrics>

#include <kpushbutton.h>
#include <kicon.h>
#include <klocale.h>
#include <krandom.h>
#include <kdebug.h>
#include <KColorScheme>
#include <kurl.h>

#include <Nepomuk/Types/Class>


namespace {
const int s_spacing = 5;

int itemDepth(const QModelIndex& index) {
    int d = 1;
    QModelIndex p = index.parent();
    while(p.isValid()) {
        ++d;
        p = p.parent();
    }
    return d;
}
}

MergeConflictDelegate::MergeConflictDelegate( QAbstractItemView* view, QObject* parent )
    : KWidgetItemDelegate( view, parent ),
      m_sizeHelperResolveButton(new KPushButton),
      m_sizeHelperDiscardButton(new KPushButton)
{
    m_sizeHelperResolveButton->setText(i18nc("@action:button Resolve the conflict in this row", "Resolve..."));
    m_sizeHelperResolveButton->setIcon(KIcon(QLatin1String("list-add")));
    m_sizeHelperDiscardButton->setText(i18nc("@action:button Discard the item in this row", "Discard"));
    m_sizeHelperDiscardButton->setIcon(KIcon(QLatin1String("list-remove")));
}

MergeConflictDelegate::~MergeConflictDelegate()
{
    delete m_sizeHelperResolveButton;
}

QList<QWidget *> MergeConflictDelegate::createItemWidgets() const
{
    kDebug();
    QList<QWidget*> widgets;
    KPushButton* resolveButton = new KPushButton;
    resolveButton->setText(m_sizeHelperResolveButton->text());
    resolveButton->setIcon(KIcon(QLatin1String("list-add")));
    connect(resolveButton, SIGNAL(clicked()), this, SLOT(slotResolveButtonClicked()));

    KPushButton* discardButton = new KPushButton;
    discardButton->setText(m_sizeHelperDiscardButton->text());
    discardButton->setIcon(KIcon(QLatin1String("list-remove")));
    connect(discardButton, SIGNAL(clicked()), this, SLOT(slotDiscardButtonClicked()));

    widgets << resolveButton << discardButton;
    return widgets;
}

void MergeConflictDelegate::updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    const QRect rect = itemRect(option, index);

    KPushButton* resolveButton = static_cast<KPushButton*>(widgets[0]);
    KPushButton* discardButton = static_cast<KPushButton*>(widgets[1]);

    const QSize resolveButtonSize = resolveButton->sizeHint();
    const QSize discardButtonSize = discardButton->sizeHint();

    resolveButton->resize(resolveButtonSize);
    discardButton->resize(discardButtonSize);

    resolveButton->move(rect.right() - resolveButtonSize.width(), rect.height() / 2 - resolveButtonSize.height() / 2);
    discardButton->move(rect.right() - resolveButtonSize.width() - discardButtonSize.width() - s_spacing, rect.height() / 2 - discardButtonSize.height() / 2);
}

void MergeConflictDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    const QRect rect = itemRect(option, index);
    const int buttonWidth = m_sizeHelperDiscardButton->sizeHint().width() + 2*s_spacing + m_sizeHelperResolveButton->sizeHint().width();
    const int availTextWidth = rect.width() - buttonWidth;

    if(option.state & QStyle::State_Selected)
        painter->fillRect(rect, option.palette.highlight());

    // draw label
    QFont font(option.font);
    QFontMetrics metrics(option.fontMetrics);
    int x = option.rect.left();
    int y = option.rect.top() + metrics.height();
    painter->drawText(x, y, metrics.elidedText(labelText(index), Qt::ElideRight, availTextWidth));

    // draw type
    x += metrics.width(index.data(Nepomuk::IdentifierModel::LabelRole).toString()) + metrics.averageCharWidth();
    font.setItalic(true);
    metrics = font;
    painter->setFont(font);
    painter->drawText(x, y, metrics.elidedText(typeText(index), Qt::ElideRight, availTextWidth));

    // draw status
    x = option.rect.left();
    y += option.fontMetrics.lineSpacing();
    const QString status = metrics.elidedText(statusText(index, painter), Qt::ElideMiddle, availTextWidth );
    painter->drawText(x, y, status);

    painter->restore();
}

QSize MergeConflictDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeView* view = qobject_cast<QTreeView*>(itemView());

    int w = 0;
    QFontMetrics metrics(option.font);
    w += metrics.width(labelText(index));
    w += metrics.averageCharWidth();
    w += metrics.width(typeText(index));
    w += s_spacing;
    w += m_sizeHelperResolveButton->sizeHint().width();
    w += s_spacing;
    w += m_sizeHelperDiscardButton->sizeHint().width();

    // trueg: this is a hack since I could not find out how to do it properly
    w += itemDepth(index) * view->indentation();

    int h = 0;
    h += 2*metrics.height() + metrics.descent();
    h = qMax(h, m_sizeHelperResolveButton->sizeHint().height());

    return QSize(w, h);
}

void MergeConflictDelegate::slotResolveButtonClicked()
{
    kDebug() << focusedIndex().data(Qt::DisplayRole);
    emit requestResourceResolve(focusedIndex().data(Nepomuk::IdentifierModel::ResourceRole).toUrl());
}

void MergeConflictDelegate::slotDiscardButtonClicked()
{
    kDebug() << focusedIndex().data(Qt::DisplayRole);
    emit requestResourceDiscard(focusedIndex().data(Nepomuk::IdentifierModel::ResourceRole).toUrl());
}

QString MergeConflictDelegate::labelText(const QModelIndex &index) const
{
    return index.data(Nepomuk::IdentifierModel::LabelRole).toString();
}

QString MergeConflictDelegate::typeText(const QModelIndex &index) const
{
    return QLatin1String("(") + Nepomuk::Types::Class(index.data(Nepomuk::IdentifierModel::TypeRole).toUrl()).label() + QLatin1String(")");
}

QString MergeConflictDelegate::statusText(const QModelIndex &index, QPainter* painter) const
{
    const KUrl identifiedResource = index.data(Nepomuk::IdentifierModel::IdentifiedResourceRole).toUrl();
    if( identifiedResource.isValid() ) {
        if(painter) {
            KColorScheme cs(QPalette::Active);
            painter->setPen(cs.foreground(KColorScheme::PositiveText).color());
        }
        return i18nc("@item:inlistbox Refers to a set of metadata that has been identified as beloging to file file at %1. %1 is a URL or part of it.",
                     "Identified as: %1", identifiedResource.pathOrUrl(KUrl::RemoveTrailingSlash));
    }
    else if( index.data(Nepomuk::IdentifierModel::DiscardedRole).toBool() ) {
        if(painter) {
            KColorScheme cs(QPalette::Active);
            painter->setPen(cs.foreground(KColorScheme::NegativeText).color());
        }
        return i18nc("@item:inlistbox The item in this row has been discarded, ie. should be ignored in the following steps",
                     "Discarded");
    }
    else {
        return i18nc("@item:inlistbox The item in this row has not been identified yet, ie. the file corresponding to it has not been chosen yet.",
                     "Not identified");
    }
}

QRect MergeConflictDelegate::itemRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeView* view = qobject_cast<QTreeView*>(itemView());
    QRect rect = option.rect;
    // trueg: this is a hack since I could not find out how to do it properly
    rect.setWidth(rect.width() - itemDepth(index)*view->indentation());
    return rect;
}

#include "mergeconflictdelegate.moc"
