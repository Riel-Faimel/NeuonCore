pragma ComponentBehavior: Bound
import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property int score: 0
    property int mines: 0
    property int price: 60
    property bool buyRequested: false
    property bool backRequested: false
    property real pointerX: 640
    property real pointerY: 360
    property real pointerTime: 0
    property bool pointerVisible: false

    readonly property color amber: "#ffab4e"
    readonly property color ink: "#f4fbff"
    readonly property color text: "#c6dbea"
    readonly property color muted: "#7792ab"
    readonly property bool affordable: score >= price

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
                    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, 720); ctx.stroke()
                }
                for (let y = 0; y < 720; y += 80) {
                    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(1280, y); ctx.stroke()
                }
            }
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 112
            text: "SUPPLY DEPOT  //  ARMORY"
            color: "#79b0c2"
            opacity: 0.76
            font.family: "Bahnschrift"
            font.pixelSize: 12
            font.letterSpacing: 1.7
        }

        NeonText {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 142
            text: "补给站"
            color: root.ink
            pixelSize: 44
            bold: true
        }

        Canvas {
            x: 516
            y: 158
            width: 44
            height: 44
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#ffab4e"
                ctx.lineWidth = 2
                ctx.strokeRect(6, 10, 32, 24)
                ctx.beginPath()
                ctx.moveTo(6, 10); ctx.lineTo(38, 34)
                ctx.moveTo(38, 10); ctx.lineTo(6, 34)
                ctx.moveTo(6, 22); ctx.lineTo(38, 22)
                ctx.stroke()
            }
        }

        Text {
            renderType: Text.NativeRendering
            anchors.right: parent.right
            anchors.rightMargin: 34
            y: 22
            text: "积分  " + root.score
            color: root.amber
            font.family: "Bahnschrift"
            font.pixelSize: 14
            font.letterSpacing: 1.0
        }

        Rectangle {
            x: 300
            y: 214
            width: 680
            height: 290
            radius: 12
            color: "#081420"
            opacity: 0.95
            border.width: 1
            border.color: root.amber

            Rectangle {
                x: 24
                y: 30
                width: 64
                height: 64
                radius: 32
                color: "#28ffab4e"
                border.width: 1
                border.color: root.amber
            }

            Text {
                renderType: Text.NativeRendering
                x: 24
                y: 49
                width: 64
                horizontalAlignment: Text.AlignHCenter
                text: "MINE"
                color: root.amber
                font.family: "Bahnschrift"
                font.pixelSize: 12
            }

            Text {
                renderType: Text.NativeRendering
                x: 112
                y: 24
                text: "震荡地雷"
                color: root.ink
                font.family: "Microsoft YaHei"
                font.pixelSize: 23
            }

            Text {
                renderType: Text.NativeRendering
                x: 113
                y: 58
                text: "IMPACT MINE  //  CONSUMABLE"
                color: root.muted
                opacity: 0.75
                font.family: "Bahnschrift"
                font.pixelSize: 11
                font.letterSpacing: 1.0
            }

            Text {
                renderType: Text.NativeRendering
                anchors.right: parent.right
                anchors.rightMargin: 34
                y: 30
                text: root.price + " 积分"
                color: root.amber
                font.family: "Bahnschrift"
                font.pixelSize: 18
            }

            Text {
                renderType: Text.NativeRendering
                x: 40
                y: 118
                width: 600
                wrapMode: Text.WordWrap
                text: "右键在光标处布置，敌人进入感应范围即引爆，对范围内敌人造成大量伤害。一次性消耗，购买后进入库存。"
                color: root.text
                opacity: 0.88
                font.family: "Microsoft YaHei"
                font.pixelSize: 18
                lineHeight: 1.35
            }

            Rectangle {
                x: 40
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 90
                width: 600
                height: 1
                color: "#5b3b1f"
            }

            Text {
                renderType: Text.NativeRendering
                x: 40
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 56
                text: "持有  ×" + root.mines
                color: root.text
                font.family: "Microsoft YaHei"
                font.pixelSize: 16
            }
        }

        Rectangle {
            x: 700
            y: 430
            width: 240
            height: 42
            radius: 9
            color: root.affordable ? "#0a1822" : "#080e16"
            border.width: 1
            border.color: root.affordable ? root.amber : "#465460"
            opacity: root.affordable ? 0.95 : 0.8

            Text {
                renderType: Text.NativeRendering
                anchors.centerIn: parent
                text: root.affordable ? "购买  " + root.price : "积分不足"
                color: root.affordable ? root.amber : root.muted
                font.family: "Bahnschrift"
                font.pixelSize: 14
                font.letterSpacing: 1.0
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.affordable
                cursorShape: Qt.BlankCursor
                onClicked: root.buyRequested = true
            }
        }

        Rectangle {
            x: 550
            y: 600
            width: 180
            height: 42
            radius: 9
            color: "#070e19"
            border.width: 1
            border.color: "#41687c"

            Text {
                renderType: Text.NativeRendering
                anchors.centerIn: parent
                text: "ESC  返回"
                color: root.text
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
            text: "ENTER / 点击购买    ESC 返回    M 静音"
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
