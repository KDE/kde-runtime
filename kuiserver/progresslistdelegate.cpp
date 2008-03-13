/**
  * This file is part of the KDE project
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "progresslistdelegate.h"
#include "progresslistdelegate_p.h"
#include "progresslistmodel.h"

#include <QApplication>
#include <QPushButton>
#include <QPainter>
#include <QStyleOptionProgressBarV2>
#include <QHash>
#include <QFontMetrics>
#include <QListView>
#include <QHBoxLayout>

#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>

#define MIN_CONTENT_PIXELS 100

QString ProgressListDelegate::Private::getApplicationName(const QModelIndex &index) const
{
    return index.model()->data(index, ApplicationName).toString();
}

QString ProgressListDelegate::Private::getIcon(const QModelIndex &index) const
{
    return index.model()->data(index, Icon).toString();
}

QString ProgressListDelegate::Private::getSizeTotals(const QModelIndex &index) const
{
    return index.model()->data(index, SizeTotals).toString();
}

QString ProgressListDelegate::Private::getSizeProcessed(const QModelIndex &index) const
{
    return index.model()->data(index, SizeProcessed).toString();
}

qlonglong ProgressListDelegate::Private::getTimeTotals(const QModelIndex &index) const
{
    return index.model()->data(index, TimeTotals).toLongLong();
}

qlonglong ProgressListDelegate::Private::getTimeProcessed(const QModelIndex &index) const
{
    return index.model()->data(index, TimeElapsed).toLongLong();
}

QString ProgressListDelegate::Private::getSpeed(const QModelIndex &index) const
{
    return index.model()->data(index, Speed).toString();
}

int ProgressListDelegate::Private::getPercent(const QModelIndex &index) const
{
    return index.model()->data(index, Percent).toInt();
}

QString ProgressListDelegate::Private::getMessage(const QModelIndex &index) const
{
    return index.model()->data(index, Message).toString();
}

QStyleOptionProgressBarV2 *ProgressListDelegate::Private::getProgressBar(const QModelIndex &index) const
{
    const ProgressListModel *progressListModel = static_cast<const ProgressListModel*>(index.model());

    return progressListModel->progressBar(index);
}

int ProgressListDelegate::Private::getCurrentLeftMargin(int fontHeight) const
{
    return leftMargin + separatorPixels + fontHeight;
}

ProgressListDelegate::ProgressListDelegate(QObject *parent, QListView *listView)
    : QItemDelegate(parent)
    , d(new Private(parent, listView))
{
}

ProgressListDelegate::~ProgressListDelegate()
{
    delete d;
}

void ProgressListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QFontMetrics fontMetrics = painter->fontMetrics();
    int textHeight = fontMetrics.height();

    int coordY = d->separatorPixels + option.rect.top();

    KIcon iconToShow(d->getIcon(index));

    QColor unselectedTextColor = option.palette.text().color();
    QColor selectedTextColor = option.palette.highlightedText().color();
    QPen currentPen = painter->pen();
    QPen unselectedPen = QPen(currentPen);
    QPen selectedPen = QPen(currentPen);

    unselectedPen.setColor(unselectedTextColor);
    selectedPen.setColor(selectedTextColor);

    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(selectedPen);
    }
    else
    {
        painter->setPen(unselectedPen);
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRect canvas = option.rect;
    int iconWidth = canvas.height() - d->leftMargin - d->rightMargin;
    int iconHeight = iconWidth;
    d->iconWidth = iconWidth;

    painter->setOpacity(0.25);

    painter->drawPixmap(option.rect.right() - iconWidth - d->rightMargin, coordY, iconToShow.pixmap(iconWidth, iconHeight));

    painter->translate(d->leftMargin + option.rect.left(), d->separatorPixels + qMin(fontMetrics.width(d->getApplicationName(index)) / 2, (option.rect.height() / 2) - d->separatorPixels) + (iconHeight / 2) + canvas.top());
    painter->rotate(270);

    QRect appNameRect(0, 0, qMin(fontMetrics.width(d->getApplicationName(index)), option.rect.height() - d->separatorPixels * 2), textHeight);

    painter->drawText(appNameRect, Qt::AlignLeft, fontMetrics.elidedText(d->getApplicationName(index), Qt::ElideRight, option.rect.height() - d->separatorPixels * 2));

    painter->resetMatrix();

    painter->setOpacity(1);

    if (!d->getMessage(index).isEmpty())
    {
        QString textToShow = fontMetrics.elidedText(d->getMessage(index), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);

        textHeight = fontMetrics.size(Qt::TextSingleLine, textToShow).height();

        painter->drawText(d->getCurrentLeftMargin(textHeight) + option.rect.left(), coordY, fontMetrics.width(textToShow), textHeight, Qt::AlignLeft, textToShow);

        coordY += d->separatorPixels + textHeight;
    }

    if (!d->getSizeProcessed(index).isEmpty() || !d->getSizeTotals(index).isEmpty() || !d->getSpeed(index).isEmpty())
    {
        QString textToShow;
        if (!d->getSizeTotals(index).isEmpty() && !d->getSpeed(index).isEmpty())
            textToShow = fontMetrics.elidedText(i18n("%1 of %2 processed at %3/s", d->getSizeProcessed(index), d->getSizeTotals(index), d->getSpeed(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);
        else if (!d->getSizeTotals(index).isEmpty() && d->getSpeed(index).isEmpty())
            textToShow = fontMetrics.elidedText(i18n("%1 of %2 processed", d->getSizeProcessed(index), d->getSizeTotals(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);
        else if (d->getSizeTotals(index).isEmpty() && !d->getSpeed(index).isEmpty())
            textToShow = fontMetrics.elidedText(i18n("%1 processed at %2/s", d->getSizeProcessed(index), d->getSpeed(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);
        else
            textToShow = fontMetrics.elidedText(i18n("%1 processed", d->getSizeProcessed(index)), Qt::ElideRight, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin);

        textHeight = fontMetrics.size(Qt::TextSingleLine, textToShow).height();

        painter->drawText(d->getCurrentLeftMargin(textHeight) + option.rect.left(), coordY, fontMetrics.width(textToShow), textHeight, Qt::AlignLeft, textToShow);

        coordY += d->separatorPixels + textHeight;
    }

    if (d->getPercent(index) > -1)
    {
        QStyleOptionProgressBarV2 *progressBarModel = d->getProgressBar(index);

        progressBarModel->rect = QRect(d->getCurrentLeftMargin(textHeight) + option.rect.left(), coordY, canvas.width() - d->getCurrentLeftMargin(textHeight) - d->rightMargin - iconWidth - d->separatorPixels, d->progressBarHeight);

        QApplication::style()->drawControl(QStyle::CE_ProgressBar, progressBarModel, painter);
    }

    painter->restore();
}

QSize ProgressListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QFontMetrics fontMetrics = option.fontMetrics;

    int itemHeight = d->separatorPixels;
    int itemWidth = d->leftMargin + d->rightMargin + d->iconWidth + d->separatorPixels * 2 +
                    fontMetrics.height();

    int textSize = fontMetrics.height() + d->separatorPixels;

    if (!d->getMessage(index).isEmpty())
    {
        textSize = fontMetrics.size(Qt::TextSingleLine, d->getMessage(index)).height() + d->separatorPixels;
        itemHeight += textSize;
    }

    if (!d->getSizeProcessed(index).isEmpty() || !d->getSpeed(index).isEmpty() ||
        !d->getSizeTotals(index).isEmpty())
    {
        textSize = fontMetrics.size(Qt::TextSingleLine, d->getSizeProcessed(index)).height() + d->separatorPixels;
        itemHeight += textSize;
    }

    if (d->getPercent(index) > -1)
        itemHeight += d->progressBarHeight + d->separatorPixels;

    if (d->editorHeight > 0)
        itemHeight += d->editorHeight + d->separatorPixels;

    if (itemHeight + d->separatorPixels >= d->minimumItemHeight)
        itemHeight += d->separatorPixels;
    else
        itemHeight = d->minimumItemHeight;

    return QSize(itemWidth + MIN_CONTENT_PIXELS, itemHeight);
}

void ProgressListDelegate::setSeparatorPixels(int separatorPixels)
{
    d->separatorPixels = separatorPixels;
}

void ProgressListDelegate::setLeftMargin(int leftMargin)
{
    d->leftMargin = leftMargin;
}

void ProgressListDelegate::setRightMargin(int rightMargin)
{
    d->rightMargin = rightMargin;
}

void ProgressListDelegate::setProgressBarHeight(int progressBarHeight)
{
    d->progressBarHeight = progressBarHeight;
}

void ProgressListDelegate::setMinimumItemHeight(int minimumItemHeight)
{
    d->minimumItemHeight = minimumItemHeight;
}

void ProgressListDelegate::setMinimumContentWidth(int minimumContentWidth)
{
    d->minimumContentWidth = minimumContentWidth;
}

void ProgressListDelegate::setEditorHeight(int editorHeight)
{
    d->editorHeight = editorHeight;
}

#include "progresslistdelegate.moc"
#include "progresslistdelegate_p.moc"
