#pragma once

#include "Debug.h"

#include "GameConfig.h"
#include "Math.h"
#include "Simulation/Upgrades/Upgrades.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>

#include <array>
#include <vector>

namespace client {
class GameView;
}

namespace games {

// Renderer 组件：局内画面表现（一局一个实例，由 Games 持有）。
// 它不画标题和结算——那些是 menu::*；也不跨局保存状态，切局即重建。
// 它只读一份 client::GameView（快照重建的只读世界），不再直接读 Games / Arena /
// Waves / Upgrades 的模拟对象；震屏/闪屏/横幅等表现状态仍由它自己持有。
class Renderer {
public:
    Renderer();

    void initialize();
    void update(float dt);
    void render(const client::GameView &view);
    const QImage &frame() const { return frame_; }

    void shake(float amount);
    void flash(const QColor &color, int alpha);
    void showBanner(const QString &title, const QString &subtitle, float duration,
                    const QColor &color, const QString &kicker = QStringLiteral("INCOMING SIGNAL"));
    void burstConfetti();

    int upgradeIndexAt(const Math::Vec2 &pos) const;

private:
    DebugInfo(friend class ::SelfTest;)

    struct Star {
        Math::Vec2 pos;
        float speed = 2.0f;
        float phase = 0.0f;
        int radius = 1;
        QColor tint;
    };

    struct Spark {
        Math::Vec2 pos;
        Math::Vec2 vel;
        float life = 0.0f;
        float maxLife = 1.0f;
        float radius = 2.0f;
        float drag = 0.8f;
        float gravity = 0.0f;
        QColor color;
    };

    void buildBackground();
    void buildVignette();
    void updateStars(float dt);
    void updateSparks(float dt);
    void drawStars(QPainter &world, QPainter &glow) const;
    void drawSparks(QPainter &world, QPainter &glow) const;
    // 世界通道（背景/星空/实体/粒子，不含字体）与辉光通道写不同目标，可并行。
    void drawWorldPass(const client::GameView &view, int offsetX, int offsetY);
    void drawCombinedPass(const client::GameView &view, int offsetX, int offsetY);
    void drawGlowPass(const client::GameView &view);
    void drawUi(QPainter &painter, const client::GameView &view);
    void compositeFrame(int offsetX, int offsetY);
    void drawHUD(QPainter &painter, const client::GameView &view);
    void drawBossBar(QPainter &painter, const client::GameView &view);
    void drawWaveBanner(QPainter &painter);
    void drawPlayHints(QPainter &painter);
    void drawUpgradeOverlay(QPainter &painter, const client::GameView &view);
    void drawUpgradeCard(QPainter &painter, int index,
                         const Upgrades::Offer &offer, const QRectF &rect, bool hovered);
    void drawPauseOverlay(QPainter &painter, const client::GameView &view);
    QRectF cardRect(int index) const;

    QImage background_;
    QImage vignette_;
    QImage glow_;
    QImage frame_;
    // 反通道的 1×1 哑目标：世界通道里忽略辉光、辉光通道里忽略世界绘制。
    QImage dummyWorld_;
    QImage dummyGlow_;
    std::vector<Star> stars_;
    std::vector<Spark> sparks_;
    float time_ = 0.0f;

    float shake_ = 0.0f;
    QColor flashColor_;
    float flashAlpha_ = 0.0f;

    QString bannerTitle_;
    QString bannerSub_;
    QString bannerKicker_ = QStringLiteral("INCOMING SIGNAL");
    QColor bannerColor_;
    float bannerTimer_ = 0.0f;
    float bannerDuration_ = 1.0f;
};

} // namespace games
