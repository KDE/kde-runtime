/*
*   Copyright (C) 2010 by Marco Martin <mart@kde.org>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

import Qt 4.7
import org.kde.plasma.core 0.1 as PlasmaCore

    
PlasmaCore.FrameSvgItem {
    id: scrollBar
    width: 200
    height: 22

    property int minimum: 0
    property int maximum: 100
    property int value: 0

    onValueChanged: {
        if (drag.state != "dragging") {
            drag.x = (value/(maximum-minimum))*(scrollBar.width - drag.width)
        }
    }

    imagePath: "widgets/scrollbar"
    prefix: "background-horizontal"

    PlasmaCore.FrameSvgItem {
        id: drag
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        state: "normal"
        width: 32
        x: 0
        onXChanged: {
            if (state == "dragging") {
                value = (maximum - minimum)*(x/(scrollBar.width-width))
            }
        }
        imagePath: "widgets/scrollbar"
        prefix: "slider"
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            
            drag.target: parent;
            drag.axis: "XAxis"
            drag.minimumX: 0;
            drag.maximumX: scrollBar.width-drag.width;
            
            onEntered: drag.prefix = "mouseover-slider"
            onExited: drag.prefix = "slider"
            onPressed: {
                drag.prefix = "sunken-slider"
                drag.state = "dragging"
            }
            onReleased: {
                containsMouse?drag.prefix = "mouseover-slider":drag.prefix = "slider"
                drag.state = "normal"
            }
        }
    }
}


