#pragma once

#include "GameConfig.h"
#include "Math.h"
#include "Games/Upgrades/Upgrades.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>

#include <array>
#include <vector>

class Arena;
class Games;

namespace games {

class Waves;

// Renderer 组件：Games 的局内画面表现（一局一个实例，由 Games 持有）。
// 它不画标题和结算——那些是 menu::*；也不跨局保存状态，切局即重建。
// 工具：BaseService（绘制原语/输入）、RandomService（抖动/粒子）、
//       Arena / Waves / Upgrades 的只读状态。
// 封装：背景、星空、发光合成、屏幕震动/闪屏、波次横幅、HUD、Boss 血条、
//       强化三选一、暂停遮罩、胜利彩带。
// 暴露：渲染一帧、frame、表现反馈（shake/flash/banner）、强化卡命中测试。
class Renderer {
public:
    Renderer();

    void initialize();
    void update(float dt);
    void render(const Games &game, const Arena &arena, const Waves &waves,
                const Upgrades &upgrades);
    const QImage &frame() const { return frame_; }

    void shake(float amount);
    void flash(const QColor &color, int alpha);
    void showBanner(const QString &title, const QString &subtitle, float duration,
                    const QColor &color, const QString &kicker = QStringLiteral("INCOMING SIGNAL"));
    void burstConfetti();

    int upgradeIndexAt(const Math::Vec2 &pos) const;

private:
    friend class ::SelfTest;

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
    void drawHUD(QPainter &painter, const Arena &arena, const Waves &waves);
    void drawBossBar(QPainter &painter, const Arena &arena);
    void drawWaveBanner(QPainter &painter);
    void drawPlayHints(QPainter &painter);
    void drawUpgradeOverlay(QPainter &painter, const Upgrades &upgrades,
                            const Waves &waves);
    void drawUpgradeCard(QPainter &painter, int index,
                         const Upgrades::Offer &offer, const QRectF &rect, bool hovered);
    void drawPauseOverlay(QPainter &painter, const Arena &arena, const Waves &waves);
    QRectF cardRect(int index) const;

    QImage background_;
    QImage vignette_;
    QImage world_;
    QImage glow_;
    QImage frame_;
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
