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
#include "Loadout/Loadout.h"
#include "GameConfig.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace arena {

QColor playerColor(int playerId)
{
    switch (((playerId - 1) % 4 + 4) % 4) {
    case 1:
        return Base::Amber;
    case 2:
        return Base::Violet;
    case 3:
        return Base::Green;
    default:
        return Base::Cyan;
    }
}

Math::Vec2 playerSpawnPosition(int playerId)
{
    // slot 位置固定、彼此拉开；单机（id=1 且只有一架飞机）不走这里。
    switch (((playerId - 1) % 4 + 4) % 4) {
    case 1:
        return {860.0f, 470.0f};
    case 2:
        return {480.0f, 600.0f};
    case 3:
        return {800.0f, 600.0f};
    default:
        return {420.0f, 470.0f};
    }
}

void Player::reset()
{
    state = PlayerState{};
    state.playerId = 1;
    state.color = Base::Cyan;
    state.hp = data::kPlayer.hp;
    state.maxHp = data::kPlayer.hp;
    state.hpDisplay = data::kPlayer.hp;
    state.speed = data::kPlayer.speed;
    const data::LauncherDef &launcher = LoadoutService.launcher();
    state.damage = data::kPlayer.damage * launcher.damageScale;
    state.fireCd = data::kPlayer.fireCd * launcher.fireCdScale;
    state.bulletSpeed = data::kPlayer.bulletSpeed * launcher.speedScale;
    state.pos = {GameConfig::ScreenWidth / 2.0f,
                 GameConfig::ScreenHeight / 2.0f + 32.0f};
    state.muzzlePos = state.pos;
    state.aim = -Math::Pi / 2.0f;
    spawnPos = state.pos;
    respawnEnabled = false;
}

void Player::configure(int playerId, const QColor &colorValue,
                       const Math::Vec2 &spawn, bool respawn)
{
    reset();
    state.playerId = playerId;
    state.color = colorValue.isValid() ? colorValue : playerColor(playerId);
    state.pos = spawn;
    state.muzzlePos = spawn;
    spawnPos = spawn;
    respawnEnabled = respawn;
}

void Player::respawn(const Context &ctx)
{
    state.alive = true;
    state.active = true;
    state.hp = state.maxHp;
    state.hpDisplay = state.maxHp;
    state.pos = spawnPos;
    state.muzzlePos = spawnPos;
    state.vel = Math::Vec2{};
    state.dashTime = 0.0f;
    state.dashBuffer = 0.0f;
    state.dashCd = 0.0f;
    state.invuln = std::max(state.invuln, 1.25f);
    state.spawnPulse = 0.9f;
    state.respawnTimer = 0.0f;
    input = InputState{};
    ctx.entities.spawnBurst(ctx, state.pos, state.color, 22, 170.0f, 0.55f);
    ctx.entities.addShockwave(state.pos, 96.0f, state.color, 0.42f, 2);
    ctx.host.addFeedback(state.color, 0, 4.0f);
}

void Player::beginWave()
{
    state.dashCd = 0.0f;
    state.invuln = std::max(state.invuln, 0.7f);
    state.spawnPulse = 0.9f;
}

