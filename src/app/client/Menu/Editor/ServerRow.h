#pragma once

#include "Types.h"

#include <QRectF>
#include <QString>

class QPainter;

namespace menu::editor {

// ServerRow 组件：局域网主机模式下，波次表格下方的服务器端口行。
class ServerRow {
public:
    static QRectF rowRect(int waveCount);
    static QRectF portRect(int waveCount);

    void draw(QPainter &painter, int waveCount, const QString &portText,
              bool focused, bool valid, const Pointer &pointer, float pulse) const;
};

} // namespace menu::editor
