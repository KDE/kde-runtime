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
*   GNU Library General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

import QtQuick 1.0
import org.kde.plasma.core 0.1 as PlasmaCore

/**
 * Private base component for several public components. Any element documented here
 * can be used in components inheriting from DualStateButton.
 */
Item {
    id: dualButton

    // Common API
    /**
     * If the button is checked, its checked property is true; otherwise false. The property is false by default.
     */
    property bool checked

    /**
     * type:bool
     * If the button is pressed, its pressed property is true.
     * @see clicked
     */
    property alias pressed: mouseArea.pressed

    /**
     * Emitted when the user clicked a mouse button over the button (or
     * tapped on the touch screen)
     */
    signal clicked()

    // Plasma API
    /**
     * The text is shown beside the button. By default text is an empty string.
     */
    property alias text: label.text // TODO: Not yet part of the common API
    property alias view: surfaceLoader.sourceComponent
    property alias shadow: shadowLoader.sourceComponent

    width: surfaceLoader.width + label.paintedWidth
    height: theme.defaultFont.mSize.height*1.6
    // TODO: needs to define if there will be specific graphics for
    //     disabled buttons
    opacity: dualButton.enabled ? 1.0 : 0.5

    function released() {
        if (dualButton.enabled) {
            dualButton.checked = !dualButton.checked;
            dualButton.clicked();
        }
    }

    Keys.onReleased: {
        if(event.key == Qt.Key_Space ||
           event.key == Qt.Key_Return)
            released();
    }

    Loader {
        id: shadowLoader
        anchors.fill: surfaceLoader
        state: (dualButton.enabled && (dualButton.focus || mouseArea.containsMouse)) ? "hover" : "shadow"
    }

    Loader {
        id: surfaceLoader

        anchors {
            verticalCenter: parent.verticalCenter
            left: text ? parent.left : undefined
            horizontalCenter: text ? undefined : parent.horizontalCenter
        }
    }

    Text {
        id: label

        text: dualButton.text
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: surfaceLoader.right
            right: parent.right
            //FIXME: see how this margin will be set
            leftMargin: height/4
        }
        color: theme.textColor
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true

        onReleased: dualButton.released();
        onPressed: dualButton.forceActiveFocus();
    }
}
