pragma ComponentBehavior: Bound
import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property var cards: []
    property int selected: 0
    property bool muted: false
    property bool audioUnavailable: false
    property bool activateRequested: false
    property int requestedAction: 0
    property int requestedLevel: 0
    property bool selectRequested: false
    property int requestedSelect: 0
    property real pointerX: 640
    property real pointerY: 360
    property real pointerTime: 0
    property bool pointerVisible: false
    property real phase: 0

    readonly property color cyan: "#53e8ff"
    readonly property color ink: "#f4fbff"
    readonly property color text: "#cfe2f2"
    readonly property color mutedColor: "#7792ab"
    readonly property color panel: "#070e1b"

    NumberAnimation on phase {
        id: phaseAnim
        from: 0
        to: Math.PI * 2
        duration: 6000
        loops: Animation.Infinite
        running: true
    }

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
                GradientStop { position: 0.55; color: "#030812" }
                GradientStop { position: 1.0; color: "#010308" }
            }
        }

        Repeater {
            model: 120

            Rectangle {
                required property int index
                x: (index * 137 + 31) % 1280
                y: (index * 83 + 17) % 720
                width: index % 5 === 0 ? 3.6 : 2.0
                height: width
                radius: width / 2
                color: root.cyan
                opacity: 0.08 + 0.10 * (0.5 + 0.5 * Math.sin(phase + index))
            }
        }

        Canvas {
            anchors.fill: parent
            opacity: 0.55
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#19384c"
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
            y: 94
            text: "SECTOR 07  //  DEEP SPACE DEFENSE GRID"
            color: "#79b0c2"
            opacity: 0.73
            font.family: "Bahnschrift"
            font.pixelSize: 12
            font.letterSpacing: 1.8
        }

        NeonText {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 142
            text: "星核协议"
            color: root.ink
            pixelSize: 92
            bold: true
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 264
            text: "N E O N   C O R E   P R O T O C O L"
            color: root.cyan
            opacity: 0.9
            font.family: "Bahnschrift"
            font.pixelSize: 18
            font.letterSpacing: 1.6
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 322
            text: "选择行动。每一次部署，都会改写星核的命运。"
            color: root.text
            opacity: 0.86
            font.family: "Microsoft YaHei"
            font.pixelSize: 19
        }

        Text {
            renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            y: 362
            text: "SELECT OPERATION"
            color: root.mutedColor
            opacity: 0.75
            font.family: "Bahnschrift"
            font.pixelSize: 14
            font.letterSpacing: 1.6
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 402
            spacing: 26

            Repeater {
                model: root.cards

                Item {
                    id: card
                    required property var modelData
                    required property int index
                    width: 360
                    height: 156

                    readonly property bool active: root.selected === index

                    Rectangle {
                        anchors.fill: parent
                        radius: 12
                        color: card.active ? "#081624" : "#060d18"
                        opacity: card.active ? 0.94 : 0.82
                        border.width: 1
                        border.color: card.active ? root.cyan : "#305b70"
                    }

                    Rectangle {
                        visible: card.active
                        anchors.fill: parent
                        anchors.margins: -7
                        radius: 16
                        color: "transparent"
                        border.width: 3
                        border.color: Qt.rgba(root.cyan.r, root.cyan.g, root.cyan.b,
                                              0.18 + 0.18 * (0.5 + 0.5 * Math.sin(phase * 2.6)))
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 24
                        y: 14
                        text: "0" + (index + 1)
                        color: card.active ? root.cyan : "#305b70"
                        font.family: "Bahnschrift"
                        font.pixelSize: 12
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 24
                        y: 42
                        text: modelData.name
                        color: card.active ? root.ink : "#bfd8e6"
                        font.family: "Microsoft YaHei"
                        font.pixelSize: 23
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 24
                        y: 78
                        text: modelData.subtitle
                        color: root.mutedColor
                        font.family: "Microsoft YaHei"
                        font.pixelSize: 16
                    }

                    Text {
                        renderType: Text.NativeRendering
                        x: 24
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 14
                        text: modelData.custom ? "SCRIPT EDITOR" : "CAMPAIGN"
                        color: card.active ? "#46c5d8" : "#305b70"
                        font.family: "Bahnschrift"
                        font.pixelSize: 11
                    }

                    Text {
                        renderType: Text.NativeRendering
                        anchors.right: parent.right
                        anchors.rightMargin: 20
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 15
                        text: modelData.custom
                              ? (card.active ? "ENTER  EDIT" : "CLICK  EDIT")
                              : (card.active ? "ENTER  MAP" : "CLICK  MAP")
                        color: card.active ? root.cyan : root.mutedColor
                        font.family: "Bahnschrift"
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
                            root.requestedAction = card.modelData.custom ? 2 : 1
                            root.requestedLevel = card.index
                            root.activateRequested = true
                        }
                    }
                }
            }
        }

        MainEntry {
            x: 860
            y: 26
            label: "战备"
            icon: "cross"
            onClicked: {
                root.requestedAction = 4
                root.requestedLevel = -1
                root.activateRequested = true
            }
        }

        MainEntry {
            x: 994
            y: 26
            label: "商店"
            icon: "crate"
            onClicked: {
                root.requestedAction = 3
                root.requestedLevel = -1
                root.activateRequested = true
            }
        }

        MainEntry {
            x: 1128
            y: 26
            label: "设置"
            icon: "gear"
            onClicked: {
                root.requestedAction = 5
                root.requestedLevel = -1
                root.activateRequested = true
            }
        }

        Text {
            renderType: Text.NativeRendering
            x: 34
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 22
            text: "WASD 移动    LMB / J 射击    SPACE 冲刺"
            color: "#698a9e"
            opacity: 0.6
            font.family: "Microsoft YaHei"
            font.pixelSize: 13
        }

        Text {
            renderType: Text.NativeRendering
            anchors.right: parent.right
            anchors.rightMargin: 34
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 22
            text: (root.selected >= 0 && root.selected < root.cards.length
                   && root.cards[root.selected].custom)
                  ? "↑ / ↓ 选择    ENTER 编辑台本    "
                  : "↑ / ↓ 选择    ENTER 开始    "
                  + (root.muted ? "M 已静音" : "M 静音")
            color: root.muted ? "#ffab4e" : "#698a9e"
            opacity: 0.6
            font.family: "Microsoft YaHei"
            font.pixelSize: 13
        }

        Text {
            renderType: Text.NativeRendering
            visible: root.audioUnavailable
            x: 34
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 46
            text: "音频设备不可用"
            color: "#ffab4e"
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
