/***************************************************************************
 *   Copyright 2013 Marco Martin <mart@kde.org            >                *
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

#include "units.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QtGlobal>
#include <cmath>

Units::Units (QObject *parent)
    : QObject(parent)
{
    //dividing by 25.4 gives pixels per one millimeter
    m_gridUnit = QApplication::desktop()->physicalDpiY()/25.4;
}

Units::~Units()
{
}

qreal Units::gridUnit() const
{
    return m_gridUnit;
}

qreal Units::dp(qreal value) const
{
    //Usual "default" is 96 dpi
    const qreal ratio = m_gridUnit / (96/25.4);
    if (value <= 2.0) {
        return qRound(value * floor(ratio));
    } else {
        return qRound(value * ratio);
    }
}

qreal Units::gu(qreal value) const
{
    return qRound(m_gridUnit * value);
}

#include "units.moc"

