#pragma once

#include "Bullet.h"

#include <QColor>

#include <array>

namespace arena {

// 标准弹：直线飞行，可带穿透和暴击；行为和旧的实体版一致。
class StandardBullet : public Bullet {
public:
    StandardBullet(const Math::Vec2 &position, const Math::Vec2 &velocity, float damage,
                   const QColor &color, Standpoint standpoint, float radius = 3.0f,
                   int pierce = 0, float critChance = 0.0f);

    // 对象池复用：重置全部状态，供 Bullets 回收后再发射。
    void reset(const Math::Vec2 &position, const Math::Vec2 &velocity, float damage,
               const QColor &color, Standpoint standpoint, float radius = 3.0f,
               int pierce = 0, float critChance = 0.0f);

    void advance(float dt) override;
    void draw(QPainter &world, QPainter &glow) const override;
    bool hit(const Context &ctx, Fighter &target) override;

    bool alive() const override { return alive_; }
    const Math::Vec2 &prev() const override { return prev_; }
    const Math::Vec2 &position() const override { return pos_; }
    float radius() const override { return radius_; }
    Standpoint standpoint() const override { return standpoint_; }

    // 快照提取用的只读字段（Bullet 接口保持最小，不把样式塞进去）。
    const Math::Vec2 &velocity() const { return vel_; }
    const QColor &color() const { return color_; }
    float damage() const { return damage_; }

private:
    // 穿透上限 5 级 ⇒ 一颗弹最多命中 6 个目标；固定数组避免每颗弹都分配哈希表。
    static constexpr int MaxHitTargets = 8;

    Math::Vec2 pos_;
    Math::Vec2 prev_;
    Math::Vec2 vel_;
    float damage_ = 0.0f;
    QColor color_;
    Standpoint standpoint_ = Standpoint::Player;
    bool alive_ = true;
    float radius_ = 3.0f;
    int pierce_ = 0;
    float critChance_ = 0.0f;
    float life_ = 1.6f;
    std::array<int, MaxHitTargets> hitTargets_{};
    int hitTargetCount_ = 0;
};

} // namespace arena
