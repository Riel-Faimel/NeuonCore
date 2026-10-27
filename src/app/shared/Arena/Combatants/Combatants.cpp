#include "Combatants.h"

#include "Arena/Combatants/Enemies/MiniEnemy.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Host.h"
#include "Net/Snapshot.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

void Combatants::reset()
{
    player_.reset();
    enemies_.clear();
    boss_ = nullptr;
}

void Combatants::beginWave()
{
    player_.beginWave();
    boss_ = nullptr;
}

void Combatants::updatePlayer(const Context &ctx, float dt)
{
    player_.update(ctx, dt);
}

void Combatants::updateEnemies(const Context &ctx, float dt)
{
    const size_t count = enemies_.size();
    for (size_t i = 0; i < count; ++i) {
        if (i < enemies_.size()) {
            Fighter &fighter = *enemies_[i];
            if (fighter.isAlive()) {
                fighter.update(ctx, dt);
            }
        }
    }
}

void Combatants::cleanup()
{
    if (boss_ != nullptr && !boss_->isAlive()) {
        boss_ = nullptr;
    }
    std::erase_if(enemies_, [](const std::unique_ptr<Enemy> &enemy) {
        return !enemy->isAlive();
    });
}

void Combatants::draw(QPainter &world, QPainter &glow, const Context &ctx) const
{
    // 世界通道与辉光通道并行绘制，各自持有线程内的排序缓冲。
    thread_local std::vector<const Fighter *> drawOrder;
    drawOrder.clear();
    if (drawOrder.capacity() < enemies_.size()) {
        drawOrder.reserve(enemies_.size());
    }
    for (const auto &enemy : enemies_) {
        if (enemy->isAlive()) {
            drawOrder.push_back(enemy.get());
        }
    }
    std::sort(drawOrder.begin(), drawOrder.end(),
              [](const Fighter *a, const Fighter *b) {
                  return a->position().y < b->position().y;
              });
    for (const Fighter *fighter : drawOrder) {
        fighter->draw(world, glow, ctx);
    }
    player_.draw(world, glow, ctx);
}

void Combatants::spawnEnemy(const Context &ctx, EnemyType type,
                            const Math::Vec2 &pos, int wave, int enemyLevel)
{
    std::unique_ptr<Enemy> enemy = Enemy::create(type, pos, wave);
    if (!enemy) {
        return;
    }
    Enemy *ptr = enemy.get();
    ptr->setLevel(enemyLevel);
    enemies_.push_back(std::move(enemy));
    if (ptr->boss) {
        boss_ = ptr;
        BaseService.playSound(QStringLiteral("boss"));
        ctx.host.addFeedback(Base::Magenta, 45, 10.0f);
    } else {
        ctx.entities.spawnBurst(ctx, pos, ptr->color, 8, 120.0f, 0.4f);
    }
}

void Combatants::spawnEnemy(const Context &ctx, const QString &dataKey,
                            const Math::Vec2 &pos, int wave, int enemyLevel)
{
    if (const auto type = Enemy::typeForKey(dataKey)) {
        spawnEnemy(ctx, *type, pos, wave, enemyLevel);
    }
}

void Combatants::spawnMinions(const Context &ctx, const Math::Vec2 &pos, int count,
                              int wave, int enemyLevel)
{
    for (int i = 0; i < count; ++i) {
        const float angle = RandomService.real(0.0f, Math::Tau);
        const Math::Vec2 offset = Math::Vec2(std::cos(angle), std::sin(angle))
            * RandomService.real(60.0f, 90.0f);
        std::unique_ptr<Enemy> child = Enemy::create(
            EnemyType(typeid(MiniEnemy)), pos + offset, wave);
        if (!child) {
            continue;
        }
        child->setLevel(enemyLevel);
        ctx.entities.spawnBurst(ctx, child->pos, child->color, 6, 110.0f, 0.35f);
        enemies_.push_back(std::move(child));
    }
}

