#pragma once

#include "Math.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>

namespace base {

// Paint 组件：颜色、字体、文字排版和基本绘制原语。
class Paint {
public:
    static void initialize();

    // 渲染通道：世界层与辉光层写不同的目标，可以并行绘制。
    // 原语只在自己所属的通道生效（All 为普通 UI/菜单绘制）。
    enum class Pass {
        All,
        World,
        Glow,
    };
    static void setPass(Pass pass);
    static Pass currentPass();

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
    static const QFont &techTinyFont();
    static const QFont &techSmallFont();
    static const QFont &techFont();

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
    // 鼠标准星：局内、主菜单、结算共用这一套画法（外弧按 time 旋转）。
    static void reticle(QPainter &painter, const Math::Vec2 &center, float time);
    // 齿轮图标（设置按钮用）：八个齿 + 中间的圆环。
    static void gear(QPainter &painter, const Math::Vec2 &center, float radius,
                     const QColor &color);
    // 补给箱图标（商店入口用）。
    static void crate(QPainter &painter, const Math::Vec2 &center, float radius,
                      const QColor &color);
    // 应用图标：程序化绘制的“星核”，按 size 直接渲染（不加载外部图片）。
    static QPixmap appIcon(int size);
    static void polygon(QPainter &painter, const QPolygonF &points,
                        const QColor &fill, const QColor &outline, float width = 1.0f);
    static void glow(QPainter &glowPainter, const Math::Vec2 &pos, float radius,
                     const QColor &color, float intensity = 1.0f);

    static void text(QPainter &painter, const QFont &font, const QString &value,
                     const QColor &color, const Math::Vec2 &pos,
                     const QString &anchor = "topleft", int alpha = 255);
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
    static QFont sTechTiny;
    static QFont sTechSmall;
    static QFont sTech;
};

} // namespace base
