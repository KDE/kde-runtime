/*
*   Copyright (C) 2011 by Daker Fernandes Pinheiro <dakerfp@gmail.com>
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

/**Documented API
        Inherits:
        Item

Imports:
        QtQuick 1.0
        org.kde.plasma.core

Description:
            It is a simple progressBar which is using the plasma theme.
            Some operations take a period of time to be performed and the user needs a confirmation that the operation is still ongoing. If the user does not get any confirmation, they might suspect that they did something wrong or that the device has broken. A progress bar is one of available mechanisms for providing this reassurance to the user.

Properties:
          * real minimumValue:
            Minimum Value for the progressBar

          * real maximumValue:
            Maximum value for the progressBar

            real value:
            Current value of the progressBar

          * bool indeterminate:
            Indicates whether the operation's duration is known or not. The property can have the following values:
            true - the operation's duration is unknown, so the progress bar is animated. The value, minimum, and maximum properties are ignored.
            false - the operation's duration is known, so the progress bar is drawn to indicate progress between the minimum and maximum values.
            The default value is false.

          * int orientation:
            Orientation of the progressBar:
            Qt.Horizontal or
            Qt.Vertical
**/

import QtQuick 1.0
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.qtextracomponents 0.1

Item {
    id: progressBar

    // Common API
    property alias minimumValue: range.minimumValue
    property alias maximumValue: range.maximumValue
    property alias value: range.value
    property alias indeterminate: indeterminateAnimation.running

    // Plasma API
    property int orientation: Qt.Horizontal

    width: 100
    height: 20
    opacity: enabled ? 1.0 : 0.5

    RangeModel {
        id: range

        // default values
        minimumValue: 0.0
        maximumValue: 1.0
        value: 0

        positionAtMinimum: 0
        positionAtMaximum: contents.width
    }

    Item {
        id: contents

        property bool _isVertical: orientation == Qt.Vertical

        width: _isVertical ? progressBar.height : progressBar.width
        height: _isVertical ? progressBar.width : progressBar.height
        rotation: _isVertical ? 90 : 0
        anchors.centerIn: parent

        Timer {
            id: resizeTimer
            repeat: false
            interval: 0
            running: false
            onTriggered: {
                barFrameSvg.resizeFrame(Qt.size(Math.floor(contents.height/1.6), contents.height))
                barPixmapItem.pixmap = barFrameSvg.framePixmap()
                backgroundFrameSvg.resizeFrame(Qt.size(Math.floor(contents.height/1.6), contents.height))
                backgroundPixmapItem.pixmap = backgroundFrameSvg.framePixmap()
            }
        }
        PlasmaCore.FrameSvg {
            id: barFrameSvg
            Component.onCompleted: {
                barFrameSvg.setImagePath("widgets/bar_meter_horizontal")
                barFrameSvg.setElementPrefix("bar-active")
                resizeTimer.restart()
            }
        }
        PlasmaCore.FrameSvg {
            id: backgroundFrameSvg
            Component.onCompleted: {
                backgroundFrameSvg.setImagePath("widgets/bar_meter_horizontal")
                backgroundFrameSvg.setElementPrefix("bar-inactive")
                resizeTimer.restart()
            }
        }
        QPixmapItem {
            id: backgroundPixmapItem
            fillMode: QPixmapItem.Tile
            width: Math.floor(parent.width/(height/1.6))*Math.floor(height/1.6)
            height: parent.height
            onWidthChanged: resizeTimer.restart()
            onHeightChanged: resizeTimer.restart()
        }


        QPixmapItem {
            id: barPixmapItem
            fillMode: QPixmapItem.Tile
            width: indeterminate ? Math.floor(height/1.6)*2 : range.position
            height: contents.height

            visible: indeterminate || value > 0

            SequentialAnimation {
                id: indeterminateAnimation

                loops: Animation.Infinite

                PropertyAnimation {
                    target: barPixmapItem
                    property: "x"
                    duration: 800
                    to: 0
                }
                PropertyAnimation {
                    target: barPixmapItem
                    property: "x"
                    duration: 800
                    to: backgroundPixmapItem.width - barPixmapItem.width
                }
            }
        }
    }
}