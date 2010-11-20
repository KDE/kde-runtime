import Qt 4.7
import org.kde.plasma.core 0.1 as PlasmaCore

Item {
    id: root
    width: 100; height: 100

    property bool running: true
    property string label: ""

    PlasmaCore.Theme {
        id: theme
    }

    PlasmaCore.SvgItem {
        id: widget
        width: root.width
        height: root.height
        anchors.horizontalCenter: root.horizontalCenter
        svg: PlasmaCore.Svg { imagePath: ("widgets/busywidget") }

        RotationAnimation on rotation {
            from: 0
            to: 360
            target: widget
            duration: 1500
            running: root.running
            loops: Animation.Infinite
        }

        // if you need to do anything while repainting
        // do it inside this slot
        function update() {
        }
    }

    Text {
        id: label
        text: root.label
        color: theme.textColor
        anchors.verticalCenter: root.verticalCenter
        anchors.horizontalCenter: root.horizontalCenter
    }

}
