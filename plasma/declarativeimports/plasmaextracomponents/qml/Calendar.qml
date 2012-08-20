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
import org.kde.plasma.core 0.1 as PlasmaCore;
import org.kde.plasma.components 0.1 as PlasmaComponents;
import org.kde.locale 0.1 as Locale

Item {
    property variant selectedDate;
    property variant calendarSystem: Locale.CalendarSystem { }


    PlasmaComponents.ToolButton {
        id: prev;
        flat: true;
        text: "<";
        anchors {
            left: parent.left;
            top: parent.top;
        }
        width: 24;
        height: 24;
    }

    PlasmaComponents.ToolButton {
        id: monthButton;
        height: 24;

        anchors {
            left: prev.right;
            top: parent.top;
        }
    }


    PlasmaComponents.ToolButton {
        id: yearButton;
        height: 24;

        anchors {
            right: yearButton.right;
            left: parent.left;
            top: parent.top;
        }
    }


    PlasmaComponents.ToolButton {
        id: next;
        flat: true;
        text: ">";
        anchors {
            right: parent.right;
            top: parent.top;
        }
        width: 24;
        height: 24;
    }

    CalendarTable {
        id: calendarTable;
        anchors {
            left: parent.left;
            right: parent.right;
            top: prev.bottom;
            bottom: dateField.top;
        }
    }

    PlasmaComponents.ToolButton {
        id: currentDateButton;
        text: "#";
        width: 24;
        height: 24;

        anchors {
            left: parent.left;
            bottom: parent.bottom;
        }
    }

    PlasmaComponents.TextField {
        id: dateField;

        anchors {
            left: currentDateButton.right;
            bottom: parent.bottom;
        }
    }

    PlasmaComponents.TextField {
        id: weekField;

        anchors {
            left: dateField.right;
            right: parent.right;
            bottom: parent.bottom;
        }

    }

    function setDate(newDate){
	selectedDate = newDate;
	console.log(selectedDate);
        monthButton.text = calendarSystem.monthName(calendarSystem.month(selectedDate), calendarSystem.year(selectedDate), Locale.CalendarSystem.LongName);
        yearButton.text = calendarSystem.year(selectedDate);
	//TODO: here the API is different
        //dateField.text = calendarSystem.formatDate(selectedDate, Locale.CalendarSystem.ShortDate);
	weekField.text = calendarSystem.week(selectedDate, Locale.CalendarSystem.IsoWeekFormat);
    }

    Component.onCompleted: {
        setDate(new Date());
    }
}

