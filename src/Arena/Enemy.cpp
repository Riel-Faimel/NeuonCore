#include "Enemy.h"

#include "Arena/Arena.h"
#include "Arena/BossState.h"
#include "Arena/Bullet.h"
#include "Arena/FloatingText.h"
#include "Arena/Kill.h"
#include "GameConfig.h"
#include "Random.h"
#include "base/Base.h"

#include <QHash>
#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>

namespace arena {

int Enemy::sNextId = 1;

const EnemyStats &Enemy::statsFor(const QString &kind)
{
    static const QHash<QString, EnemyStats> table = {
        {QStringLiteral("drone"), {26.0f, 143.0f, 13.0f, 9.0f, 10, Base::Coral}},
        {QStringLiteral("spitter"), {38.0f, 94.0f, 15.0f, 10.0f, 18, Base::Magenta}},
        {QStringLiteral("tank"), {122.0f, 60.0f, 24.0f, 18.0f, 36, Base::Amber}},
        {QStringLiteral("splitter"), {54.0f, 108.0f, 18.0f, 12.0f, 26, Base::Violet}},
        {QStringLiteral("mini"), {13.0f, 188.0f, 8.0f, 6.0f, 4, QColor(218, 154, 255)}},
        {QStringLiteral("boss"), {1750.0f, 68.0f, 50.0f, 28.0f, 600, QColor(255, 74, 160)}},
    };
    const auto it = table.constFind(kind);
    if (it != table.constEnd()) {
        return it.value();
    }
    return table.constFind(QStringLiteral("drone")).value();
}

Enemy::Enemy(const QString &enemyKind, const Math::Vec2 &position, int waveNumber,
             Random &random)
    : kind(enemyKind), eid(sNextId++), pos(position), wave(waveNumber)
{
    const EnemyStats &stats = statsFor(kind);
    radius = stats.radius;
    color = stats.color;
    score = stats.score;
    speed = stats.speed * (1.0f + 0.022f * (wave - 1));
    damage = stats.damage * (1.0f + 0.045f * (wave - 1));
    boss = kind == QStringLiteral("boss");
    if (boss) {
        maxHp = stats.hp;
        speed = 70.0f;
        damage = 28.0f;
    } else {
        maxHp = stats.hp * (1.0f + 0.16f * (wave - 1));
    }
    hp = maxHp;
    hpDisplay = maxHp;
    attackTimer = random.real(1.2f, 2.0f);
    radialTimer = random.real(1.3f, 2.2f);
    strafe = random.integer(0, 1) == 0 ? -1 : 1;
    phase = random.real(0.0f, Math::Tau);
}

void Enemy::update(Arena &arena, float dt)
{
    if (!alive) {
        return;
    }

    age += dt;
    hitFlash = std::max(0.0f, hitFlash - dt);
    spawnAnim = std::max(0.0f, spawnAnim - dt);
    attackTimer -= dt;
    radialTimer -= dt;
    hpDisplay += (hp - hpDisplay) * std::min(1.0f, dt * 5.0f);

    const Math::Vec2 toPlayer = arena.player_.state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    const Math::Vec2 perpendicular(-direction.y, direction.x);
    Math::Vec2 desired;

    if (kind == QStringLiteral("boss")) {
        if (distance > 265.0f) {
            desired = direction * speed;
        } else if (distance < 185.0f) {
            desired = -direction * speed * 0.9f;
        } else {
            desired = perpendicular * speed * 0.55f * float(strafe);
        }
        const bool enraged = hp < maxHp * 0.48f;
        if (age > 1.45f) {
            if (attackTimer <= 0.0f) {
                attackTimer = enraged ? 0.95f : 1.25f;
                shoot(arena);
            }
            if (radialTimer <= 0.0f) {
                radialTimer = enraged ? 2.15f : 2.65f;
                const int count = enraged ? 18 : 14;
                const float offset = age * 0.55f;
                for (int i = 0; i < count; ++i) {
                    const float angle = offset + Math::Tau * float(i) / float(count);
                    const Math::Vec2 bulletDirection(std::cos(angle), std::sin(angle));
                    arena.enemyBullets_.emplace_back(pos + bulletDirection * 42.0f,
                                                     bulletDirection * 285.0f, 13.0f,
                                                     Base::Magenta, false, 5.0f);
                }
                arena.addShockwave(pos, 92.0f, Base::Magenta, 0.36f, 2);
            }
        }
    } else if (kind == QStringLiteral("spitter")) {
        if (distance > 315.0f) {
            desired = direction * speed;
        } else if (distance < 205.0f) {
            desired = -direction * speed;
        } else {
            desired = perpendicular * speed * 0.62f * float(strafe);
        }
        if (age > 0.9f && attackTimer <= 0.0f && distance < 540.0f) {
            attackTimer = arena.random_.real(1.8f, 2.5f);
            shoot(arena);
        }
    } else {
        float weave = 0.0f;
        if (kind == QStringLiteral("drone") || kind == QStringLiteral("mini")) {
            weave = std::sin(age * 3.1f + phase) * 0.32f;
        }
        desired = direction.rotated(weave) * speed;
    }

    vel += (desired - vel) * std::min(1.0f, dt * 4.8f);
    pos += vel * dt;
    pos.x = Math::clamp(pos.x, GameConfig::FieldLeft + radius,
                        GameConfig::FieldRight - radius);
    pos.y = Math::clamp(pos.y, GameConfig::FieldTop + radius + 4.0f,
                        GameConfig::FieldBottom - radius - 4.0f);
}

void Enemy::shoot(Arena &arena)
{
    if (kind == QStringLiteral("boss")) {
        const bool enraged = hp < maxHp * 0.48f;
        fireAimed(arena, enraged ? 3 : 2, 0.11f, 315.0f);
        arena.base_.playSound(QStringLiteral("shoot_enemy"));
        return;
    }
    if (kind == QStringLiteral("spitter")) {
        const Math::Vec2 toPlayer = arena.player_.state.pos - pos;
        const float base = std::atan2(toPlayer.y, toPlayer.x);
        const Math::Vec2 aim = Math::Vec2(std::cos(base), std::sin(base))
            .rotated(arena.random_.real(-0.055f, 0.055f));
        arena.enemyBullets_.emplace_back(pos + aim * 18.0f, aim * 300.0f, 11.0f,
                                         Base::Magenta, false, 4.0f);
        arena.spawnBurst(pos, Base::Magenta, 5, 70.0f, 0.26f);
        arena.base_.playSound(QStringLiteral("shoot_enemy"));
    }
}

void Enemy::fireAimed(Arena &arena, int count, float spread, float bulletSpeed)
{
    const Math::Vec2 toPlayer = arena.player_.state.pos - pos;
    const float base = std::atan2(toPlayer.y, toPlayer.x);
    for (int i = 0; i < count; ++i) {
        const float offset = (float(i) - (count - 1) / 2.0f) * spread;
        const Math::Vec2 direction(std::cos(base + offset), std::sin(base + offset));
        arena.enemyBullets_.emplace_back(pos + direction * 46.0f,
                                          direction * bulletSpeed, 13.0f,
                                          QColor(255, 112, 190), false, 5.0f);
    }
    arena.spawnBurst(pos, Base::Magenta, 8, 100.0f, 0.35f);
}

void Enemy::takeDamage(Arena &arena, const Damage &damage)
{
    if (!alive) {
        return;
    }
    hp -= damage.amount;
    hitFlash = 0.09f;
    const QColor textColor = damage.critical ? Base::Amber : Base::White;
    arena.floatingTexts_.push_back(FloatingText{
        pos + Math::Vec2(arena.random_.real(-7.0f, 7.0f), -radius - 7.0f),
        QString::number(int(damage.amount)), textColor, 0.62f, 0.62f,
        damage.critical ? 16 : 14, damage.critical});
    arena.spawnBurst(pos, color, damage.critical ? 6 : 3, 90.0f, 0.22f);
    if (damage.source == DamageSource::Bullet
        && arena.random_.real(0.0f, 1.0f) < 0.35f) {
        arena.base_.playSound(QStringLiteral("hit"));
    }

    if (boss) {
        arena.bossState_.onDamage(arena, *this);
    }

    if (hp <= 0.0f) {
        die(arena);
    }
}

void Enemy::die(Arena &arena)
{
    if (!alive) {
        return;
    }
    arena.kills_.handle(arena, *this);
}

void Enemy::draw(QPainter &world, QPainter &glow, const Arena &arena) const
{
    if (!alive) {
        return;
    }
    const Math::Vec2 toPlayer = arena.player_.state.pos - pos;
    const float facing = std::atan2(toPlayer.y, toPlayer.x);
    drawBody(world, glow, facing);

    if (hitFlash > 0.0f) {
        Base::fillCircle(world, pos, radius * 0.9f,
                         Base::withAlpha(Base::White,
                                         int(110.0f * (hitFlash / 0.09f))));
    }
    if (!boss && hp < maxHp) {
        const float width = std::max(20.0f, radius * 2.1f);
        const QRectF bar(pos.x - width / 2.0f, pos.y - radius - 11.0f, width, 3.0f);
        world.save();
        world.fillRect(bar, QColor(24, 26, 38));
        QRectF fill = bar;
        fill.setWidth(std::max(1.0f, width * Math::clamp(hp / maxHp, 0.0f, 1.0f)));
        world.fillRect(fill, color);
        world.restore();
    }
}

void Enemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const Math::Vec2 side(-forward.y, forward.x);
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    if (kind == QStringLiteral("drone")) {
        const Math::Vec2 tip = pos + forward * (r * 1.2f);
        const Math::Vec2 left = pos - forward * (r * 0.78f) + side * (r * 0.72f);
        const Math::Vec2 notch = pos - forward * (r * 0.32f);
        const Math::Vec2 right = pos - forward * (r * 0.78f) - side * (r * 0.72f);
        QPolygonF body;
        body << QPointF(tip.x, tip.y) << QPointF(left.x, left.y)
             << QPointF(notch.x, notch.y) << QPointF(right.x, right.y);
        Base::polygon(world, body, Base::shade(color, 0.18f), color, 2.0f);
        Base::fillCircle(world, pos, 3.0f, color);
        Base::strokeLine(world, left, right, Base::shade(color, 0.55f), 1.0f);
    } else if (kind == QStringLiteral("spitter")) {
        QPolygonF body;
        const float rotation = age * 0.8f;
        for (int i = 0; i < 6; ++i) {
            const float angle = rotation + Math::Tau * float(i) / 6.0f;
            body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
        }
        Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
        QPolygonF inner;
        for (int i = 0; i < 3; ++i) {
            const float angle = -rotation * 1.3f + Math::Tau * float(i) / 3.0f;
            inner << QPointF(pos.x + std::cos(angle) * (r * 0.48f),
                             pos.y + std::sin(angle) * (r * 0.48f));
        }
        Base::polygon(world, inner, QColor(0, 0, 0, 0), color, 1.0f);
        Base::fillCircle(world, pos, 3.0f, Base::White);
        Base::strokeCircle(world, pos, 7.0f, color, 1.0f);
        Base::strokeLine(world, pos + forward * 8.0f, pos + forward * (r + 6.0f),
                         Base::shade(color, 0.8f), 2.0f);
    } else if (kind == QStringLiteral("tank")) {
        QPolygonF body;
        const float rotation = age * 0.25f;
        for (int i = 0; i < 8; ++i) {
            const float angle = rotation + Math::Tau * float(i) / 8.0f;
            body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
        }
        Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
        QPolygonF inner;
        for (int i = 0; i < 4; ++i) {
            const float angle = -rotation + Math::Tau * float(i) / 4.0f + Math::Pi / 4.0f;
            inner << QPointF(pos.x + std::cos(angle) * (r * 0.56f),
                             pos.y + std::sin(angle) * (r * 0.56f));
        }
        Base::polygon(world, inner, Base::shade(color, 0.34f), color, 1.0f);
        Base::fillCircle(world, pos, 4.0f, QColor(255, 220, 160));
    } else if (kind == QStringLiteral("splitter")) {
        Base::fillCircle(world, pos, r, Base::shade(color, 0.18f));
        Base::strokeCircle(world, pos, r, color, 2.0f);
        Base::strokeLine(world, pos - side * r, pos + side * r,
                         Base::shade(color, 0.75f), 1.0f);
        const Math::Vec2 offset = side * (3.0f + std::sin(age * 4.0f) * 2.0f);
        Base::fillCircle(world, pos + offset, 3.0f, color);
        Base::fillCircle(world, pos - offset, 3.0f, color);
        Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 70), 1.0f);
    } else if (kind == QStringLiteral("mini")) {
        QPolygonF body;
        body << QPointF(pos.x + forward.x * 9.0f, pos.y + forward.y * 9.0f)
             << QPointF(pos.x + side.x * 6.0f, pos.y + side.y * 6.0f)
             << QPointF(pos.x - forward.x * 7.0f, pos.y - forward.y * 7.0f)
             << QPointF(pos.x - side.x * 6.0f, pos.y - side.y * 6.0f);
        Base::polygon(world, body, Base::shade(color, 0.18f), color, 1.0f);
        Base::fillCircle(world, pos, 2.0f, Base::White);
    } else if (kind == QStringLiteral("boss")) {
        drawBoss(world, glow, facing);
    }
}

