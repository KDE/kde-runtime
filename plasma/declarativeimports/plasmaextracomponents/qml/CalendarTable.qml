/***************************************************************************
 *   Copyright (C) 2012 by Davide Bettio <davide.bettio@kdemail.net>       *
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

import QtQuick 1.0;
import org.kde.plasma.core 0.1 as PlasmaCore
import "private/"

Item {
    id: calendar;
    
    property variant daysModel;
    property variant weekDaysModel;
    property variant weeksModel;
    
    PlasmaCore.Svg {
        id: calendarSvg;
        imagePath: "widgets/calendar";
    }
    
    PlasmaCore.SvgItem {
        id: weekDaysHeader;

        anchors {
            left: parent.left;
            top: parent.top;
        }
        width: parent.width;
        height: ~~(parent.height / (daysGrid.rows + 1)) - 1;

        svg: calendarSvg;
        elementId: "weekDayHeader";
        
        Row {
            anchors {
                fill: parent;
                leftMargin: ~~(parent.width / (daysGrid.columns + 1)) - 1; 
                right: parent.right;
            }

            spacing: 1;
            
            Repeater {
                id: weekDays;
                model: weekDaysModel;
                WeekDay {
                    width: ~~(daysGrid.width / daysGrid.columns) - 1;
                    height: ~~(calendar.height / (daysGrid.rows + 1)) - 1;
                    weekDay: modelData;
                }
            }
        }
    }
    
    PlasmaCore.SvgItem {
        id: weeksColumn;

        anchors {
            left: parent.left;
            top: weekDaysHeader.bottom;
            bottom: parent.bottom;
        }
        width: ~~(parent.width / (daysGrid.columns + 1)) - 1;
        
        svg: calendarSvg;
        elementId: "weeksColumn";        
        
        Column {
            spacing: 1;
            anchors.fill: parent;
            
            Repeater {
                id: weeks;

                model: weeksModel;

                Week {
                    week: index;
                    width: ~~(calendar.width / (daysGrid.columns + 1)) - 1;
                    height: ~~(daysGrid.height / daysGrid.rows) - 1;
                }
            }
        }
    }
    
    Grid {
        id: daysGrid;
        
        anchors {
            left: weeksColumn.right;
            top: weekDaysHeader.bottom;
            right: parent.right;
            bottom: parent.bottom;
        }
        
        spacing: 1;
        columns: 7;
        rows: 6;
        
        Repeater {
            id: days;

            model: daysModel;

            Day {
                width: ~~(daysGrid.width / daysGrid.columns) - 1;
                height: ~~(daysGrid.height / daysGrid.rows) - 1;

                day: index;
            }
        }
    }
}
