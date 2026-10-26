#include "Player.h"

#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Host.h"
#include "Arena/State.h"
#include "Arena/Bullets/StandardBullet.h"
#include "Arena/Entities/FloatingText.h"
#include "Arena/Entities/Particle.h"
#include "Arena/Entities/Shockwave.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace arena {

void Player::reset()
{
    state = PlayerState{};
    state.hp = data::kPlayer.hp;
    state.maxHp = data::kPlayer.hp;
    state.hpDisplay = data::kPlayer.hp;
    state.speed = data::kPlayer.speed;
    state.damage = data::kPlayer.damage;
    state.fireCd = data::kPlayer.fireCd;
    state.bulletSpeed = data::kPlayer.bulletSpeed;
    state.pos = {GameConfig::ScreenWidth / 2.0f,
                 GameConfig::ScreenHeight / 2.0f + 32.0f};
    state.muzzlePos = state.pos;
    state.aim = -Math::Pi / 2.0f;
}

void Player::beginWave()
{
    state.dashCd = 0.0f;
    state.invuln = std::max(state.invuln, 0.7f);
    state.spawnPulse = 0.9f;
}

void Player::update(const Context &ctx, float dt)
{
    const Math::Vec2 move = BaseService.movementAxis();
    if (BaseService.mouseActive()) {
        const Math::Vec2 aimDelta = BaseService.mousePosition() - state.pos;
        if (aimDelta.lengthSquared() > 1.0f) {
            state.aim = std::atan2(aimDelta.y, aimDelta.x);
        }
    }
    const Math::Vec2 forward(std::cos(state.aim), std::sin(state.aim));

    state.shotTimer = std::max(0.0f, state.shotTimer - dt);
    state.fireFlash = std::max(0.0f, state.fireFlash - dt);
    state.invuln = std::max(0.0f, state.invuln - dt);
    state.dashCd = std::max(0.0f, state.dashCd - dt);
    state.dashBuffer = std::max(0.0f, state.dashBuffer - dt);
    state.spawnPulse = std::max(0.0f, state.spawnPulse - dt);

    if (state.dashBuffer > 0.0f && state.dashCd <= 0.0f) {
        const Math::Vec2 direction = move.lengthSquared() > 0.0f ? move : forward;
        state.dashDir = direction.normalized();
        state.dashTime = 0.16f;
        state.dashCd = state.dashCdMax;
        state.invuln = std::max(state.invuln, 0.30f);
        state.dashBuffer = 0.0f;
        BaseService.playSound(QStringLiteral("dash"));
        ctx.host.addFeedback(Base::Cyan, 0, 5.0f);
        ctx.entities.spawnBurst(ctx, state.pos, Base::Cyan, 18, 185.0f, 0.45f);
        ctx.entities.addShockwave(state.pos, 80.0f, Base::Cyan, 0.28f, 2);
        if (state.shockLevel > 0) {
            const float radius = 95.0f + 14.0f * state.shockLevel;
            const float damage = 20.0f + 14.0f * state.shockLevel;
            ctx.entities.addShockwave(state.pos, radius, Base::Magenta, 0.40f, 3);
            ctx.combatants.damageEnemiesInRadius(ctx, state.pos, radius, damage,
                                                  DamageSource::Shock);
        }
    }

    if (state.dashTime > 0.0f) {
        state.dashTime -= dt;
        state.pos += state.dashDir * 930.0f * dt;
        state.vel = state.dashDir * state.speed;
        if (ctx.entities.effectsTimer() <= 0.0f) {
            ctx.entities.effectsTimer() = 0.018f;
            for (int i = 0; i < 2; ++i) {
                ctx.entities.particles().push_back(Particle{
                    state.pos + Math::Vec2(RandomService.real(-6.0f, 6.0f),
                                           RandomService.real(-6.0f, 6.0f)),
                    -state.dashDir * RandomService.real(20.0f, 90.0f)
                        + Math::Vec2(RandomService.real(-25.0f, 25.0f),
                                     RandomService.real(-25.0f, 25.0f)),
                    RandomService.real(0.15f, 0.25f), 0.25f,
                    RandomService.real(3.0f, 7.0f), Base::Cyan, 2.8f, 0.0f, 0});
            }
        }
    } else {
        const Math::Vec2 target = move * state.speed;
        state.vel += (target - state.vel) * std::min(1.0f, dt * 14.0f);
        state.pos += state.vel * dt;
    }

    state.pos.x = Math::clamp(state.pos.x, GameConfig::FieldLeft + 2.0f,
                              GameConfig::FieldRight - 2.0f);
    state.pos.y = Math::clamp(state.pos.y, GameConfig::FieldTop + 2.0f,
                              GameConfig::FieldBottom - 2.0f);

    if (state.vel.lengthSquared() > 1600.0f && ctx.entities.effectsTimer() <= 0.0f) {
        ctx.entities.effectsTimer() = 0.035f;
        const Math::Vec2 back = -state.vel.normalized() * 10.0f;
        ctx.entities.particles().push_back(Particle{
            state.pos + back + Math::Vec2(RandomService.real(-3.0f, 3.0f),
                                          RandomService.real(-3.0f, 3.0f)),
            -state.vel.normalized() * RandomService.real(30.0f, 70.0f),
            RandomService.real(0.18f, 0.32f), 0.32f,
            RandomService.real(2.0f, 4.5f), Base::CyanSoft, 3.0f, 0.0f, 0});
    }

    if (BaseService.shootingHeld() && state.shotTimer <= 0.0f) {
        shoot(ctx);
        state.shotTimer = state.fireCd;
    }
}

