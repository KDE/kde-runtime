/*
 *   Copyright (C) 2010 by Anselmo Lacerda Silveira de Melo <anselmolsm@gmail.com>
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

Rectangle {
    id: test
    width: 400
    height: 400
    color: "tomato"

    Column {
        x: 10
        anchors.fill: parent
        spacing: 5

        FlashingLabel {
            id: label
            font: "Times"
        }

        Row {
            id: busyRow

            Repeater {
                model: 8
                BusyWidget {
                    width: 50
                    height: 50
                }
            }
        }

        PushButton {
            text: "Ok"
            onClicked: {print("Clicked!"); scrollBar.value=35}
        }
        ScrollBar {
            id: scrollBar
        }
        ScrollBar {
            id: scrollBarV
            orientation: Qt.Vertical
        }
        
    }

    Component.onCompleted: {
        label.flash("I am a FlashingLabel!!!");
    }
}
