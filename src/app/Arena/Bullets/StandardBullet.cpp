#include "Arena/Bullets/StandardBullet.h"

#include "Arena/Arena.h"
#include "Base/Base.h"
#include "Fighter.h"
#include "GameConfig.h"
#include "Random.h"

#include <algorithm>

namespace arena {

StandardBullet::StandardBullet(const Math::Vec2 &position, const Math::Vec2 &velocity,
                               float damage, const QColor &color,
                               Standpoint standpoint, float radiusValue,
                               int pierce, float critChance)
    : pos_(position), prev_(position), vel_(velocity), damage_(damage), color_(color),
      standpoint_(standpoint), radius_(radiusValue), pierce_(pierce),
      critChance_(critChance), life_(standpoint == Standpoint::Player ? 1.6f : 4.5f)
{
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

bool StandardBullet::hit(Arena &arena, Fighter &target)
{
    const int targetId = target.id();
    if (hits_.count(targetId) != 0) {
        // 穿透弹在同一个目标上不重复结算。
        return true;
    }
    const bool critical = arena.random().real(0.0f, 1.0f) < critChance_;
    target.takeDamage(arena, Damage{damage_ * (critical ? 2.0f : 1.0f), critical,
                                    DamageSource::Bullet});
    hits_.insert(targetId);
    if (pierce_ <= 0) {
        alive_ = false;
        return false;
    }
    --pierce_;
    return true;
}

} // namespace arena
