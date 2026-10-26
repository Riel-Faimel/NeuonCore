#include "Enemy.h"

#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Entities/FloatingText.h"
#include "Arena/Events.h"
#include "Arena/State.h"
#include "Base/Base.h"
#include "Bullet.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Random.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <QHash>

namespace arena {

EnemyRegistry &enemyRegistry()
{
    static EnemyRegistry registry;
    return registry;
}

namespace {

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), int(text.size()));
}

QColor registryColor(const QString &kind)
{
    const EnemyRegistry &registry = enemyRegistry();
    const auto it = registry.find(kind.toStdString());
    return it != registry.end() ? it->second.color : Base::Coral;
}

} // namespace

int Enemy::sNextId = 1;

std::unique_ptr<Enemy> Enemy::create(const QString &kind,
                                     const Math::Vec2 &position, int wave)
{
    const EnemyRegistry &registry = enemyRegistry();
    const auto it = registry.find(kind.toStdString());
    if (it == registry.end() || it->second.factory == nullptr) {
        return nullptr;
    }
    return it->second.factory(position, wave);
}

const EnemyStats &Enemy::statsFor(const QString &kind)
{
    // 数值来自 src/json/enemies.json（编译期常量），颜色由注册表提供。
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
            stats.color = registryColor(toQString(base.kind));
            result[size_t(i)] = stats;
        }
        return result;
    }();
    // kind -> 下标查表，避免每次生敌都线性扫描并分配 QString。
    static const QHash<QString, int> indices = [] {
        QHash<QString, int> result;
        for (int i = 0; i < data::kEnemies.count; ++i) {
            result.insert(toQString(data::kEnemies.items[size_t(i)].kind), i);
        }
        return result;
    }();
    const auto it = indices.constFind(kind);
    return table[size_t(it != indices.constEnd() ? it.value() : 0)];
}

Enemy::Enemy(std::string_view kindName, Role role, Archetype archetype,
             const Math::Vec2 &position, int waveNumber)
    : kind(toQString(kindName)),
      eid(sNextId++),
      pos(position),
      wave(waveNumber),
      role_(role),
      archetype_(archetype)
{
    const EnemyStats &stats = statsFor(kind);
    radius = stats.radius;
    color = stats.color;
    score = stats.score;
    speed = stats.speed * (1.0f + data::kWaveScaling.speed * (wave - 1));
    damage = stats.damage * (1.0f + data::kWaveScaling.damage * (wave - 1));
    maxHp = stats.hp * (1.0f + data::kWaveScaling.hp * (wave - 1));
    hp = maxHp;
    hpDisplay = maxHp;
    attackTimer = RandomService.real(1.2f, 2.0f);
    radialTimer = RandomService.real(1.3f, 2.2f);
    strafe = RandomService.integer(0, 1) == 0 ? -1 : 1;
    phase = RandomService.real(0.0f, Math::Tau);
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

    Math::Vec2 desired = desiredVelocity(ctx, dt);

    // 远程类：检测玩家弹幕，短促侧闪。
    if (role_ == Role::Ranged) {
        dodgeTimer = std::max(0.0f, dodgeTimer - dt);
        dodgeCooldown = std::max(0.0f, dodgeCooldown - dt);
        if (dodgeTimer > 0.0f) {
            desired += dodgeDir * (speed * 1.1f);
        } else if (dodgeCooldown <= 0.0f) {
            const Math::Vec2 avoid = avoidPlayerBullets(ctx);
            if (avoid.lengthSquared() > 0.02f) {
                dodgeDir = avoid.normalized();
                dodgeTimer = 0.12f;
                dodgeCooldown = 1.6f;
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

    updateSpecial(ctx, dt);
}

Math::Vec2 Enemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    return toPlayer / distance * speed;
}

void Enemy::shoot(const Context &ctx)
{
    (void)ctx;
}

float Enemy::damageMultiplier() const
{
    return 1.0f + (damageBuffTimer > 0.0f ? damageBuffBonus : 0.0f);
}

float Enemy::contactDamage() const
{
    return damage * damageMultiplier();
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
            if (linker == this || !linker->alive) {
                continue;
            }
            // 只有防御型（linker）会分担伤害，先按缓存类别过滤，避免每次命中的虚调用。
            if (linker->archetype() != Archetype::Defensive) {
                continue;
            }
            const float shareRadius = linker->damageShareRadius();
            if (shareRadius <= 0.0f
                || linker->pos.distanceTo(pos) > shareRadius) {
                continue;
            }
            linkers.push_back(linker);
        }
        if (!linkers.empty()) {
            const float ratio = std::clamp(linkers.front()->damageShareRatio(),
                                           0.0f, 0.9f);
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
    if (showsAura()) {
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

} // namespace arena
