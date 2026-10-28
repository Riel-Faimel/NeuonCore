#pragma once

#include "Games/Games.h"
#include "Types.h"

#include <QColor>
#include <QRectF>

class QPainter;

namespace menu::editor {

// Inspector 组件：表格下方的检查器，三列分别是当前波次、台本概览和键鼠操作提示。
class Inspector {
public:
    QRectF rect(int waveCount, Mode mode) const;
    void draw(QPainter &painter, const games::Script &script, int row, Mode mode,
              const QColor &tint) const;
};

} // namespace menu::editor
