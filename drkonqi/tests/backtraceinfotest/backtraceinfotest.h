/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BACKTRACEINFOTEST_H
#define BACKTRACEINFOTEST_H

#include <QtTest>
#include "../../backtraceinfo.h"

class BacktraceInfoTest : public QObject
{
    Q_OBJECT
    Q_ENUMS(Usefulness)

public:
    /* ### These HAVE TO match BacktraceInfo::Usefulness in value
        (but not necessarily in name), except the invalid value */
    enum Usefulness { InvalidUsefulnessValue = -1, ReallyUseful = 0, MayBeUseful=1, ProbablyUseless=2, Useless = 3 };

private slots:
    void btInfoTest_data();
    void btInfoTest();

private:
    Usefulness runBacktraceInfo(const QByteArray & data);
    void readMap(QHash<QByteArray, BacktraceInfoTest::Usefulness> & map);
};

Q_DECLARE_METATYPE(BacktraceInfoTest::Usefulness)

#endif
