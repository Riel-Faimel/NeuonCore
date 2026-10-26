#include "Enemy.h"

#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Events.h"
#include "Arena/State.h"
#include "Arena/Bullets/StandardBullet.h"
#include "Bullet.h"
#include "Arena/Entities/FloatingText.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Random.h"
#include "Base/Base.h"

#include <QHash>
#include <cstdio>
#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

namespace arena {

namespace {

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), int(text.size()));
}

QColor colorForKind(std::string_view kind)
{
    if (kind == "spitter") {
        return Base::Magenta;
    }
    if (kind == "tank") {
        return Base::Amber;
    }
    if (kind == "splitter") {
        return Base::Violet;
    }
    if (kind == "mini") {
        return QColor(218, 154, 255);
    }
    if (kind == "boss") {
        return QColor(255, 74, 160);
    }
    if (kind == "amplifier") {
        return QColor(120, 220, 255);
    }
    if (kind == "linker") {
        return QColor(150, 160, 255);
    }
    if (kind == "mender") {
        return QColor(120, 255, 178);
    }
    if (kind == "rusher") {
        return QColor(255, 140, 80);
    }
    if (kind == "necromancer") {
        return QColor(190, 120, 255);
    }
    if (kind == "summoner") {
        return QColor(255, 200, 90);
    }
    return Base::Coral;
}

} // namespace

int Enemy::sNextId = 1;

Enemy::Kind Enemy::kindOf(const QString &kind)
{
    if (kind == QStringLiteral("drone")) {
        return Kind::Drone;
    }
    if (kind == QStringLiteral("spitter")) {
        return Kind::Spitter;
    }
    if (kind == QStringLiteral("tank")) {
        return Kind::Tank;
    }
    if (kind == QStringLiteral("splitter")) {
        return Kind::Splitter;
    }
    if (kind == QStringLiteral("mini")) {
        return Kind::Mini;
    }
    if (kind == QStringLiteral("boss")) {
        return Kind::Boss;
    }
    if (kind == QStringLiteral("amplifier")) {
        return Kind::Amplifier;
    }
    if (kind == QStringLiteral("linker")) {
        return Kind::Linker;
    }
    if (kind == QStringLiteral("mender")) {
        return Kind::Mender;
    }
    if (kind == QStringLiteral("rusher")) {
        return Kind::Rusher;
    }
    if (kind == QStringLiteral("necromancer")) {
        return Kind::Necromancer;
    }
    if (kind == QStringLiteral("summoner")) {
        return Kind::Summoner;
    }
    return Kind::Other;
}

Enemy::Role Enemy::roleOf(Kind kind)
{
    switch (kind) {
    case Kind::Spitter:
    case Kind::Boss:
    case Kind::Summoner:
        return Role::Ranged;
    case Kind::Amplifier:
    case Kind::Linker:
    case Kind::Mender:
    case Kind::Necromancer:
        return Role::Support;
    default:
        return Role::Melee;
    }
}

const EnemyStats &Enemy::statsFor(const QString &kind)
{
    // 数值来自 src/json/enemies.json（编译期常量），颜色属于表现，留在代码里。
    static const std::array<EnemyStats, data::MaxEnemyKinds> table = [] {
        std::array<EnemyStats, data::MaxEnemyKinds> result{};
        for (int i = 0; i < data::kEnemies.count; ++i) {
            const data::EnemyBase &base = data::kEnemies.items[size_t(i)];
            EnemyStats stats;
            stats.hp = base.hp;
            stats.speed = base.speed;
            stats.radius = base.radius;
            stats.damage = base.damage;
            stats.score = base.score;
            stats.auraRadius = base.auraRadius;
            stats.auraValue = base.auraValue;
            stats.shareRadius = base.shareRadius;
            stats.shareRatio = base.shareRatio;
            stats.healRadius = base.healRadius;
            stats.healRate = base.healRate;
            stats.reviveRadius = base.reviveRadius;
            stats.reviveChance = base.reviveChance;
            stats.lungeInterval = base.lungeInterval;
            stats.chargeRequired = base.chargeRequired;
            stats.summonCount = base.summonCount;
            stats.color = colorForKind(base.kind);
            result[size_t(i)] = stats;
        }
        return result;
    }();
    for (int i = 0; i < data::kEnemies.count; ++i) {
        if (kind == toQString(data::kEnemies.items[size_t(i)].kind)) {
            return table[size_t(i)];
        }
    }
    return table[0];
}

