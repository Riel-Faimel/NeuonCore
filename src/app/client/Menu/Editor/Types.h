#pragma once

#include "Math.h"

#include <QPointF>
#include <QRectF>
#include <QString>

namespace menu::editor {

// 编辑器三种模式：单机 / 局域网主机 / 加入主机。
enum class Mode {
    Single,
    Host,
    Join,
};

// 固定画布下的指针状态：所有子组件共享同一份悬停判断，避免各自复制逻辑。
struct Pointer {
    Math::Vec2 position;
    bool active = false;

    bool hovering(const QRectF &rect) const
    {
        return active && !rect.isEmpty()
               && rect.contains(QPointF(position.x, position.y));
    }
};

inline constexpr int kMaxWaves = 12;
inline constexpr int kMaxEnemyCount = 20;
inline constexpr int kKindCount = 12;  // 基础 5 种 + 7 种支援/特殊敌人

} // namespace menu::editor
