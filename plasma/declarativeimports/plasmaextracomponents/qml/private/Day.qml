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

Item {
    property int day: 0;
    property string dayType: "";
    property string dayStatus: "active";

    signal clicked(int clickedDay);

    PlasmaCore.SvgItem {
        id: backgroundSvg;

        anchors.fill: parent;
        elementId: dayStatus;
        svg: calendarSvg;
    }

    PlasmaCore.SvgItem {
        anchors.fill: parent;
        elementId: dayTypeToElementId(dayType);
        visible: (dayTypeToElementId(dayType) != "") ? true : false;
        svg: calendarSvg;
    }

    Text {
        anchors.centerIn: parent;
        text: "" + day + "";
    }

    MouseArea {
        anchors.fill: parent;
        
        hoverEnabled: true;

        onClicked: parent.clicked(day);
        onEntered: backgroundSvg.elementId = "hoverHighlight";
        onExited: backgroundSvg.elementId = dayStatus;
    }

    function dayTypeToElementId(dT)
    {
        if (dT == "Today"){
            return "today";

        }else if (dT == "Selected"){
            return "selected";

        }else if (dT == "PublicHoliday"){
            return "red";

        }else if (dT == "Holiday"){
            return "green";

        }else{
            return "";
        }
    }
}

