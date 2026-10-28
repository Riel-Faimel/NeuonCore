import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property string title: "无法连接服务器"
    property string detail: ""
    property string endpoint: ""
    property bool retryable: true
    property bool retryRequested: false
    property bool backRequested: false

    Rectangle {
        anchors.fill: parent
        color: "#050b14"
        opacity: 0.96
    }

    Rectangle {
        width: 760
        height: 430
        anchors.centerIn: parent
        radius: 14
        color: "#07111f"
        border.width: 1
        border.color: "#ff5d7a"

        Column {
            anchors.centerIn: parent
            spacing: 18

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.title
                color: "#ff5d7a"
                font.pixelSize: 38
                font.bold: true
                font.family: "Microsoft YaHei"
            }

            Text {
                width: 640
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: root.detail
                color: "#e8f4ff"
                font.pixelSize: 18
                font.family: "Microsoft YaHei"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.endpoint
                color: "#8fb6c8"
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 28

                Rectangle {
                    width: 210
                    height: 52
                    radius: 9
                    visible: root.retryable
                    color: retryArea.containsMouse ? "#123c4b" : "#0a1e2c"
                    border.width: 1
                    border.color: "#5ee7df"
                    Text {
                        anchors.centerIn: parent
                        text: "ENTER  重试"
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
                    width: 210
                    height: 52
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
