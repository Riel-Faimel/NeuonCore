#pragma once

#include "Types.h"

#include <QRectF>
#include <QString>

class QPainter;

namespace menu::editor {

// Actions 组件：台本摘要与底部四个按钮（新增/删除/部署/返回）的固定版式。
// 它只负责布局和绘制，按钮按下后的状态迁移仍由 Editor 处理。
class Actions {
public:
    QRectF addWaveRect() const;
    QRectF removeWaveRect() const;
    QRectF deployRect() const;
    QRectF backRect() const;

    void draw(QPainter &painter, Mode mode, int waveCount, int totalEnemies,
              const QString &hostText, const QString &portText, bool startValid,
              const Pointer &pointer) const;
};

} // namespace menu::editor
