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

    Rectangle {
        anchors.fill: parent
        color: "#050b14"
        opacity: 0.92
    }

    Rectangle {
        width: 900
        height: 500
        anchors.centerIn: parent
        radius: 14
        color: "#07111f"
        border.width: 1
        border.color: root.victory ? "#5ee7df" : "#ff5d7a"

        Column {
            anchors.centerIn: parent
            spacing: 16

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.victory ? "区域已肃清" : "防线失守"
                color: root.victory ? "#5ee7df" : "#ff5d7a"
                font.pixelSize: 46
                font.bold: true
                font.family: "Microsoft YaHei"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.levelName
                color: "#e8f4ff"
                font.pixelSize: 20
                font.family: "Microsoft YaHei"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "得分 " + root.score + "   ·   击破 " + root.kills
                      + "   ·   波次 " + root.wave + "/" + root.totalWaves
                      + "   ·   " + root.runTime.toFixed(1) + "s"
                color: "#8fb6c8"
                font.pixelSize: 18
                font.family: "Microsoft YaHei"
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 28

                Rectangle {
                    width: 220
                    height: 54
                    radius: 9
                    color: retryArea.containsMouse ? "#123c4b" : "#0a1e2c"
                    border.width: 1
                    border.color: "#5ee7df"
                    Text {
                        anchors.centerIn: parent
                        text: "ENTER  重新部署"
                        color: "#e8f4ff"
                        font.pixelSize: 18
                        font.family: "Microsoft YaHei"
                    }
                    MouseArea {
                        id: retryArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.retryRequested = true
                    }
                }

                Rectangle {
                    width: 220
                    height: 54
                    radius: 9
                    color: backArea.containsMouse ? "#2a1820" : "#150b10"
                    border.width: 1
                    border.color: "#7d4a57"
                    Text {
                        anchors.centerIn: parent
                        text: "ESC  返回标题"
                        color: "#e8f4ff"
                        font.pixelSize: 18
                        font.family: "Microsoft YaHei"
                    }
                    MouseArea {
                        id: backArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.backRequested = true
                    }
                }
            }
        }
    }
}
