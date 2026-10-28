import QtQuick 2.15

// 与 base::Paint::reticle() 相同的准星：中心点、四向短射线、旋转外弧。
Canvas {
    id: reticle
    width: 1280
    height: 720
    visible: pointerVisible
    z: 1000
    antialiasing: true

    property real pointerX: 640
    property real pointerY: 360
    property real pointerTime: 0
    property bool pointerVisible: false

    onPointerXChanged: requestPaint()
    onPointerYChanged: requestPaint()
    onPointerTimeChanged: requestPaint()
    onPointerVisibleChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        if (!reticle.pointerVisible) {
            return
        }

        const x = reticle.pointerX
        const y = reticle.pointerY

        ctx.fillStyle = "#53e8ff"
        ctx.beginPath()
        ctx.arc(x, y, 2.0, 0, Math.PI * 2)
        ctx.fill()

        ctx.strokeStyle = "#53e8ff"
        ctx.lineWidth = 1.0
        for (let i = 0; i < 4; ++i) {
            const angle = Math.PI * 0.5 * i
            const dx = Math.cos(angle)
            const dy = Math.sin(angle)
            ctx.beginPath()
            ctx.moveTo(x + dx * 8.0, y + dy * 8.0)
            ctx.lineTo(x + dx * 15.0, y + dy * 15.0)
            ctx.stroke()
        }

        ctx.strokeStyle = "#51a9be"
        ctx.lineWidth = 1.0
        ctx.beginPath()
        const start = reticle.pointerTime * 0.8
        ctx.arc(x, y, 22.0, start, start + 1.2)
        ctx.stroke()
    }
}
