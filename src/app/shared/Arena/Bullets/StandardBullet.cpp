#include "Arena/Bullets/StandardBullet.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Base/Base.h"
#include "Fighter.h"
#include "GameConfig.h"
#include "Random.h"

#include <algorithm>

namespace arena {

StandardBullet::StandardBullet(const Math::Vec2 &position, const Math::Vec2 &velocity,
                               float damage, const QColor &color,
                               Standpoint standpoint, float radiusValue,
                               int pierce, float critChance, float homingRadius,
                               float homingRatio)
{
    reset(position, velocity, damage, color, standpoint, radiusValue, pierce, critChance,
          homingRadius, homingRatio);
}

void StandardBullet::reset(const Math::Vec2 &position, const Math::Vec2 &velocity,
                           float damage, const QColor &color, Standpoint standpoint,
                           float radiusValue, int pierce, float critChance,
                           float homingRadius, float homingRatio)
{
    pos_ = position;
    prev_ = position;
    vel_ = velocity;
    speed_ = velocity.length();
    damage_ = damage;
    color_ = color;
    standpoint_ = standpoint;
    radius_ = radiusValue;
    pierce_ = pierce;
    critChance_ = critChance;
    homingRadius_ = std::max(0.0f, homingRadius);
    homingRatio_ = std::clamp(homingRatio, 0.0f, 1.0f);
    life_ = standpoint == Standpoint::Player ? 1.6f : 4.5f;
    alive_ = true;
    hitTargetCount_ = 0;
}

// 吸附：找 homingRadius_ 内最近的对手，把当前飞行方向朝它转 homingRatio_。
// 只改方向不改速度，所以弹道会拐弯但不会忽快忽慢；ratio 按 60Hz 归一，
// 高/低帧率下的转向手感一致。
void StandardBullet::steer(const Context &ctx, float dt)
{
    if (homingRadius_ <= 0.0f || homingRatio_ <= 0.0f || speed_ <= 0.0f
        || !alive_) {
        return;
    }

    Math::Vec2 target;
    bool found = false;
    float bestSquared = homingRadius_ * homingRadius_;
    if (standpoint_ == Standpoint::Player) {
        // 玩家弹吸附敌人（含 Boss）。
        for (const auto &owned : ctx.combatants.enemies()) {
            if (owned == nullptr || !owned->isAlive()) {
                continue;
            }
            const float distance = (owned->pos - pos_).lengthSquared();
            if (distance < bestSquared) {
                bestSquared = distance;
                target = owned->pos;
                found = true;
            }
        }
    } else {
        // 敌方弹吸附玩家飞机（多玩家时各弹独立挑最近的）。
        for (const Player &player : ctx.combatants.players()) {
            if (!player.state.active || !player.state.alive) {
                continue;
            }
            const float distance = (player.state.pos - pos_).lengthSquared();
            if (distance < bestSquared) {
                bestSquared = distance;
                target = player.state.pos;
                found = true;
            }
        }
    }
    if (!found) {
        return;
    }

    const Math::Vec2 desired = target - pos_;
    if (desired.lengthSquared() < 1.0f) {
        return;
    }
    const Math::Vec2 direction = vel_.lengthSquared() > 0.0f
                                     ? vel_.normalized()
                                     : Math::Vec2(1.0f, 0.0f);
    const float blend = std::clamp(homingRatio_ * dt * 60.0f, 0.0f, 1.0f);
    const Math::Vec2 mixed = direction * (1.0f - blend)
                             + desired.normalized() * blend;
    if (mixed.lengthSquared() < 1e-4f) {
        return;
    }
    vel_ = mixed.normalized() * speed_;
}

void StandardBullet::advance(float dt)
{
    prev_ = pos_;
    pos_ += vel_ * dt;
    life_ -= dt;
    if (life_ <= 0.0f) {
        alive_ = false;
    }
    if (pos_.x < -50.0f || pos_.x > GameConfig::ScreenWidth + 50.0f
        || pos_.y < -50.0f || pos_.y > GameConfig::ScreenHeight + 50.0f) {
        alive_ = false;
    }
}

void StandardBullet::draw(QPainter &world, QPainter &glow) const
{
    if (!alive_) {
        return;
    }
    const Math::Vec2 direction = vel_.lengthSquared() > 0.0f
                                     ? vel_.normalized()
                                     : Math::Vec2(1.0f, 0.0f);
    const bool friendly = standpoint_ == Standpoint::Player;
    const Math::Vec2 tail = pos_ - direction * (friendly ? 8.0f : 6.0f);
    Base::strokeLine(world, tail, pos_, Base::withAlpha(color_, 105), 2.0f);
    Base::fillCircle(world, pos_, radius_, color_);
    Base::fillCircle(world, pos_, std::max(1.0f, radius_ * 0.42f), Base::White);
    Base::glow(glow, pos_, radius_ * 3.0f, color_, 1.0f);
}

bool StandardBullet::hit(const Context &ctx, Fighter &target)
{
    const int targetId = target.id();
    for (int i = 0; i < hitTargetCount_; ++i) {
        if (hitTargets_[i] == targetId) {
            // 穿透弹在同一个目标上不重复结算。
            return true;
        }
    }
    const bool critical = RandomService.real(0.0f, 1.0f) < critChance_;
    target.takeDamage(ctx, Damage{damage_ * (critical ? 2.0f : 1.0f), critical,
                                    DamageSource::Bullet});
    if (hitTargetCount_ < MaxHitTargets) {
        hitTargets_[hitTargetCount_++] = targetId;
    }
    if (pierce_ <= 0) {
        alive_ = false;
        return false;
    }
    --pierce_;
    return true;
}

} // namespace arena