void Player::update(const Context &ctx, float dt)
{
    // 输入由 GameSimulation::setInput() 写入 input（单机时来自 BaseService），
    // Player 不再直接读全局键鼠状态。
    if (input.dashRequested) {
        state.dashBuffer = std::max(state.dashBuffer, 0.16f);
        input.dashRequested = false;
    }
    const Math::Vec2 move = input.moveAxis;
    if (input.mouseActive) {
        const Math::Vec2 aimDelta = input.mousePosition - state.pos;
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

    if (input.shooting && state.shotTimer <= 0.0f) {
        shoot(ctx);
        state.shotTimer = state.fireCd;
    }
}

void Player::shoot(const Context &ctx)
{
    const Math::Vec2 forward(std::cos(state.aim), std::sin(state.aim));
    const Math::Vec2 side(-forward.y, forward.x);
    const data::LauncherDef &launcher = LoadoutService.launcher();
    // 霰弹的 3 发算一波；分裂弹头每次增加一整波，而不是只加 1 枚弹丸，
    // 因此相对收益和单发武器一致。多波在同一收紧的锥内随机分布，避免张角膨胀。
    const int pellets = std::max(1, launcher.extraShots + 1);
    const int count = pellets * std::max(1, state.multishot);
    const float spread = (pellets > 1 ? 0.075f : 0.115f) * launcher.spreadScale;
    for (int index = 0; index < count; ++index) {
        const float offset = pellets > 1
                                 ? RandomService.real(-spread, spread)
                                 : ((float(index) - (count - 1) / 2.0f) * spread);
        const Math::Vec2 direction = forward.rotated(offset);
        const float sideOffset = pellets > 1
                                     ? RandomService.real(-5.0f, 5.0f)
                                     : ((float(index) - (count - 1) / 2.0f) * 4.0f);
        const Math::Vec2 origin = state.pos + forward * 20.0f + side * sideOffset;
        // 吸附是 buff（强化“磁吸弹道”）给的：没拿过强化时两个值都是 0 → 直线弹道。
        ctx.bullets.spawnStandard(
            origin, direction * state.bulletSpeed, state.damage,
            LoadoutService.color(), Bullet::Standpoint::Player,
            launcher.radius, state.pierce + launcher.pierceBonus,
            state.critChance, state.homingRadius, state.homingRatio);
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
    // 单机：由 Arena 在没剩下任何活着的飞机时标记失败，保持原来的“阵亡即失败”。
    // 多玩家：这一架进入重新部署倒计时；如果所有飞机同时倒下，Arena 仍会判负。
    state.respawnTimer = respawnEnabled ? 3.2f : 0.0f;
    ctx.entities.spawnBurst(ctx, state.pos, state.color, 55, 280.0f, 1.0f);
    ctx.entities.spawnBurst(ctx, state.pos, Base::Coral, 35, 210.0f, 0.85f);
    ctx.entities.addShockwave(state.pos, 190.0f, Base::Red, 0.9f, 3);
}

void Player::draw(QPainter &world, QPainter &glow, const Context &ctx) const
{
    if (!state.alive || !state.active) {
        return;
    }
    const bool local = ctx.combatants.isLocalPlayer(state.playerId);
    const QColor accent = state.color.isValid() ? state.color : Base::Cyan;
    const Math::Vec2 forward(std::cos(state.aim), std::sin(state.aim));
    const Math::Vec2 side(-forward.y, forward.x);
    const bool flicker = state.invuln > 0.0f && int(state.invuln * 26.0f) % 2 == 0;
    const QColor bodyColor = flicker ? Base::shade(accent, 0.55f) : accent;
    Base::glow(glow, state.pos, 20.0f, accent, local ? 0.7f : 0.45f);

    if (state.spawnPulse > 0.0f) {
        const float pulseT = Math::clamp(state.spawnPulse / 0.9f, 0.0f, 1.0f);
        const float ringRadius = 26.0f + (1.0f - pulseT) * 68.0f;
        Base::strokeCircle(world, state.pos, ringRadius,
                           Base::withAlpha(accent, int(150.0f * pulseT)), 2.0f);
        Base::glow(glow, state.pos, ringRadius, accent, 0.35f * pulseT);
    }
    if (state.invuln > 0.0f) {
        Base::strokeCircle(world, state.pos,
                           state.radius + 9.0f + std::sin(state.invuln * 30.0f) * 3.0f,
                           Base::withAlpha(accent, 70), 1.0f);
    }
    if (state.dashTime > 0.0f) {
        Base::strokeCircle(world, state.pos, 26.0f,
                           Base::withAlpha(accent, 60), 2.0f);
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
    Base::strokeCircle(world, state.pos, 7.0f, accent, 1.0f);

    if (local) {
        // 只有本机飞机画瞄准辅助线；远程飞机只显示机身和朝向。
        Base::strokeLine(world, state.pos + forward * 24.0f,
                         state.pos + forward * 66.0f,
                         Base::withAlpha(accent, 42), 1.0f);
    } else {
        // 远程飞机：额外的识别环 + 更短的朝向标记。
        Base::strokeCircle(world, state.pos, state.radius + 5.0f,
                           Base::withAlpha(accent, 95), 1.2f);
        Base::strokeLine(world, state.pos + forward * (state.radius - 2.0f),
                         state.pos + forward * (state.radius + 10.0f),
                         Base::withAlpha(accent, 150), 1.5f);
    }

    if (state.fireFlash > 0.0f) {
        const int strength = int(160.0f * (state.fireFlash / 0.07f));
        Base::fillCircle(world, state.muzzlePos,
                         6.0f + 3.0f * (state.fireFlash / 0.07f),
                         Base::withAlpha(QColor(205, 252, 255), strength));
        Base::glow(glow, state.muzzlePos, 14.0f, accent, 0.9f);
    }
}

} // namespace arena
