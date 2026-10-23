#pragma once

#include "Math.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>

#include <memory>

// Base 是唯一的底层 I/O 基础设施接口。
// 对外只包含：键鼠输入、程序化音频、颜色/字体/文字排版、基本绘制原语。
// QSoundEffect、WAV 生成、QFontDatabase、输入状态等细节都藏在 Base.cpp 的 Impl 中。
class Base {
public:
    Base();
    ~Base();
    Base(const Base &) = delete;
    Base &operator=(const Base &) = delete;

    static void initialize();

    // ----- 输入 -----
    void keyPress(int key);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mouseReleaseLeft();
    void clear();
    Math::Vec2 movementAxis() const;
    bool shootingHeld() const;
    bool mouseDown() const;
    bool mouseActive() const;
    Math::Vec2 mousePosition() const;
    void setMouseActive(bool active);

    // ----- 音频 -----
    void initAudio();
    void playSound(const QString &name);
    bool toggleMute();
    bool muted() const;
    bool audioEnabled() const;

    // ----- 颜色 -----
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

    // ----- 字体与排版 -----
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

    // ----- 绘制原语 -----
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
                      const QColor &fill = Base::Panel);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

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
