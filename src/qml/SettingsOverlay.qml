pragma ComponentBehavior: Bound
import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property real sound: 0.68
    property real music: 0.45
    property int selected: 0
    property bool volumeRequested: false
    property int requestedRow: 0
    property real requestedValue: 0
    property bool selectRequested: false
    property int requestedSelect: 0
    property bool backRequested: false
    property real pointerX: 640
    property real pointerY: 360
    property real pointerTime: 0
    property bool pointerVisible: false

    readonly property color cyan: "#53e8ff"
    readonly property color ink: "#f4fbff"
    readonly property color text: "#cfe2f2"
    readonly property color muted: "#7792ab"

    Item {
        id: stage
        width: 1280
        height: 720
        scale: Math.min(root.width / 1280, root.height / 720)
        transformOrigin: Item.TopLeft
        x: (root.width - width * scale) / 2
        y: (root.height - height * scale) / 2

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#050f1c" }
                GradientStop { position: 0.6; color: "#030812" }
                GradientStop { position: 1.0; color: "#010308" }
            }
        }

        Canvas {
            anchors.fill: parent
            opacity: 0.55
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#173246"
                ctx.lineWidth = 1
                for (let x = 0; x < 1280; x += 80) {
                    ctx.beginPath()
                    ctx.moveTo(x, 0)
                    ctx.lineTo(x, 720)
                    ctx.stroke()
                }
                for (let y = 0; y < 720; y += 80) {
                    ctx.beginPath()
                    ctx.moveTo(0, y)
                    ctx.lineTo(1280, y)
                    ctx.stroke()
                }
            }
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 120
            text: "AUDIO CONFIGURATION  //  SETTINGS"
            color: "#79b0c2"
            opacity: 0.76
            font.family: "Bahnschrift"
            font.pixelSize: 12
            font.letterSpacing: 1.7
        }

        NeonText {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 154
            text: "设置"
            color: root.ink
            pixelSize: 44
            bold: true
        }

        Canvas {
            x: 536
            y: 169
            width: 34
            height: 34
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#53e8ff"
                ctx.lineWidth = 2
                const cx = 17
                const cy = 17
                ctx.beginPath()
                ctx.arc(cx, cy, 6, 0, Math.PI * 2)
                ctx.stroke()
                for (let i = 0; i < 8; ++i) {
                    const a = i * Math.PI / 4
                    ctx.beginPath()
                    ctx.moveTo(cx + Math.cos(a) * 6, cy + Math.sin(a) * 6)
                    ctx.lineTo(cx + Math.cos(a) * 13, cy + Math.sin(a) * 13)
                    ctx.stroke()
                }
            }
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 222
            text: "音量调整会立即生效并保存"
            color: root.muted
            opacity: 0.82
            font.family: "Microsoft YaHei"
            font.pixelSize: 16
        }

        Repeater {
            model: [
                { title: "音效音量  //  SFX", note: "射击、命中、界面提示", value: root.sound },
                { title: "背景音乐  //  BGM", note: "music 目录轮转播放", value: root.music }
            ]

            Item {
                id: row
                required property var modelData
                required property int index
                x: 300
                y: 286 + index * 100
                width: 680
                height: 80

                readonly property bool active: root.selected === index
                readonly property real value: index === 0 ? root.sound : root.music

                Rectangle {
                    anchors.fill: parent
                    radius: 12
                    color: row.active ? "#081624" : "#060d18"
                    opacity: row.active ? 0.94 : 0.82
                    border.width: 1
                    border.color: row.active ? root.cyan : "#305b70"
                }

                Text {
                    renderType: Text.NativeRendering
                    x: 28
                    y: 20
                    text: row.modelData.title
                    color: row.active ? root.ink : "#bfd8e6"
                    font.family: "Bahnschrift"
                    font.pixelSize: 14
                    font.letterSpacing: 1.0
                }

                Text {
                    renderType: Text.NativeRendering
                    x: 28
                    y: 44
                    text: row.modelData.note
                    color: root.muted
                    opacity: 0.78
                    font.family: "Microsoft YaHei"
                    font.pixelSize: 13
                }

                Rectangle {
                    id: track
                    z: 2
                    x: 266
                    y: 32
                    width: 356
                    height: 16
                    radius: 8
                    color: "#101e2c"

                    Rectangle {
                        width: parent.width * row.value
                        height: parent.height
                        radius: 8
                        color: row.active ? root.cyan : "#2798b5"
                    }

                    Rectangle {
                        x: parent.width * row.value - 7
                        y: -4
                        width: 14
                        height: 24
                        radius: 7
                        color: row.active ? root.ink : "#2798b5"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.BlankCursor
                        onPressed: {
                            root.selected = row.index
                            root.requestedSelect = row.index
                            root.selectRequested = true
                            root.requestedRow = row.index
                            root.requestedValue = Math.max(0, Math.min(1, mouse.x / width))
                            root.volumeRequested = true
                        }
                        onPositionChanged: {
                            if (!pressed) {
                                return
                            }
                            root.requestedRow = row.index
                            root.requestedValue = Math.max(0, Math.min(1, mouse.x / width))
                            root.volumeRequested = true
                        }
                    }
                }

                Text {
                    renderType: Text.NativeRendering
                    x: 650
                    anchors.verticalCenter: parent.verticalCenter
                    text: Math.round(row.value * 100) + "%"
                    color: row.active ? root.cyan : root.text
                    font.family: "Bahnschrift"
                    font.pixelSize: 18
                }

                MouseArea {
                    z: 1
                    anchors.fill: parent
                    anchors.rightMargin: 120
                    cursorShape: Qt.BlankCursor
                    onClicked: {
                        root.selected = row.index
                        root.requestedSelect = row.index
                        root.selectRequested = true
                    }
                }
            }
        }

        Rectangle {
            id: back
            x: 550
            y: 600
            width: 180
            height: 42
            radius: 9
            color: "#07121f"
            border.width: 1
            border.color: root.cyan

            Text {
                renderType: Text.NativeRendering
                anchors.centerIn: parent
                text: "ESC  返回"
                color: root.cyan
                font.family: "Bahnschrift"
                font.pixelSize: 14
                font.letterSpacing: 1.0
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.BlankCursor
                onClicked: root.backRequested = true
            }
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 676
            text: "↑ ↓ 选择    ← → 调整    ESC 返回    M 静音"
            color: "#698a9e"
            opacity: 0.63
            font.family: "Microsoft YaHei"
            font.pixelSize: 13
        }

        Reticle {
            anchors.fill: parent
            pointerX: root.pointerX
            pointerY: root.pointerY
            pointerTime: root.pointerTime
            pointerVisible: root.pointerVisible
        }
    }
}