Enemy::Enemy(const QString &enemyKind, const Math::Vec2 &position, int waveNumber)
    : kind(enemyKind), eid(sNextId++), pos(position), wave(waveNumber)
{
    const EnemyStats &stats = statsFor(kind);
    radius = stats.radius;
    color = stats.color;
    score = stats.score;
    speed = stats.speed * (1.0f + data::kWaveScaling.speed * (wave - 1));
    damage = stats.damage * (1.0f + data::kWaveScaling.damage * (wave - 1));
    kindId = kindOf(kind);
    boss = kindId == Kind::Boss;
    if (boss) {
        maxHp = stats.hp;
        speed = stats.speed;
        damage = stats.damage;
    } else {
        maxHp = stats.hp * (1.0f + data::kWaveScaling.hp * (wave - 1));
    }
    hp = maxHp;
    hpDisplay = maxHp;
    attackTimer = RandomService.real(1.2f, 2.0f);
    radialTimer = RandomService.real(1.3f, 2.2f);
    strafe = RandomService.integer(0, 1) == 0 ? -1 : 1;
    phase = RandomService.real(0.0f, Math::Tau);

    if (kindId == Kind::Amplifier) {
        supportRadius = stats.auraRadius;
        supportValue = stats.auraValue;
    } else if (kindId == Kind::Linker) {
        supportRadius = stats.shareRadius;
        supportValue = stats.shareRatio;
    } else if (kindId == Kind::Mender) {
        supportRadius = stats.healRadius;
        supportValue = stats.healRate;
    } else if (kindId == Kind::Necromancer) {
        supportRadius = stats.reviveRadius;
        supportValue = stats.reviveChance;
    } else if (kindId == Kind::Rusher) {
        lungeInterval = std::max(0.4f, stats.lungeInterval);
        lungeTimer = RandomService.real(0.4f, 1.1f);
    } else if (kindId == Kind::Summoner) {
        chargeRequired = std::max(1, stats.chargeRequired);
        summonCount = std::max(1, stats.summonCount);
    }
}

