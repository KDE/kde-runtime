/*******************************************************************
* statuswidget.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/
#include "statuswidget.h"

#include <QtGui/QApplication>
#include <QtGui/QSizePolicy>
#include <QtGui/QProgressBar>
#include <QtGui/QHBoxLayout>

StatusWidget::StatusWidget(QWidget * parent) :
        QStackedWidget(parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    
    //Main layout
    m_statusPage = new QWidget(this);
    m_busyPage = new QWidget(this);

    addWidget(m_statusPage);
    addWidget(m_busyPage);

    //Status widget
    m_statusLabel = new WrapLabel();
    m_statusLabel->setOpenExternalLinks(true);
    m_statusLabel->setTextFormat(Qt::RichText);
    //m_statusLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
    
    QHBoxLayout * statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(2);
    m_statusPage->setLayout(statusLayout);

    statusLayout->addWidget(m_statusLabel);

    //Busy widget
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);
    m_progressBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    
    m_busyLabel = new WrapLabel();
    //m_busyLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
    
    QHBoxLayout * busyLayout = new QHBoxLayout();
    busyLayout->setContentsMargins(0, 0, 0, 0);
    busyLayout->setSpacing(2);
    m_busyPage->setLayout(busyLayout);

    busyLayout->addWidget(m_busyLabel);
    busyLayout->addWidget(m_progressBar);
    busyLayout->setAlignment(m_progressBar,Qt::AlignVCenter);
}

void StatusWidget::setBusy(QString busyMessage)
{
    m_statusLabel->clear();
    m_busyLabel->setText(busyMessage);
    setCurrentWidget(m_busyPage);
    //setBusyCursor();
}

void StatusWidget::setIdle(QString idleMessage)
{
    m_busyLabel->clear();
    m_statusLabel->setText(idleMessage);
    setCurrentWidget(m_statusPage);
    //setIdleCursor();
}

void StatusWidget::addCustomStatusWidget(QWidget * widget)
{
    QVBoxLayout * statusLayout = static_cast<QVBoxLayout*>(m_statusPage->layout());

    statusLayout->addWidget(widget);
    statusLayout->setAlignment(widget,Qt::AlignVCenter);
    widget->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed));
}

void StatusWidget::setBusyCursor()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
}

void StatusWidget::setIdleCursor()
{
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
}
