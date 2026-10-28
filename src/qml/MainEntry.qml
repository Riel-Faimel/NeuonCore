import QtQuick 2.15

Item {
    id: root
    width: 122
    height: 38

    property string label: ""
    property string icon: "cross"
    property color accent: "#53e8ff"
    property color restColor: "#96c8dc"
    signal clicked()

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: "#060e19"
        opacity: 0.92
        border.width: 1
        border.color: mouse.containsMouse ? root.accent : "#305b70"
    }

    Canvas {
        x: 14
        y: 9
        width: 20
        height: 20
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = mouse.containsMouse ? root.accent : root.restColor
            ctx.fillStyle = ctx.strokeStyle
            ctx.lineWidth = 1.5
            const cx = 10
            const cy = 10
            if (root.icon === "crate") {
                ctx.strokeRect(3, 5, 14, 11)
                ctx.beginPath()
                ctx.moveTo(3, 5)
                ctx.lineTo(17, 16)
                ctx.moveTo(17, 5)
                ctx.lineTo(3, 16)
                ctx.stroke()
            } else if (root.icon === "gear") {
                ctx.beginPath()
                ctx.arc(cx, cy, 3.2, 0, Math.PI * 2)
                ctx.stroke()
                for (let i = 0; i < 6; ++i) {
                    const a = i * Math.PI / 3
                    ctx.beginPath()
                    ctx.moveTo(cx + Math.cos(a) * 3.2, cy + Math.sin(a) * 3.2)
                    ctx.lineTo(cx + Math.cos(a) * 8.0, cy + Math.sin(a) * 8.0)
                    ctx.stroke()
                }
            } else {
                ctx.beginPath()
                ctx.arc(cx, cy, 7, 0, Math.PI * 2)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(cx - 12, cy)
                ctx.lineTo(cx - 4, cy)
                ctx.moveTo(cx + 4, cy)
                ctx.lineTo(cx + 12, cy)
                ctx.moveTo(cx, cy - 12)
                ctx.lineTo(cx, cy - 4)
                ctx.moveTo(cx, cy + 4)
                ctx.lineTo(cx, cy + 12)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(cx, cy, 2, 0, Math.PI * 2)
                ctx.fill()
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 44
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: mouse.containsMouse ? "#f4fbff" : (root.icon === "crate" ? "#e8c98e" : "#bfd8e6")
        renderType: Text.NativeRendering
        font.family: "Microsoft YaHei"
        font.pixelSize: 16
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.BlankCursor
        onClicked: root.clicked()
    }
}