void Player::shoot(const Context &ctx)
{
    const Math::Vec2 forward(std::cos(state.aim), std::sin(state.aim));
    const Math::Vec2 side(-forward.y, forward.x);
    constexpr float spread = 0.115f;
    const int count = std::max(1, state.multishot);
    for (int index = 0; index < count; ++index) {
        const float offset = (float(index) - (count - 1) / 2.0f) * spread;
        const Math::Vec2 direction = forward.rotated(offset);
        const Math::Vec2 origin = state.pos + forward * 20.0f
                                  + side * ((float(index) - (count - 1) / 2.0f) * 4.0f);
        ctx.bullets.submit(std::make_unique<StandardBullet>(
            origin, direction * state.bulletSpeed, state.damage, Base::Cyan,
            Bullet::Standpoint::Player, data::kPlayer.bulletRadius, state.pierce,
            state.critChance));
    }
    state.muzzlePos = state.pos + forward * 22.0f;
    state.fireFlash = 0.07f;
    BaseService.playSound(QStringLiteral("shoot"));
    ctx.host.addFeedback(Base::Cyan, 0, 0.55f);
    for (int i = 0; i < 3; ++i) {
        ctx.entities.particles().push_back(Particle{
            state.muzzlePos + forward * RandomService.real(0.0f, 8.0f),
            forward * RandomService.real(70.0f, 180.0f)
                + side * RandomService.real(-65.0f, 65.0f),
            RandomService.real(0.12f, 0.22f), 0.22f,
            RandomService.real(1.6f, 3.5f), QColor(180, 250, 255), 4.0f, 0.0f, 1});
    }
}

void Player::takeDamage(const Context &ctx, const Damage &damage)
{
    if (!state.alive || state.invuln > 0.0f) {
        return;
    }
    state.hp -= damage.amount;
    state.invuln = 0.88f;
    ctx.host.addFeedback(Base::Red, 58, 9.0f);
    BaseService.playSound(QStringLiteral("hurt"));
    ctx.entities.floatingTexts().push_back(FloatingText{
        state.pos + Math::Vec2(0.0f, -30.0f),
        QStringLiteral("-%1").arg(int(damage.amount)), Base::Red,
        0.9f, 0.9f, 17, false});
    ctx.entities.spawnBurst(ctx, state.pos, Base::Coral, 18, 150.0f, 0.5f);
    if (state.hp <= 0.0f) {
        die(ctx);
    }
}

