#pragma once

#include "Math.h"

#include <QColor>
#include <QCursor>
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>

namespace base {

// Paint 组件：颜色、字体、文字排版和基本绘制原语。
class Paint {
public:
    static void initialize();

    // 全局鼠标光标：与局内准星同一套语言，系统默认箭头不再出现。
    static const QCursor &cursor();

    static const QColor Ink;
    static const QColor Text;
    static const QColor Muted;
    static const QColor Dim;
    static const QColor Cyan;
    static const QColor CyanSoft;
    static const QColor Magenta;
    static const QColor Coral;
    static const QColor Amber;
    static const QColor Violet;
    static const QColor Green;
    static const QColor White;
    static const QColor Red;
    static const QColor Panel;
    static const QColor Black;

    static const QFont &tinyFont();
    static const QFont &smallFont();
    static const QFont &bodyFont();
    static const QFont &mediumFont();
    static const QFont &largeFont();
    static const QFont &bigFont();
    static const QFont &titleFont();
    static const QFont &hugeFont();
    static const QFont &techTinyFont();
    static const QFont &techSmallFont();
    static const QFont &techFont();
    static const QFont &techLargeFont();

    static QColor shade(const QColor &color, float factor);
    static QColor withAlpha(const QColor &color, int alpha);

    static void fillCircle(QPainter &painter, const Math::Vec2 &center,
                           float radius, const QColor &color);
    static void strokeCircle(QPainter &painter, const Math::Vec2 &center,
                             float radius, const QColor &color, float width = 1.0f);
    static void strokeLine(QPainter &painter, const Math::Vec2 &start,
                           const Math::Vec2 &end, const QColor &color, float width = 1.0f);
    static void arcRadians(QPainter &painter, const Math::Vec2 &center, float radius,
                           float startRadians, float spanRadians,
                           const QColor &color, float width = 1.0f);
    static void polygon(QPainter &painter, const QPolygonF &points,
                        const QColor &fill, const QColor &outline, float width = 1.0f);
    static void glow(QPainter &glowPainter, const Math::Vec2 &pos, float radius,
                     const QColor &color, float intensity = 1.0f);

    static QRectF textBounds(const QFont &font, const QString &value,
                             const Math::Vec2 &pos, const QString &anchor = "topleft");
    static void text(QPainter &painter, const QFont &font, const QString &value,
                     const QColor &color, const Math::Vec2 &pos,
                     const QString &anchor = "topleft", int alpha = 255);
    static void shadowText(QPainter &painter, const QFont &font, const QString &value,
                           const QColor &color, const Math::Vec2 &pos,
                           const QString &anchor = "center", int alpha = 255,
                           const QColor &shadow = QColor(0, 0, 0));
    static void glowText(QPainter &painter, const QFont &font, const QString &value,
                         const QColor &color, const Math::Vec2 &pos,
                         const QString &anchor = "center", int alpha = 255);
    static QStringList wrapText(const QString &value, const QFont &font, float maxWidth);

    static void bar(QPainter &painter, const QRectF &rect, float ratio,
                    const QColor &color, float ghost = -1.0f,
                    const QColor &background = QColor(18, 30, 45));
    static void panel(QPainter &painter, const QRectF &rect, const QColor &border,
                      int alpha = 220, int radius = 12,
                      const QColor &fill = Paint::Panel);

private:
    static bool sFontsReady;
    static QFont sTiny;
    static QFont sSmall;
    static QFont sBody;
    static QFont sMedium;
    static QFont sLarge;
    static QFont sBig;
    static QFont sTitle;
    static QFont sHuge;
    static QFont sTechTiny;
    static QFont sTechSmall;
    static QFont sTech;
    static QFont sTechLarge;
};

} // namespace base