void Enemy::drawBoss(QPainter &world, QPainter &glow, float facing) const
{
    const float r = radius;
    Base::glow(glow, pos, 100.0f, Base::Magenta, 1.3f);

    const std::array<std::array<float, 5>, 3> rings = {{
        {r + 14.0f, 0.55f, age * 0.9f, 1.8f, 2.0f},
        {r + 28.0f, -0.35f, -age * 0.7f, 1.2f, 1.0f},
        {r + 42.0f, 0.22f, age * 0.4f + 2.4f, 0.8f, 1.0f},
    }};
    for (const auto &ring : rings) {
        Base::arcRadians(world, pos, ring[0], ring[2], ring[3],
                         Base::withAlpha(Base::Magenta, 95), ring[4]);
        Base::arcRadians(glow, pos, ring[0], ring[2], ring[3],
                         Base::shade(Base::Magenta, 0.25f), ring[4] + 1.0f);
    }

    const float rotation = -age * 0.5f;
    QPolygonF body;
    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 6.0f;
        body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
    }
    Base::polygon(world, body, QColor(43, 12, 40), QColor(255, 122, 186), 3.0f);

    QPolygonF inner;
    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Pi / 6.0f + Math::Tau * float(i) / 6.0f;
        inner << QPointF(pos.x + std::cos(angle) * (r * 0.66f),
                         pos.y + std::sin(angle) * (r * 0.66f));
    }
    Base::polygon(world, inner, Base::shade(Base::Magenta, 0.22f), Base::Magenta, 1.0f);

    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 6.0f;
        const Math::Vec2 pod = pos + Math::Vec2(std::cos(angle), std::sin(angle)) * (r + 8.0f);
        Base::fillCircle(world, pod, 5.0f, QColor(255, 148, 200));
        Base::strokeCircle(world, pod, 8.0f, Base::Magenta, 1.0f);
    }

    const Math::Vec2 eyeCenter = pos + Math::Vec2(std::cos(facing), std::sin(facing)) * 6.0f;
    Base::fillCircle(world, eyeCenter, 13.0f, QColor(255, 228, 241));
    const Math::Vec2 pupil = eyeCenter + Math::Vec2(std::cos(facing), std::sin(facing)) * 4.0f;
    Base::fillCircle(world, pupil, 6.0f, QColor(91, 7, 43));
    Base::fillCircle(world, pupil + Math::Vec2(-2.0f, -2.0f), 2.0f, QColor(255, 245, 250));
    Base::strokeCircle(world, pos, r + 54.0f, Base::withAlpha(Base::Magenta, 70), 2.0f);
}

} // namespace arena