void Combatants::clearForVictory(const Context &ctx)
{
    for (const auto &enemy : enemies_) {
        if (enemy->isAlive() && !enemy->boss) {
            enemy->alive = false;
            ctx.entities.spawnBurst(ctx, enemy->pos, enemy->color,
                                    8, 130.0f, 0.45f);
        }
    }
    std::erase_if(enemies_, [](const std::unique_ptr<Enemy> &enemy) {
        return !enemy->boss;
    });
    boss_ = nullptr;
    player_.state.invuln = 3.0f;
}

void Combatants::damageEnemiesInRadius(const Context &ctx, const Math::Vec2 &center,
                                       float radius, float damage,
                                       DamageSource source)
{
    const size_t count = enemies_.size();
    for (size_t i = 0; i < count; ++i) {
        if (i >= enemies_.size()) {
            break;
        }
        Enemy *enemy = enemies_[i].get();
        if (!enemy->isAlive()) {
            continue;
        }
        if (center.distanceTo(enemy->pos) <= radius + enemy->radius) {
            const Math::Vec2 direction = enemy->pos - center;
            if (direction.lengthSquared() > 0.01f && !enemy->boss) {
                enemy->pos += direction.normalized() * 32.0f;
            }
            enemy->takeDamage(ctx, Damage{damage, false, source});
        }
    }
}

int Combatants::aliveEnemyCount() const
{
    int count = 0;
    for (const auto &enemy : enemies_) {
        if (enemy->isAlive()) {
            ++count;
        }
    }
    return count;
}

BossInfo Combatants::bossInfo() const
{
    BossInfo info;
    if (boss_ != nullptr && boss_->isAlive()) {
        info.active = true;
        info.hp = boss_->hp;
        info.maxHp = boss_->maxHp;
        info.hpDisplay = boss_->hpDisplay;
    }
    return info;
}

void Combatants::writeSnapshot(net::Snapshot &out) const
{
    const PlayerState &state = player_.state;
    net::PlayerSnapshot &player = out.player;
    player.pos = state.pos;
    player.vel = state.vel;
    player.dashDir = state.dashDir;
    player.muzzlePos = state.muzzlePos;
    player.radius = state.radius;
    player.speed = state.speed;
    player.maxHp = state.maxHp;
    player.hp = state.hp;
    player.hpDisplay = state.hpDisplay;
    player.damage = state.damage;
    player.fireCd = state.fireCd;
    player.shotTimer = state.shotTimer;
    player.bulletSpeed = state.bulletSpeed;
    player.multishot = state.multishot;
    player.pierce = state.pierce;
    player.critChance = state.critChance;
    player.shockLevel = state.shockLevel;
    player.dashCdMax = state.dashCdMax;
    player.dashCd = state.dashCd;
    player.dashTime = state.dashTime;
    player.dashBuffer = state.dashBuffer;
    player.invuln = state.invuln;
    player.fireFlash = state.fireFlash;
    player.aim = state.aim;
    player.alive = state.alive;
    player.spawnPulse = state.spawnPulse;

    out.enemies.clear();
    out.enemies.reserve(enemies_.size());
    for (const auto &owned : enemies_) {
        const Enemy &enemy = *owned;
        if (!enemy.isAlive()) {
            continue;
        }
        net::EnemySnapshot data;
        data.dataKey = Enemy::dataKeyFor(enemy.type());
        data.eid = enemy.eid;
        data.pos = enemy.pos;
        data.vel = enemy.vel;
        data.wave = enemy.wave;
        data.enemyLevel = enemy.enemyLevel;
        data.radius = enemy.radius;
        data.color = enemy.color;
        data.score = enemy.score;
        data.speed = enemy.speed;
        data.damage = enemy.damage;
        data.maxHp = enemy.maxHp;
        data.hp = enemy.hp;
        data.hpDisplay = enemy.hpDisplay;
        data.alive = enemy.alive;
        data.boss = enemy.boss;
        data.revived = enemy.revived;
        data.age = enemy.age;
        data.hitFlash = enemy.hitFlash;
        data.attackTimer = enemy.attackTimer;
        data.radialTimer = enemy.radialTimer;
        data.strafe = enemy.strafe;
        data.phase = enemy.phase;
        data.supportRadius = enemy.supportRadius;
        data.supportValue = enemy.supportValue;
        data.damageBuffTimer = enemy.damageBuffTimer;
        data.damageBuffBonus = enemy.damageBuffBonus;
        data.healFxTimer = enemy.healFxTimer;
        data.dodgeTimer = enemy.dodgeTimer;
        data.dodgeCooldown = enemy.dodgeCooldown;
        data.dodgeDir = enemy.dodgeDir;
        data.lungeInterval = enemy.lungeInterval;
        data.lungeTimer = enemy.lungeTimer;
        data.charge = enemy.charge;
        data.chargeRequired = enemy.chargeRequired;
        data.summonCount = enemy.summonCount;
        out.enemies.push_back(std::move(data));
    }

    const BossInfo info = bossInfo();
    out.boss.active = info.active;
    out.boss.hp = info.hp;
    out.boss.maxHp = info.maxHp;
    out.boss.hpDisplay = info.hpDisplay;
}