void Enemy::update(const Context &ctx, float dt)
{
    if (!alive) {
        return;
    }

    age += dt;
    hitFlash = std::max(0.0f, hitFlash - dt);
    attackTimer -= dt;
    radialTimer -= dt;
    hpDisplay += (hp - hpDisplay) * std::min(1.0f, dt * 5.0f);
    damageBuffTimer = std::max(0.0f, damageBuffTimer - dt);
    if (damageBuffTimer <= 0.0f) {
        damageBuffBonus = 0.0f;
    }

    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    const Math::Vec2 perpendicular(-direction.y, direction.x);
    Math::Vec2 desired;

    if (kindId == Kind::Boss) {
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
                shoot(ctx);
            }
            if (radialTimer <= 0.0f) {
                radialTimer = enraged ? 2.15f : 2.65f;
                const int count = enraged ? 18 : 14;
                const float offset = age * 0.55f;
                for (int i = 0; i < count; ++i) {
                    const float angle = offset + Math::Tau * float(i) / float(count);
                    const Math::Vec2 bulletDirection(std::cos(angle), std::sin(angle));
                    ctx.bullets.submit(std::make_unique<StandardBullet>(
                        pos + bulletDirection * 42.0f,
                        bulletDirection * data::kBossRadialBullet.speed,
                        data::kBossRadialBullet.damage * damageMultiplier(), Base::Magenta,
                        Bullet::Standpoint::Enemy, data::kBossRadialBullet.radius));
                }
                ctx.entities.addShockwave(pos, 92.0f, Base::Magenta, 0.36f, 2);
            }
        }
    } else if (kindId == Kind::Spitter) {
        if (distance > 315.0f) {
            desired = direction * speed;
        } else if (distance < 205.0f) {
            desired = -direction * speed;
        } else {
            desired = perpendicular * speed * 0.62f * float(strafe);
        }
        if (age > 0.9f && attackTimer <= 0.0f && distance < 540.0f) {
            attackTimer = RandomService.real(1.8f, 2.5f);
            shoot(ctx);
        }
    } else if (kindId == Kind::Summoner) {
        // 召唤类按远程处理：保持距离、绕圈，并躲玩家弹幕。
        if (distance > 340.0f) {
            desired = direction * speed;
        } else if (distance < 250.0f) {
            desired = -direction * speed;
        } else {
            desired = perpendicular * speed * 0.5f * float(strafe);
        }
    } else if (kindId == Kind::Amplifier || kindId == Kind::Linker
               || kindId == Kind::Mender || kindId == Kind::Necromancer) {
        // 增益类：朝友军最密集的位置靠拢；没有友军时才保持距离。
        const Math::Vec2 cluster = allyClusterDirection(ctx);
        if (cluster.lengthSquared() > 1.0f) {
            desired = cluster.normalized() * speed;
        } else if (distance > 300.0f) {
            desired = direction * speed;
        } else if (distance < 210.0f) {
            desired = -direction * speed;
        } else {
            desired = perpendicular * speed * 0.55f * float(strafe);
        }
        if (distance < 160.0f) {
            desired -= direction * speed * 0.8f;
        }
        if (kindId == Kind::Amplifier) {
            applyAura(ctx);
        } else if (kindId == Kind::Mender) {
            healAllies(ctx, dt);
        }
    } else if (kindId == Kind::Rusher) {
        desired = direction * speed;
        lungeTimer -= dt;
        if (lungeTimer <= 0.0f) {
            lungeTimer = lungeInterval;
            vel = direction * speed * 2.4f;
            ctx.entities.spawnBurst(ctx, pos, color, 5, 90.0f, 0.25f);
        }
    } else {
        float weave = 0.0f;
        if (kindId == Kind::Drone || kindId == Kind::Mini) {
            weave = std::sin(age * 3.1f + phase) * 0.32f;
        }
        desired = direction.rotated(weave) * speed;
    }

    if (roleOf(kindId) == Role::Ranged) {
        dodgeTimer = std::max(0.0f, dodgeTimer - dt);
        dodgeCooldown = std::max(0.0f, dodgeCooldown - dt);
        if (dodgeTimer > 0.0f) {
            desired += dodgeDir * (speed * 1.1f);
        } else if (dodgeCooldown <= 0.0f) {
            const Math::Vec2 avoid = avoidPlayerBullets(ctx);
            if (avoid.lengthSquared() > 0.02f) {
                dodgeDir = avoid.normalized();
                dodgeTimer = 0.16f;
                dodgeCooldown = 1.2f;
            }
        }
    }
    const float limit = speed * 1.7f;
    if (desired.lengthSquared() > limit * limit) {
        desired = desired.normalized() * limit;
    }

    vel += (desired - vel) * std::min(1.0f, dt * 4.8f);
    pos += vel * dt;
    pos.x = Math::clamp(pos.x, GameConfig::FieldLeft + radius,
                        GameConfig::FieldRight - radius);
    pos.y = Math::clamp(pos.y, GameConfig::FieldTop + radius + 4.0f,
                        GameConfig::FieldBottom - radius - 4.0f);
}

float Enemy::damageMultiplier() const
{
    return 1.0f + (damageBuffTimer > 0.0f ? damageBuffBonus : 0.0f);
}

float Enemy::contactDamage() const
{
    return damage * damageMultiplier();
}

// amplifier：给范围内的友军刷新增伤 buff。
void Enemy::applyAura(const Context &ctx)
{
    for (const auto &other : ctx.combatants.enemies()) {
        Enemy *ally = other.get();
        if (ally == this || !ally->alive) {
            continue;
        }
        if (pos.distanceTo(ally->pos) > supportRadius) {
            continue;
        }
        ally->damageBuffBonus = std::max(ally->damageBuffBonus, supportValue);
        ally->damageBuffTimer = 0.35f;
    }
}

// mender：按秒治疗范围内的友军。
void Enemy::healAllies(const Context &ctx, float dt)
{
    bool healed = false;
    for (const auto &other : ctx.combatants.enemies()) {
        Enemy *ally = other.get();
        if (ally == this || !ally->alive || ally->hp >= ally->maxHp) {
            continue;
        }
        if (pos.distanceTo(ally->pos) > supportRadius + ally->radius) {
            continue;
        }
        ally->hp = std::min(ally->maxHp, ally->hp + supportValue * dt);
        healed = true;
    }
    healFxTimer -= dt;
    if (healed && healFxTimer <= 0.0f) {
        healFxTimer = 0.45f;
        ctx.entities.spawnBurst(ctx, pos, color, 4, 60.0f, 0.3f);
    }
}

