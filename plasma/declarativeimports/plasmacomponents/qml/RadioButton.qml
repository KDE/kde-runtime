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

/**Documentanted API
Inherits:
        DualStateButton

Imports:
        QtQuick 1.0
        org.kde.plasma.core

Description:
        It is a simple Radio button which is using the plasma theme.
        To create a the Radio button group, use it in conjuction with
        ButtonRow or ButtonColumn and let the exclusive property to
        true.

Properties:
        bool checked:
        This property holds if the RadionButton is checked or not.

        bool pressed:
        This property holds if the RadioButton is being pressed.
    Read-only.

        string text:
        This property holds the text for the RadioButton label
     The default value is an empty string.

Plasma Properties:
        bool enabeld:
        This property holds if the button will be enabled for user
    interaction.
    The default value is true.

Signals:
        clicked:
        The signal is emited when the button is clicked.
**/

import QtQuick 1.0
import org.kde.plasma.core 0.1 as PlasmaCore


//FIXME: this should be round, DualStateButton shouldn't draw the shadow
DualStateButton {
    id: radioButton
    view: PlasmaCore.SvgItem {
        svg: PlasmaCore.Svg {
            id: buttonSvg
            imagePath: "widgets/actionbutton"
        }
        elementId: "normal"
        width: theme.defaultFont.mSize.height + 6
        height: width

        PlasmaCore.SvgItem {
            svg: PlasmaCore.Svg {
                id: checkmarkSvg
                imagePath: "widgets/checkmarks"
            }
            elementId: "radiobutton"
            opacity: checked ? 1 : 0
            anchors {
                fill: parent
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

    shadow: RoundShadow {}
}