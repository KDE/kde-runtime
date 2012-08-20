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
import org.kde.locale 0.1 as Locale
import "private/"

Item {
    id: calendar;
    
    property variant daysModel: ListModel { }
    property variant weekDaysModel: ListModel { }
    property variant weeksModel: ListModel { }
    property variant calendarSystem: Locale.CalendarSystem { }
    property variant selectedDate;

    property int selectedYear;
    property int selectedMonth;
    property int daysInWeek;
    property int weekDayFirstOfSelectedMonth;
    property int daysShownInPrevMonth;

    signal dayClicked(int clickedDay);
    signal dayHovered(int hoveredDay);

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
        width: ~~(parent.width / (daysGrid.columns + 1)) * (daysGrid.columns + 1) - 1;
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
            topMargin: 1;
        }
        width: ~~(parent.width / (daysGrid.columns + 1)) - 1;
        height: ~~(daysGrid.height / daysGrid.rows) * daysGrid.rows - 1;

        svg: calendarSvg;
        elementId: "weeksColumn";        
        
        Column {
            spacing: 1;
            anchors.fill: parent;
            
            Repeater {
                id: weeks;

                model: weeksModel;

                Week {
                    week: modelData;
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
            leftMargin: 1;
            topMargin: 1;
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

                day: modelData;

                onClicked: calendar.dayClicked(clickedDay);
                onHovered: calendar.dayHovered(hoveredDay);
            }
        }
    }

    function setDate(newDate){
        selectedDate = newDate;
	selectedMonth = calendarSystem.month(newDate);
	selectedYear = calendarSystem.year(newDate);
	daysInWeek = calendarSystem.daysInWeek(newDate);
	//FIX: locale binding
	//daysInSelectedMonth = calendarSystem.daysInMonth(newDate);
	//FIX: locale binding
	daysShownInPrevMonth = 0;//(weekDayFirstOfSelectedMonth - calendarSystem.weekStartDay() + daysInWeek) % daysInWeek;
        if (daysShownInPrevMonth < 1) {
	    daysShownInPrevMonth += daysInWeek;
	}
    }

    function dateFromRowColumn(weekRow, weekdayColumn){
        var cellDate = new Date();

        //FIX: locale binding
	//if (calendarSystem.setYMD(cellDate, selectedYear, selectedMonth, 1)){
            cellDate = calendarSystem.addDays(cellDate, (weekRow * daysInWeek) + weekdayColumn - daysShownInPrevMonth);
	//}

	return cellDate;
    }

    Component.onCompleted: {
        setDate(new Date());

        for (var i = 0; i < 6; i++){
	    //FIX: locale biding
            weeksModel.append({'modelData': i});
	}

        for (var i = 1; i <= 7; i++){
            weekDaysModel.append({'modelData': calendarSystem.weekDayName(i, Locale.CalendarSystem.ShortDayName)});
	}

        for (var weekRow = 0; weekRow < 6; weekRow++){
	    for (var weekdayColumn = 0; weekdayColumn < daysInWeek; weekdayColumn++){
                var cellDate = dateFromRowColumn(weekRow, weekdayColumn);
		var cellDay = calendarSystem.day(cellDate);
                daysModel.append({'modelData': cellDay});
            }
	}	
    }
}
