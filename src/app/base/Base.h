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

#include <memory>

// Base 是全局模块类，组合 namespace base 的 Input / Audio / Music / Paint 组件。
// 对外仍只暴露 Base 这一组接口，组件细节不泄露给业务模块。
class Base {
public:
    Base();
    ~Base();
    Base(const Base &) = delete;
    Base &operator=(const Base &) = delete;

    static void initialize();

    // ----- 输入（转发 base::Input） -----
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

    // ----- 音频（转发 base::Audio / base::Music） -----
    void initAudio();
    void playSound(const QString &name);
    void setSoundVolume(float volume);
    float soundVolume() const;
    void setMusicVolume(float volume);
    float musicVolume() const;
    bool toggleMute();
    bool muted() const;
    bool audioEnabled() const;

    // 程序化绘制的应用图标（窗口/任务栏用），不读取外部图片。
    static QPixmap appIcon(int size);

    // ----- 颜色（来自 base::Paint） -----
    static const QColor &Ink;
    static const QColor &Text;
    static const QColor &Muted;
    static const QColor &Dim;
    static const QColor &Cyan;
    static const QColor &CyanSoft;
    static const QColor &Magenta;
    static const QColor &Coral;
    static const QColor &Amber;
    static const QColor &Violet;
    static const QColor &Green;
    static const QColor &White;
    static const QColor &Red;
    static const QColor &Panel;
    static const QColor &Black;

    // ----- 字体与绘制（转发 base::Paint） -----
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

    // 鼠标准星：全应用唯一的指针画法（局内、主菜单、结算都调用它）。
    static void reticle(QPainter &painter, const Math::Vec2 &center, float time);
    // 齿轮图标（设置入口用）。
    static void gear(QPainter &painter, const Math::Vec2 &center, float radius,
                     const QColor &color);

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
                      const QColor &fill = Base::Panel);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