void Combatants::applySnapshot(const net::Snapshot &snapshot)
{
    const net::PlayerSnapshot &player = snapshot.player;
    PlayerState &state = player_.state;
    state.pos = player.pos;
    state.vel = player.vel;
    state.dashDir = player.dashDir;
    state.muzzlePos = player.muzzlePos;
    state.radius = player.radius;
    state.speed = player.speed;
    state.maxHp = player.maxHp;
    state.hp = player.hp;
    state.hpDisplay = player.hpDisplay;
    state.damage = player.damage;
    state.fireCd = player.fireCd;
    state.shotTimer = player.shotTimer;
    state.bulletSpeed = player.bulletSpeed;
    state.multishot = player.multishot;
    state.pierce = player.pierce;
    state.critChance = player.critChance;
    state.shockLevel = player.shockLevel;
    state.dashCdMax = player.dashCdMax;
    state.dashCd = player.dashCd;
    state.dashTime = player.dashTime;
    state.dashBuffer = player.dashBuffer;
    state.invuln = player.invuln;
    state.fireFlash = player.fireFlash;
    state.aim = player.aim;
    state.alive = player.alive;
    state.spawnPulse = player.spawnPulse;
    player_.input = InputState{};

    enemies_.clear();
    enemies_.reserve(snapshot.enemies.size());
    boss_ = nullptr;
    for (const net::EnemySnapshot &data : snapshot.enemies) {
        const auto type = Enemy::typeForKey(data.dataKey);
        if (!type) {
            continue;
        }
        std::unique_ptr<Enemy> enemy = Enemy::create(*type, data.pos, data.wave);
        if (!enemy) {
            continue;
        }
        enemy->eid = data.eid;
        enemy->pos = data.pos;
        enemy->vel = data.vel;
        enemy->wave = data.wave;
        enemy->enemyLevel = data.enemyLevel;
        enemy->radius = data.radius;
        enemy->color = data.color;
        enemy->score = data.score;
        enemy->speed = data.speed;
        enemy->damage = data.damage;
        enemy->maxHp = data.maxHp;
        enemy->hp = data.hp;
        enemy->hpDisplay = data.hpDisplay;
        enemy->alive = data.alive;
        enemy->boss = data.boss;
        enemy->revived = data.revived;
        enemy->age = data.age;
        enemy->hitFlash = data.hitFlash;
        enemy->attackTimer = data.attackTimer;
        enemy->radialTimer = data.radialTimer;
        enemy->strafe = data.strafe;
        enemy->phase = data.phase;
        enemy->supportRadius = data.supportRadius;
        enemy->supportValue = data.supportValue;
        enemy->damageBuffTimer = data.damageBuffTimer;
        enemy->damageBuffBonus = data.damageBuffBonus;
        enemy->healFxTimer = data.healFxTimer;
        enemy->dodgeTimer = data.dodgeTimer;
        enemy->dodgeCooldown = data.dodgeCooldown;
        enemy->dodgeDir = data.dodgeDir;
        enemy->lungeInterval = data.lungeInterval;
        enemy->lungeTimer = data.lungeTimer;
        enemy->charge = data.charge;
        enemy->chargeRequired = data.chargeRequired;
        enemy->summonCount = data.summonCount;
        if (enemy->boss) {
            boss_ = enemy.get();
        }
        enemies_.push_back(std::move(enemy));
    }
}

} // namespace arena
