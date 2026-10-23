#pragma once

#include "Math.h"

#include <QPainter>

class Arena;

namespace arena {

enum class DamageSource {
    Bullet,
    Shock,
    Contact,
};

struct Damage {
    float amount = 0.0f;
    bool critical = false;
    DamageSource source = DamageSource::Bullet;
};

// Fighter 是 Player / Enemy 的共同战斗者接口。
// 只放真正共享的“动作”和只读查询；不把 PlayerState / Enemy 字段塞进基类。
class Fighter {
public:
    virtual ~Fighter() = default;

    virtual void update(Arena &arena, float dt) = 0;
    virtual void shoot(Arena &arena) = 0;
    virtual void takeDamage(Arena &arena, const Damage &damage) = 0;
    virtual void die(Arena &arena) = 0;
    virtual void draw(QPainter &world, QPainter &glow, const Arena &arena) const = 0;

    virtual bool isAlive() const = 0;
    virtual const Math::Vec2 &position() const = 0;
    virtual float collisionRadius() const = 0;
    virtual int id() const = 0;
    virtual float contactDamage() const = 0;
};

} // namespace arena
