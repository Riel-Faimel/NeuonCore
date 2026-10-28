#pragma once

#include "Types.h"

#include <QRectF>
#include <QString>

class QPainter;

namespace menu::editor {

// JoinPanel 组件：加入主机模式的地址/端口卡片，包含输入框、连接示意和状态行。
class JoinPanel {
public:
    static QRectF cardRect();
    static QRectF hostRect();
    static QRectF portRect();

    void draw(QPainter &painter, const QString &hostText, const QString &portText,
              bool hostFocused, bool portFocused, bool hostValid, bool portValid,
              const Pointer &pointer, float pulse) const;
};

} // namespace menu::editor
