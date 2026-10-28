pragma ComponentBehavior: Bound
import QtQuick 2.15

// Reusable six-pass glow text, matching Paint::glowText():
// offsets (-3,0) (3,0) (0,-3) (0,3) (-2,-2) (2,2), 0.45 shade, 0.42 alpha.
Item {
    id: root
    property string text: ""
    property color color: "#f4fbff"
    property color glowColor: Qt.rgba(color.r * 0.45, color.g * 0.45,
                                      color.b * 0.45, 1.0)
    property int pixelSize: 24
    property string family: "Microsoft YaHei"
    property bool bold: false
    property real glowOpacity: 0.42
    implicitWidth: mainText.implicitWidth + 16
    implicitHeight: mainText.implicitHeight + 16

    Repeater {
        model: [
            Qt.point(-3, 0), Qt.point(3, 0), Qt.point(0, -3),
            Qt.point(0, 3), Qt.point(-2, -2), Qt.point(2, 2)
        ]

        Text {
            required property var modelData
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: modelData.x
            anchors.verticalCenterOffset: modelData.y
            text: root.text
            color: root.glowColor
            opacity: root.glowOpacity
            renderType: Text.NativeRendering
            font.family: root.family
            font.pixelSize: root.pixelSize
            font.bold: root.bold
        }
    }

    Text {
        id: mainText
        anchors.centerIn: parent
        text: root.text
        color: root.color
        renderType: Text.NativeRendering
        font.family: root.family
        font.pixelSize: root.pixelSize
        font.bold: root.bold
    }
}
