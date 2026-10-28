#pragma once

#include "Types.h"

#include <QRectF>

class QPainter;

namespace menu::editor {

// Tabs 组件：顶部 F1/F2/F3 三个模式选项卡，负责固定版式与选中/悬停绘制。
class Tabs {
public:
    static constexpr int Count = 3;

    QRectF rect(int index) const;
    void draw(QPainter &painter, Mode mode, const Pointer &pointer, float pulse) const;
};

} // namespace menu::editor
