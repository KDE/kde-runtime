/****************************************************************************
**
** Copyright (C) 2011 Marco Martin  <mart@kde.org>
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 1.1
import org.kde.plasma.extras 0.1 as PlasmaExtras
import "." 0.1

CommonDialog {
    id: root
    objectName: "root"

    property string message
    property string acceptButtonText: i18n("Ok")
    property string rejectButtonText: i18n("Cancel")

    onAcceptButtonTextChanged: internal.updateButtonTexts()
    onRejectButtonTextChanged: internal.updateButtonTexts()

    onButtonClicked: {
        if (acceptButtonText && index == 0)
            accept()
        else
            reject()
    }

    content: Item {
        implicitWidth: Math.max(theme.defaultFont.mSize.width * 15,  Math.min(label.implicitWidth+12, theme.defaultFont.mSize.width * 25))

        implicitHeight: Math.min(theme.defaultFont.mSize.height * 12, label.paintedHeight + 6)


        width: implicitWidth
        height: implicitHeight

        PlasmaExtras.ScrollArea {
            anchors {
                top: parent.top
                topMargin: 6
                bottom: parent.bottom
                left: parent.left
                leftMargin: 6
                right: parent.right
            }

            Flickable {
                id: flickable
                anchors.fill: parent
                contentHeight: label.paintedHeight
                flickableDirection: Flickable.VerticalFlick
                interactive: contentHeight > height

                Label {
                    id: label
                    anchors {
                        top: parent.top
                        right: parent.right
                    }
                    width: flickable.width
                    height: paintedHeight
                    wrapMode: Text.WordWrap
                    text: root.message
                    horizontalAlignment: lineCount > 1 ? Text.AlignLeft : Text.AlignHCenter
                }
            }
        }
    }

    QtObject {
        id: internal

        function updateButtonTexts() {
            var newButtonTexts = []
            if (acceptButtonText)
                newButtonTexts.push(acceptButtonText)
            if (rejectButtonText)
                newButtonTexts.push(rejectButtonText)
            root.buttonTexts = newButtonTexts
        }
    }
}
