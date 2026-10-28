pragma ComponentBehavior: Bound
import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property var cards: []
    property int selected: 0
    property bool selectRequested: false
    property int requestedSelect: 0
    property bool equipRequested: false
    property int requestedEquip: 0
    property bool backRequested: false
    property real pointerX: 640
    property real pointerY: 360
    property real pointerTime: 0
    property bool pointerVisible: false

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
            opacity: 1.0
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
            y: 66
            text: "WAR PREP  //  LAUNCHER LOADOUT"
            color: "#79b0c2"
            opacity: 0.76
            font.family: "Bahnschrift"
            font.pixelSize: 12
            font.letterSpacing: 1.7
        }

        NeonText {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 94
            text: "战备"
            color: root.ink
            pixelSize: 44
            bold: true
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 158
            text: "选择发射器，默认标准弹仍然保留"
            color: root.muted
            opacity: 0.84
            font.family: "Microsoft YaHei"
            font.pixelSize: 16
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 214
            spacing: 28

            Repeater {
                model: root.cards

                Item {
                    id: card
                    required property var modelData
                    required property int index
                    width: 290
                    height: 314

                    readonly property bool active: root.selected === index
                    readonly property color accent: modelData.accent || "#53e8ff"

                    Rectangle {
                        anchors.fill: parent
                        radius: 12
                        color: card.active ? "#091826" : "#060e19"
                        opacity: card.active ? 0.94 : 0.75
                        border.width: 1
                        border.color: card.active ? card.accent : "#305b70"
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 18
                        y: 20
                        text: modelData.equipped ? "EQUIPPED" : "LAUNCHER"
                        color: modelData.equipped ? "#53e8ff" : root.muted
                        font.family: "Bahnschrift"
                        font.pixelSize: 11
                        font.letterSpacing: 1.0
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 18
                        y: 54
                        text: modelData.name
                        color: root.ink
                        font.family: "Microsoft YaHei"
                        font.pixelSize: 23
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 18
                        y: 86
                        text: modelData.subtitle
                        color: root.muted
                        opacity: 0.82
                        font.family: "Microsoft YaHei"
                        font.pixelSize: 13
                    }

                    Canvas {
                        x: 0
                        y: 120
                        width: parent.width
                        height: 80
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            const cx = width / 2
                            ctx.fillStyle = card.accent
                            ctx.beginPath()
                            ctx.arc(cx, 50, 5, 0, Math.PI * 2)
                            ctx.fill()
                            const shots = Math.max(1, 1 + (card.modelData.shots || 0))
                            for (let i = 0; i < shots; ++i) {
                                const offset = (i - (shots - 1) / 2) * 16
                                ctx.beginPath()
                                ctx.arc(cx + offset, 14, 2.6, 0, Math.PI * 2)
                                ctx.fill()
                                ctx.strokeStyle = card.accent
                                ctx.globalAlpha = 0.4
                                ctx.lineWidth = 1
                                ctx.beginPath()
                                ctx.moveTo(cx + offset, 20)
                                ctx.lineTo(cx + offset, 36)
                                ctx.stroke()
                                ctx.globalAlpha = 1.0
                            }
                        }
                    }

                    Repeater {
                        model: [
                            { label: "DAMAGE", value: "x" + (card.modelData.damage || "1.00") },
                            { label: "RATE", value: "x" + (card.modelData.rate || "1.00") },
                            { label: "SHOTS", value: "+" + (card.modelData.shots || 0) },
                            { label: "PIERCE", value: "+" + (card.modelData.pierce || 0) }
                        ]

                        Item {
                            required property var modelData
                            required property int index
                            x: 18
                            y: 204 + index * 22
                            width: card.width - 36
                            height: 22

                            Text {
                                renderType: Text.NativeRendering
                                text: modelData.label
                                color: root.muted
                                font.family: "Bahnschrift"
                                font.pixelSize: 11
                            }

                            Text {
                                renderType: Text.NativeRendering
                                anchors.right: parent.right
                                text: modelData.value
                                color: root.text
                                font.family: "Bahnschrift"
                                font.pixelSize: 13
                            }
                        }
                    }

                    Text {
                        renderType: Text.NativeRendering
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 12
                        text: modelData.equipped ? "已装备" : "ENTER / 点击装备"
                        color: modelData.equipped ? "#53e8ff" : root.muted
                        font.family: "Microsoft YaHei"
                        font.pixelSize: 13
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.BlankCursor
                        onEntered: {
                            root.selected = index
                            root.requestedSelect = index
                            root.selectRequested = true
                        }
                        onClicked: {
                            root.selected = index
                            root.requestedEquip = index
                            root.equipRequested = true
                        }
                    }
                }
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
            y: 684
            text: "← → 选择    ENTER 装备    ESC 返回"
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
