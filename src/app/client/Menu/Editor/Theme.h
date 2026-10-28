#pragma once

#include "GameConfig.h"
#include "Types.h"

#include <QColor>
#include <QFont>
#include <QRectF>
#include <QString>

class QPainter;

namespace menu::editor {

// Theme 组件：编辑器固定 1280x720 画布的版式常量，以及全局共享的
// 强调色、字体和键帽/按钮/面板/输入框绘制。它不持有任何编辑器状态，
// 只把项目现有 Base 绘制语言收敛成一套可复用的小工具。
class Theme {
public:
    // ----- 版式（固定 1280x720 逻辑画布） -----
    static constexpr float Left = 90.0f;
    static constexpr float Right = float(GameConfig::ScreenWidth) - 90.0f;
    static constexpr float WaveColumn = 92.0f;
    static constexpr float HeaderY = 236.0f;
    static constexpr float RowsTop = 252.0f;
    static constexpr float RowHeight = 21.0f;
    static constexpr float TabsTop = 94.0f;
    static constexpr float TabWidth = 216.0f;
    static constexpr float TabHeight = 44.0f;
    static constexpr float TabGap = 16.0f;
    static constexpr float ActionsTop = 178.0f;
    static constexpr float ActionsHeight = 36.0f;

    static constexpr float PaletteTop = 570.0f;
    static constexpr float PaletteHeight = 96.0f;
    static constexpr float PaletteChipWidth = 138.0f;
    static constexpr float PaletteChipHeight = 32.0f;
    static constexpr float PaletteGap = 9.0f;
    static constexpr float PaletteChipsLeft = Left + 218.0f;

    // ----- 强调色与字体 -----
    static QColor accent(Mode mode);
    static QColor tabAccent(int index);

    static const QFont &tinyFont();
    static const QFont &smallFont();
    static const QFont &bodyFont();
    static const QFont &mediumFont();
    static const QFont &largeFont();
    static const QFont &bigFont();
    static const QFont &techTinyFont();
    static const QFont &techSmallFont();
    static const QFont &techFont();

    // ----- 绘制助手 -----
    static void panel(QPainter &painter, const QRectF &rect, const QColor &border,
                      int alpha, int radius, const QColor &fill);
    static void keycap(QPainter &painter, const QRectF &rect, const QString &text,
                       const QColor &border, const QColor &textColor,
                       int textAlpha = 255, float radius = 5.0f);
    static void button(QPainter &painter, const QRectF &rect, const QString &key,
                       const QString &label, const QColor &tint, bool primary,
                       bool enabled, const Pointer &pointer);
    static void textField(QPainter &painter, const QRectF &rect, const QString &text,
                          const QString &placeholder, bool focused, bool valid,
                          const QColor &tint, const Pointer &pointer, float pulse);
};

} // namespace menu::editor
