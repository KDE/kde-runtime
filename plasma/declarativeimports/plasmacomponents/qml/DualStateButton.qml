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

/**Documentanted API
Inherits:
        Item

Imports:
        QtQuick 1.0
        org.kde.plasma.core

Description:
        This component is used as base component for RadioButton and
        CheckBox. This is a private component, use it carefully.

Properties:
        bool checked:
        This property holds if the RadionButton is checked or not.
    The default value is false.

        bool pressed:
        This property holds if the RadioButton is being pressed.
    Read-only.

        string text:
        This property holds the text for the button label
    The default value is an empty string.

Plasma Properties:
        bool enabeld:
        This property holds if the button will be enabled for user
    interaction.
    The default value is true.

        QtObject theme:
        This property holds the object with all the layout parameters in
    the current theme.

        Component view:
        This property holds the component used to render the button
    visualization.

        Component shadow:
        This property holds the component used to render the button shadow.
    The shadow is used to give the visual feedback for users, displaying if
    it is hovered or pressed.

Signals:
        clicked:
        The signal is emited when the button is clicked.
**/

import QtQuick 1.0
import org.kde.plasma.core 0.1 as PlasmaCore

Item {
    id: dualButton

    // Common API
    property bool checked
    property alias pressed: mouseArea.pressed

    signal clicked()

    // Plasma API
    property alias text: label.text // TODO: Not yet part of the common API
    property alias view: surfaceLoader.sourceComponent
    property alias shadow: shadowLoader.sourceComponent

    width: surfaceLoader.width + label.paintedWidth
    height: theme.defaultFont.mSize.height*1.6
    // TODO: needs to define if there will be specific graphics for
    //     disabled buttons
    opacity: dualButton.enabled ? 1.0 : 0.5

    function entered() {
        if (dualButton.enabled) {
            shadowLoader.state = "hover"
        }
    }

    function released() {
        if (dualButton.enabled) {
            dualButton.checked = !dualButton.checked;
            dualButton.clicked();
        }
    }

    Keys.onSpacePressed: entered();
    Keys.onReturnPressed: entered();
    Keys.onReleased: {
        if(event.key == Qt.Key_Space ||
           event.key == Qt.Key_Return)
            released();
    }

    Loader {
        id: shadowLoader
        anchors.fill: surfaceLoader
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
        onEntered: dualButton.entered();
        onPressed: dualButton.forceActiveFocus();
        onExited: {
            shadowLoader.state = "shadow"
        }
    }
}

