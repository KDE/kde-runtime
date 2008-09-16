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
#include <QHash>
#include <QFontMetrics>
#include <QListView>
#include <QHBoxLayout>
#include <QProgressBar>

#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>
#include <kpushbutton.h>

#define MIN_CONTENT_PIXELS 50

QString ProgressListDelegate::Private::getApplicationName(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::ApplicationName).toString();
}

QString ProgressListDelegate::Private::getIcon(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::Icon).toString();
}

QString ProgressListDelegate::Private::getSizeTotals(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::SizeTotals).toString();
}

QString ProgressListDelegate::Private::getSizeProcessed(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::SizeProcessed).toString();
}

qlonglong ProgressListDelegate::Private::getTimeTotals(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::TimeTotals).toLongLong();
}

qlonglong ProgressListDelegate::Private::getTimeProcessed(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::TimeElapsed).toLongLong();
}

QString ProgressListDelegate::Private::getSpeed(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::Speed).toString();
}

int ProgressListDelegate::Private::getPercent(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::Percent).toInt();
}

QString ProgressListDelegate::Private::getMessage(const QModelIndex &index) const
{
    return index.model()->data(index, ProgressListModel::Message).toString();
}

int ProgressListDelegate::Private::getCurrentLeftMargin(int fontHeight) const
{
    return leftMargin + separatorPixels + fontHeight;
}

ProgressListDelegate::ProgressListDelegate(QObject *parent, QListView *listView)
    : KWidgetItemDelegate(listView, parent)
    , d(new Private(listView))
{
}

ProgressListDelegate::~ProgressListDelegate()
{
    delete d;
}

void ProgressListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    if (!index.isValid()) {
        return;
    }

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

        coordY += textHeight;
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

        coordY += textHeight;
    }

    painter->restore();

    KWidgetItemDelegate::paintWidgets(painter, option, index);
}

QSize ProgressListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QFontMetrics fontMetrics = option.fontMetrics;

    int itemHeight = d->separatorPixels;
    int itemWidth = d->leftMargin + d->rightMargin + d->iconWidth + d->separatorPixels * 2 +
                    fontMetrics.height();

    int textSize = fontMetrics.height();

    if (!d->getMessage(index).isEmpty())
    {
        textSize = fontMetrics.size(Qt::TextSingleLine, d->getMessage(index)).height();
        itemHeight += textSize;
    }

    if (!d->getSizeProcessed(index).isEmpty() || !d->getSpeed(index).isEmpty() ||
        !d->getSizeTotals(index).isEmpty())
    {
        textSize = fontMetrics.size(Qt::TextSingleLine, d->getSizeProcessed(index)).height();
        itemHeight += textSize;
    }

    if (d->getPercent(index)) {
        itemHeight += d->progressBar->sizeHint().height();
    }

    if (d->editorHeight > 0)
        itemHeight += d->editorHeight;

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

QList<QWidget*> ProgressListDelegate::createItemWidgets() const
{
    QList<QWidget*> widgetList;

    KPushButton *pauseResumeButton = new KPushButton(KIcon("media-playback-pause"), i18n("Pause"));
    KPushButton *cancelButton = new KPushButton(KIcon("media-playback-stop"), i18n("Cancel"));
    QProgressBar *progressBar = new QProgressBar();

    connect(pauseResumeButton, SIGNAL(clicked(bool)), this, SLOT(slotPauseResumeClicked()));
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(slotCancelClicked()));

    setBlockedEventTypes(pauseResumeButton, QList<QEvent::Type>() << QEvent::MouseButtonPress
                            << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick);
    setBlockedEventTypes(cancelButton, QList<QEvent::Type>() << QEvent::MouseButtonPress
                            << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick);

    widgetList << pauseResumeButton << cancelButton << progressBar;

    return widgetList;
}

void ProgressListDelegate::updateItemWidgets(const QList<QWidget*> widgets,
                                             const QStyleOptionViewItem &option,
                                             const QPersistentModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    KPushButton *cancelButton = static_cast<KPushButton*>(widgets[1]);
    KPushButton *pauseResumeButton = static_cast<KPushButton*>(widgets[0]);
    QProgressBar *progressBar = static_cast<QProgressBar*>(widgets[2]);

    KJob::Capabilities capabilities = (KJob::Capabilities) index.model()->data(index, ProgressListModel::Capabilities).toInt();
    cancelButton->setEnabled(capabilities & KJob::Killable);
    pauseResumeButton->setEnabled(capabilities & KJob::Suspendable);

    JobInfo::State state = (JobInfo::State) index.model()->data(index, ProgressListModel::State).toInt();
    switch (state) {
        case JobInfo::Running:
            pauseResumeButton->setText(i18n("Pause"));
            pauseResumeButton->setIcon(KIcon("media-playback-pause"));
            break;
        case JobInfo::Suspended:
            pauseResumeButton->setText(i18n("Resume"));
            pauseResumeButton->setIcon(KIcon("media-playback-start"));
            break;
    }

    progressBar->setValue(d->getPercent(index));

    QSize cancelButtonSizeHint = cancelButton->sizeHint();
    cancelButton->resize(cancelButtonSizeHint);
    cancelButton->move(option.rect.width() - d->separatorPixels - cancelButtonSizeHint.width(), option.rect.height() - d->separatorPixels - cancelButtonSizeHint.height());

    QSize pauseResumeButtonSizeHint = pauseResumeButton->sizeHint();
    pauseResumeButton->resize(pauseResumeButtonSizeHint);
    pauseResumeButton->move(option.rect.width() - d->separatorPixels * 2 - pauseResumeButtonSizeHint.width() - cancelButtonSizeHint.width(), option.rect.height() - d->separatorPixels - pauseResumeButtonSizeHint.height());

    QFontMetrics fm(QApplication::font());
    QSize progressBarSizeHint = progressBar->sizeHint();
    progressBar->resize(QSize(option.rect.width() - d->getCurrentLeftMargin(fm.height()) - d->rightMargin, progressBarSizeHint.height()));
    progressBar->move(d->getCurrentLeftMargin(fm.height()), option.rect.height() - d->separatorPixels * 2 - pauseResumeButtonSizeHint.height() - progressBarSizeHint.height());
}

void ProgressListDelegate::slotPauseResumeClicked()
{
    const QModelIndex index = focusedIndex();
    UIServer::JobView *jobView = index.model()->data(index, ProgressListModel::JobViewRole).value<UIServer::JobView*>();
    JobInfo::State state = (JobInfo::State) index.model()->data(index, ProgressListModel::State).toInt();
    if (jobView) {
        switch (state) {
            case JobInfo::Running:
                emit jobView->suspendRequested();
                break;
            case JobInfo::Suspended:
                emit jobView->resumeRequested();
                break;
            default:
                Q_ASSERT(0); // this point should have never been reached
                break;
        }
    }
}

void ProgressListDelegate::slotCancelClicked()
{
    const QModelIndex index = focusedIndex();
    UIServer::JobView *jobView = index.model()->data(index, ProgressListModel::JobViewRole).value<UIServer::JobView*>();
    if (jobView) {
        emit jobView->cancelRequested();
    }
}

#include "progresslistdelegate.moc"