// 增益类：找“邻居最多”的友军所在位置，作为靠拢目标。
Math::Vec2 Enemy::allyClusterDirection(const Context &ctx) const
{
    const auto &allies = ctx.combatants.enemies();
    Math::Vec2 best;
    int bestNeighbors = -1;
    float bestDistance = 0.0f;
    for (const auto &owned : allies) {
        const Enemy *candidate = owned.get();
        if (candidate == this || !candidate->alive) {
            continue;
        }
        int neighbors = 0;
        for (const auto &other : allies) {
            if (other.get() == candidate || !other->alive) {
                continue;
            }
            if (candidate->pos.distanceTo(other->pos) <= 150.0f) {
                ++neighbors;
            }
        }
        const float distance = pos.distanceTo(candidate->pos);
        if (neighbors > bestNeighbors
            || (neighbors == bestNeighbors && distance < bestDistance)) {
            bestNeighbors = neighbors;
            bestDistance = distance;
            best = candidate->pos - pos;
        }
    }
    return best;
}

// 远程类：对玩家弹幕做侧向躲避；弹道越近、越正面，权重越大。
Math::Vec2 Enemy::avoidPlayerBullets(const Context &ctx) const
{
    Math::Vec2 steer;
    for (const auto &owned : ctx.bullets.playerBullets()) {
        const Bullet &bullet = *owned;
        if (!bullet.alive()) {
            continue;
        }
        const Math::Vec2 motion = bullet.position() - bullet.prev();
        if (motion.lengthSquared() < 0.5f) {
            continue;
        }
        const Math::Vec2 dir = motion.normalized();
        const Math::Vec2 side(-dir.y, dir.x);
        const Math::Vec2 delta = pos - bullet.position();
        const float along = delta.dot(dir);
        const float lateralSigned = delta.dot(side);
        const float lateral = std::abs(lateralSigned);
        if (along < 0.0f || along > 140.0f || lateral > 34.0f) {
            continue;
        }
        const float proximity = 1.0f - std::clamp(along / 140.0f, 0.0f, 1.0f);
        const float thickness = 1.0f - std::clamp(lateral / 34.0f, 0.0f, 1.0f);
        const float strength = 0.4f + 1.4f * proximity * thickness;
        const float sign = lateralSigned >= 0.0f ? 1.0f : -1.0f;
        steer += side * (sign * strength);
        steer -= dir * (0.45f * proximity);
    }
    return steer;
}

void Enemy::shoot(const Context &ctx)
{
    if (kindId == Kind::Boss) {
        const bool enraged = hp < maxHp * 0.48f;
        fireAimed(ctx, enraged ? 3 : 2, 0.11f);
        BaseService.playSound(QStringLiteral("shoot_enemy"));
        return;
    }
    if (kindId == Kind::Spitter) {
        const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
        const float base = std::atan2(toPlayer.y, toPlayer.x);
        const Math::Vec2 aim = Math::Vec2(std::cos(base), std::sin(base))
            .rotated(RandomService.real(-0.055f, 0.055f));
        ctx.bullets.submit(std::make_unique<StandardBullet>(
            pos + aim * 18.0f, aim * data::kSpitterBullet.speed,
            data::kSpitterBullet.damage * damageMultiplier(), Base::Magenta,
            Bullet::Standpoint::Enemy, data::kSpitterBullet.radius));
        ctx.entities.spawnBurst(ctx, pos, Base::Magenta, 5, 70.0f, 0.26f);
        BaseService.playSound(QStringLiteral("shoot_enemy"));
    }
}

void Enemy::fireAimed(const Context &ctx, int count, float spread)
{
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float base = std::atan2(toPlayer.y, toPlayer.x);
    for (int i = 0; i < count; ++i) {
        const float offset = (float(i) - (count - 1) / 2.0f) * spread;
        const Math::Vec2 direction(std::cos(base + offset), std::sin(base + offset));
        ctx.bullets.submit(std::make_unique<StandardBullet>(
            pos + direction * 46.0f, direction * data::kBossAimedBullet.speed,
            data::kBossAimedBullet.damage * damageMultiplier(), QColor(255, 112, 190),
            Bullet::Standpoint::Enemy, data::kBossAimedBullet.radius));
    }
    ctx.entities.spawnBurst(ctx, pos, Base::Magenta, 8, 100.0f, 0.35f);
}

