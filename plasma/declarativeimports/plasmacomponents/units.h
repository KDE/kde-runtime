/***************************************************************************
 *   Copyright 2013 Marco Martin <mart@kde.org>                            *
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

#ifndef UNITS_H
#define UNITS_H

#include <QObject>



class Units : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal gridUnit READ gridUnit NOTIFY gridUnitChanged())

public:
    Units(QObject *parent = 0);
    ~Units();

    qreal gridUnit() const;
    Q_INVOKABLE qreal dp(qreal value) const;
    Q_INVOKABLE qreal gu(qreal value) const;


Q_SIGNALS:
    void gridUnitChanged();

private:
    int m_gridUnit;
};

#endif //UNITS_H

