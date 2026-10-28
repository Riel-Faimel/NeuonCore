pragma ComponentBehavior: Bound
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



    readonly property color accent: "#ff4858"
    readonly property color titleColor: "#ff8b96"
    readonly property color ink: "#e9f6ff"
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
            GradientStop { position: 0.0; color: "#18080f" }
            GradientStop { position: 0.55; color: "#09050c" }
            GradientStop { position: 1.0; color: "#020207" }
        }
    }

    // 旧 Failure 的稀疏红点星尘。
    Repeater {
        model: 90

        Rectangle {
            id: dust
            required property int index
            x: (dust.index * 149 + 53) % 1280
            y: (dust.index * 89 + 31) % 720
            width: 2
            height: 2
            radius: 1
            color: "#e45c70"
            opacity: 0.10 + (dust.index % 4) * 0.035
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 132
        width: 500
        height: 1
        color: root.accent
        opacity: 0.78
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 158
        text: "CONNECTION FAILED  //  NO AUTHORITATIVE SERVER"
        color: root.accent
        font.family: "Bahnschrift"
        font.pixelSize: 14
        font.letterSpacing: 1.8
        opacity: 0.9
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 208
        text: root.title
        color: root.titleColor
        font.family: "Microsoft YaHei"
        font.pixelSize: 44
        font.bold: true
        style: Text.Outline
        styleColor: "#40ff7785"
        opacity: 0.98
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 275
        text: "本机没有连接到可用的房间服务器。"
        color: "#dcbfc8"
        font.family: "Microsoft YaHei"
        font.pixelSize: 19
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 312
        width: 820
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: root.detail
        color: root.muted
        font.family: "Microsoft YaHei"
        font.pixelSize: 16
    }

    Rectangle {
        x: 320
        y: 362
        width: 640
        height: 92
        radius: 10
        border.width: 1
        border.color: "#82ff4858"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#f0180c14" }
            GradientStop { position: 1.0; color: "#f009060c" }
        }

        Rectangle {
            x: 24
            y: 32
            width: 6
            height: 6
            radius: 3
            color: root.accent
        }

        Text {
            x: 40
            y: 14
            text: "TARGET  目标地址"
            color: root.muted
            font.family: "Bahnschrift"
            font.pixelSize: 12
            font.letterSpacing: 1.0
        }

        Text {
            x: 40
            y: 38
            text: root.endpoint.length > 0 ? root.endpoint : "未指定服务器"
            color: "#f4fbff"
            font.family: "Bahnschrift"
            font.pixelSize: 18
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            width: 300
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
            text: "确认主机已启动、地址端口正确；若房主已解散，请让房主重新开房。"
            color: root.muted
            font.family: "Microsoft YaHei"
            font.pixelSize: 13
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 508
        spacing: 40

        Rectangle {
            width: 250
            height: 48
            radius: 9
            visible: root.retryable
            color: retryArea.containsMouse ? "#0b2433" : "#07121f"
            border.width: 1
            border.color: "#53e8ff"

            Text {
                anchors.centerIn: parent
                text: "ENTER  重试连接"
                color: "#53e8ff"
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
            color: backArea.containsMouse ? "#0b1d2b" : "#070e19"
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
                id: backArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.backRequested = true
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 592
        text: "这不是对局结算：房间从未进入战斗，或主机已关闭房间。"
        color: "#9a7a84"
        font.family: "Microsoft YaHei"
        font.pixelSize: 13
        opacity: 0.9
    }
    }
}