void Enemy::takeDamage(const Context &ctx, const Damage &damage)
{
    if (!alive || damage.amount <= 0.0f) {
        return;
    }

    float amount = damage.amount;
    // linker：范围内的友军受伤时，按比例把伤害转给自己（只转一层）。
    if (!damage.shared) {
        std::vector<Enemy *> linkers;
        for (const auto &other : ctx.combatants.enemies()) {
            Enemy *linker = other.get();
            if (linker == this || !linker->alive || linker->kindId != Kind::Linker
                || linker->supportRadius <= 0.0f) {
                continue;
            }
            if (linker->pos.distanceTo(pos) <= linker->supportRadius) {
                linkers.push_back(linker);
            }
        }
        if (!linkers.empty()) {
            const float ratio = std::clamp(linkers.front()->supportValue, 0.0f, 0.9f);
            const float shared = amount * ratio;
            amount -= shared;
            const float per = shared / float(linkers.size());
            for (Enemy *linker : linkers) {
                linker->takeDamage(ctx,
                                   Damage{per, false, DamageSource::Shock, true});
            }
        }
    }

    hp -= amount;
    hitFlash = 0.09f;
    const QColor textColor = damage.critical ? Base::Amber : Base::White;
    ctx.entities.floatingTexts().push_back(FloatingText{
        pos + Math::Vec2(RandomService.real(-7.0f, 7.0f), -radius - 7.0f),
        QString::number(int(amount)), textColor, 0.62f, 0.62f,
        damage.critical ? 16 : 14, damage.critical});
    ctx.entities.spawnBurst(ctx, pos, color, damage.critical ? 6 : 3, 90.0f, 0.22f);
    if (damage.source == DamageSource::Bullet
        && RandomService.real(0.0f, 1.0f) < 0.35f) {
        BaseService.playSound(QStringLiteral("hit"));
    }

    // 致死伤害不再触发 Boss 的阶段召唤：垂死的 Boss 不该再拉一波小怪。
    if (boss && hp > 0.0f) {
        ctx.state.onBossDamage(ctx, *this);
    }

    if (hp <= 0.0f) {
        die(ctx);
    }
}

void Enemy::die(const Context &ctx)
{
    if (!alive) {
        return;
    }
    ctx.events.handleKill(ctx, *this);
}

