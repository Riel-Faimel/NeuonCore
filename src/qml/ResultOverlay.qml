pragma ComponentBehavior: Bound
import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property bool victory: false
    property string levelName: ""
    property int score: 0
    property int kills: 0
    property int wave: 0
    property int totalWaves: 0
    property real runTime: 0
    property bool retryRequested: false
    property bool backRequested: false

    readonly property color accent: victory ? "#53e8ff" : "#ff4858"
    readonly property color titleColor: victory ? "#97ffdc" : "#ff7785"
    readonly property color textColor: "#cfe2f2"
    readonly property color muted: "#7792ab"

    function two(value) {
        return value < 10 ? "0" + value : "" + value
    }

    function timeText() {
        const total = Math.max(0, Math.floor(root.runTime))
        return two(Math.floor(total / 60)) + ":" + two(total % 60)
    }

    function scoreText() {
        return root.score.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",")
    }

    // 与旧 Result 覆盖层一致：直接压在局内帧上，而不是另起一张卡片。
    Rectangle {
        anchors.fill: parent
        color: "#020309"
        opacity: 0.82
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "transparent"
        border.width: 1
        border.color: "#101a25"
        opacity: 0.65
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 118
        width: 500
        height: 1
        color: root.accent
        opacity: 0.9
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 145
        text: root.victory
              ? "MISSION COMPLETE  //  CORE STABILIZED"
              : "MISSION FAILED  //  CORE SIGNAL LOST"
        color: root.accent
        font.family: "Bahnschrift"
        font.pixelSize: 14
        font.letterSpacing: 1.8
        opacity: 0.9
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 196
        text: root.victory ? "星核已稳定" : "防线失守"
        color: root.titleColor
        font.family: "Microsoft YaHei"
        font.pixelSize: 44
        font.bold: true
        style: Text.Outline
        styleColor: root.victory ? "#4097ffdc" : "#40ff7785"
        opacity: 0.98
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 267
        text: root.victory ? "你守住了最后一道光。" : "星核在黑暗中熄灭。"
        color: root.textColor
        font.family: "Microsoft YaHei"
        font.pixelSize: 19
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 302
        text: root.levelName
        color: root.muted
        font.family: "Microsoft YaHei"
        font.pixelSize: 16
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 383
        spacing: 0

        Repeater {
            model: [
                { label: "本局获得", value: root.scoreText() },
                { label: "到达波次", value: root.wave + " / " + root.totalWaves },
                { label: "击破目标", value: "" + root.kills },
                { label: "存活时间", value: root.timeText() }
            ]

            Item {
                id: statCell
                required property var modelData
                required property int index
                width: 190
                height: 100

                Rectangle {
                    visible: statCell.index > 0
                    x: 0
                    y: 4
                    width: 1
                    height: 88
                    color: "#2c4353"
                }

                Text {
                    x: (parent.width - width) / 2
                    y: 0
                    text: statCell.modelData.label
                    color: root.muted
                    font.family: "Microsoft YaHei"
                    font.pixelSize: 13
                }

                Text {
                    x: (parent.width - width) / 2
                    y: 27
                    text: statCell.modelData.value
                    color: "#f4fbff"
                    font.family: "Bahnschrift"
                    font.pixelSize: 32
                    font.bold: true
                }
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 560
        spacing: 40

        Rectangle {
            width: 250
            height: 48
            radius: 9
            color: retryArea.containsMouse ? "#0b2433" : "#07121f"
            opacity: 0.94
            border.width: 1
            border.color: root.accent

            Text {
                anchors.centerIn: parent
                text: "R  重新部署"
                color: root.accent
                font.family: "Bahnschrift"
                font.pixelSize: 14
                font.letterSpacing: 1.0
            }

            MouseArea {
                id: retryArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.retryRequested = true
            }
        }

        Rectangle {
            width: 250
            height: 48
            radius: 9
            color: menuArea.containsMouse ? "#0b1d2b" : "#070e19"
            opacity: 0.94
            border.width: 1
            border.color: "#41687c"

            Text {
                anchors.centerIn: parent
                text: "ESC  返回主菜单"
                color: "#bed7e4"
                font.family: "Bahnschrift"
                font.pixelSize: 14
                font.letterSpacing: 1.0
            }

            MouseArea {
                id: menuArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.backRequested = true
            }
        }
    }
}