void Player::die(const Context &ctx)
{
    if (!state.alive) {
        return;
    }
    state.hp = 0.0f;
    state.alive = false;
    ctx.state.markPlayerDefeated();
    ctx.entities.spawnBurst(ctx, state.pos, Base::Cyan, 55, 280.0f, 1.0f);
    ctx.entities.spawnBurst(ctx, state.pos, Base::Coral, 35, 210.0f, 0.85f);
    ctx.entities.addShockwave(state.pos, 190.0f, Base::Red, 0.9f, 3);
}

void Player::draw(QPainter &world, QPainter &glow, const Context &ctx) const
{
    Q_UNUSED(ctx);
    if (!state.alive) {
        return;
    }
    const Math::Vec2 forward(std::cos(state.aim), std::sin(state.aim));
    const Math::Vec2 side(-forward.y, forward.x);
    const bool flicker = state.invuln > 0.0f && int(state.invuln * 26.0f) % 2 == 0;
    const QColor bodyColor = flicker ? QColor(110, 155, 170) : Base::Cyan;
    Base::glow(glow, state.pos, 20.0f, Base::Cyan, 0.7f);

    if (state.spawnPulse > 0.0f) {
        const float pulseT = Math::clamp(state.spawnPulse / 0.9f, 0.0f, 1.0f);
        const float ringRadius = 26.0f + (1.0f - pulseT) * 68.0f;
        Base::strokeCircle(world, state.pos, ringRadius,
                           Base::withAlpha(Base::Cyan, int(150.0f * pulseT)), 2.0f);
        Base::glow(glow, state.pos, ringRadius, Base::Cyan, 0.35f * pulseT);
    }
    if (state.invuln > 0.0f) {
        Base::strokeCircle(world, state.pos,
                           state.radius + 9.0f + std::sin(state.invuln * 30.0f) * 3.0f,
                           Base::withAlpha(Base::Cyan, 70), 1.0f);
    }
    if (state.dashTime > 0.0f) {
        Base::strokeCircle(world, state.pos, 26.0f,
                           Base::withAlpha(Base::Cyan, 60), 2.0f);
    }

    const Math::Vec2 tip = state.pos + forward * 21.0f;
    const Math::Vec2 rear = state.pos - forward * 13.0f;
    const Math::Vec2 left = rear + side * 10.0f;
    const Math::Vec2 right = rear - side * 10.0f;
    QPolygonF ship;
    ship << QPointF(tip.x, tip.y) << QPointF(left.x, left.y)
         << QPointF((state.pos - forward * 7.0f).x, (state.pos - forward * 7.0f).y)
         << QPointF(right.x, right.y);
    Base::polygon(world, ship, QColor(13, 39, 54), bodyColor, 2.0f);
    Base::strokeLine(world, left, right, Base::shade(bodyColor, 0.62f), 1.0f);
    Base::fillCircle(world, state.pos, 4.0f, QColor(222, 253, 255));
    Base::strokeCircle(world, state.pos, 7.0f, Base::Cyan, 1.0f);
    Base::strokeLine(world, state.pos + forward * 24.0f,
                     state.pos + forward * 66.0f,
                     Base::withAlpha(Base::Cyan, 42), 1.0f);

    if (state.fireFlash > 0.0f) {
        const int strength = int(160.0f * (state.fireFlash / 0.07f));
        Base::fillCircle(world, state.muzzlePos,
                         6.0f + 3.0f * (state.fireFlash / 0.07f),
                         Base::withAlpha(QColor(205, 252, 255), strength));
        Base::glow(glow, state.muzzlePos, 14.0f, Base::Cyan, 0.9f);
    }
}

} // namespace arena