void Enemy::draw(QPainter &world, QPainter &glow, const Context &ctx) const
{
    if (!alive) {
        return;
    }
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float facing = std::atan2(toPlayer.y, toPlayer.x);
    drawBody(world, glow, facing);

    if (hitFlash > 0.0f) {
        Base::fillCircle(world, pos, radius * 0.9f,
                         Base::withAlpha(Base::White,
                                         int(110.0f * (hitFlash / 0.09f))));
    }
    if (kindId == Kind::Amplifier || kindId == Kind::Linker
        || kindId == Kind::Mender || kindId == Kind::Necromancer) {
        Base::strokeCircle(world, pos, supportRadius,
                           Base::withAlpha(color, 18), 1.0f);
    }
    if (damageBuffTimer > 0.0f) {
        Base::strokeCircle(world, pos, radius + 6.0f,
                           Base::withAlpha(Base::Amber, 120), 1.5f);
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

    if (kindId == Kind::Drone) {
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
    } else if (kindId == Kind::Spitter) {
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
    } else if (kindId == Kind::Tank) {
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
    } else if (kindId == Kind::Splitter) {
        Base::fillCircle(world, pos, r, Base::shade(color, 0.18f));
        Base::strokeCircle(world, pos, r, color, 2.0f);
        Base::strokeLine(world, pos - side * r, pos + side * r,
                         Base::shade(color, 0.75f), 1.0f);
        const Math::Vec2 offset = side * (3.0f + std::sin(age * 4.0f) * 2.0f);
        Base::fillCircle(world, pos + offset, 3.0f, color);
        Base::fillCircle(world, pos - offset, 3.0f, color);
        Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 70), 1.0f);
    } else if (kindId == Kind::Mini) {
        QPolygonF body;
        body << QPointF(pos.x + forward.x * 9.0f, pos.y + forward.y * 9.0f)
             << QPointF(pos.x + side.x * 6.0f, pos.y + side.y * 6.0f)
             << QPointF(pos.x - forward.x * 7.0f, pos.y - forward.y * 7.0f)
             << QPointF(pos.x - side.x * 6.0f, pos.y - side.y * 6.0f);
        Base::polygon(world, body, Base::shade(color, 0.18f), color, 1.0f);
        Base::fillCircle(world, pos, 2.0f, Base::White);
    } else if (kindId == Kind::Amplifier) {
        QPolygonF body;
        for (int i = 0; i < 6; ++i) {
            const float angle = age * 0.5f + Math::Tau * float(i) / 6.0f;
            body << QPointF(pos.x + std::cos(angle) * r,
                            pos.y + std::sin(angle) * r);
        }
        Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
        Base::strokeLine(world, pos - side * (r * 0.5f), pos + side * (r * 0.5f),
                         Base::shade(color, 0.9f), 2.0f);
        Base::strokeLine(world, pos - forward * (r * 0.5f),
                         pos + forward * (r * 0.5f), Base::shade(color, 0.9f), 2.0f);
        Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 80), 1.0f);
    } else if (kindId == Kind::Linker) {
        QPolygonF body;
        const float rotation = age * 0.4f;
        for (int i = 0; i < 4; ++i) {
            const float angle = rotation + Math::Tau * float(i) / 4.0f;
            body << QPointF(pos.x + std::cos(angle) * (r * 1.15f),
                            pos.y + std::sin(angle) * (r * 1.15f));
        }
        Base::polygon(world, body, Base::shade(color, 0.18f), color, 2.0f);
        Base::fillCircle(world, pos, r * 0.34f, Base::shade(color, 1.4f));
        Base::strokeLine(world, pos - side * (r + 5.0f), pos + side * (r + 5.0f),
                         Base::withAlpha(color, 150), 1.0f);
    } else if (kindId == Kind::Mender) {
        Base::fillCircle(world, pos, r, Base::shade(color, 0.16f));
        Base::strokeCircle(world, pos, r, color, 2.0f);
        const float arm = r * 0.62f;
        Base::strokeLine(world, {pos.x - arm, pos.y}, {pos.x + arm, pos.y},
                         Base::shade(color, 1.1f), 4.0f);
        Base::strokeLine(world, {pos.x, pos.y - arm}, {pos.x, pos.y + arm},
                         Base::shade(color, 1.1f), 4.0f);
        Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 70), 1.0f);
    } else if (kindId == Kind::Rusher) {
        const Math::Vec2 tip = pos + forward * (r * 1.7f);
        const Math::Vec2 left = pos - forward * (r * 0.8f) + side * (r * 0.85f);
        const Math::Vec2 right = pos - forward * (r * 0.8f) - side * (r * 0.85f);
        QPolygonF body;
        body << QPointF(tip.x, tip.y) << QPointF(left.x, left.y)
             << QPointF(right.x, right.y);
        Base::polygon(world, body, Base::shade(color, 0.2f), color, 1.5f);
        for (int i = 1; i <= 3; ++i) {
            const Math::Vec2 back = pos - forward * (r * (1.0f + i * 0.8f));
            Base::strokeLine(world, back - side * (r * 0.4f),
                             back + side * (r * 0.4f),
                             Base::withAlpha(color, 110 - i * 25), 1.0f);
        }
    } else if (kindId == Kind::Necromancer) {
        Base::fillCircle(world, pos, r, Base::shade(color, 0.14f));
        Base::strokeCircle(world, pos, r, color, 2.0f);
        for (int i = 0; i < 3; ++i) {
            const float angle = -age * 1.4f + Math::Tau * float(i) / 3.0f;
            const Math::Vec2 dot = pos + Math::Vec2(std::cos(angle),
                                                    std::sin(angle)) * (r + 6.0f);
            Base::fillCircle(world, dot, 3.0f, Base::shade(color, 1.25f));
        }
        Base::fillCircle(world, pos, r * 0.28f, QColor(245, 235, 255));
        Base::strokeCircle(world, pos, r + 12.0f, Base::withAlpha(color, 55), 1.0f);
    } else if (kindId == Kind::Summoner) {
        QPolygonF body;
        for (int i = 0; i < 6; ++i) {
            const float angle = Math::Tau * float(i) / 6.0f + Math::Pi / 6.0f;
            body << QPointF(pos.x + std::cos(angle) * r,
                            pos.y + std::sin(angle) * r);
        }
        Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
        Base::fillCircle(world, pos, r * 0.36f, Base::shade(color, 0.55f));
        const float progress = chargeRequired > 0
                                   ? Math::clamp(float(charge) / float(chargeRequired),
                                                 0.0f, 1.0f)
                                   : 0.0f;
        Base::arcRadians(world, pos, r + 7.0f, -Math::Pi / 2.0f,
                         Math::Tau * progress, Base::shade(color, 1.2f), 3.0f);
    } else if (kindId == Kind::Boss) {
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
