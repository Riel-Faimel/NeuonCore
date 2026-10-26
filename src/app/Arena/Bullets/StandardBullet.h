#pragma once

#include "Bullet.h"

#include <QColor>

#include <unordered_set>

namespace arena {

// 标准弹：直线飞行，可带穿透和暴击；行为和旧的实体版一致。
class StandardBullet : public Bullet {
public:
    StandardBullet(const Math::Vec2 &position, const Math::Vec2 &velocity, float damage,
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

private:
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
    std::unordered_set<int> hits_;
};

} // namespace arena
