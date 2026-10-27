#include "Combatants.h"

#include "Arena/Combatants/Enemies/MiniEnemy.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Host.h"
#include "Net/Snapshot.h"
#include "Random.h"
#include "Base/Base.h"
#include "GameConfig.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace arena {

void Combatants::reset()
{
    players_.clear();
    players_.emplace_back();
    players_.back().reset();
    localPlayerId_ = 0;
    enemies_.clear();
    boss_ = nullptr;
}

void Combatants::configurePlayers(const std::vector<int> &playerIds)
{
    players_.clear();
    const bool multiplayer = playerIds.size() > 1;
    for (const int id : playerIds) {
        Player player;
        if (multiplayer) {
            player.configure(id, playerColor(id), playerSpawnPosition(id), true);
        } else {
            // 单机/只有一名玩家：保持原来的出生点和 Cyan 配色。
            player.configure(id, Base::Cyan, playerSpawnPosition(1), false);
            player.state.pos = {GameConfig::ScreenWidth / 2.0f,
                                GameConfig::ScreenHeight / 2.0f + 32.0f};
            player.state.muzzlePos = player.state.pos;
            player.spawnPos = player.state.pos;
        }
        players_.push_back(std::move(player));
    }
    if (players_.empty()) {
        players_.emplace_back();
        players_.back().reset();
    }
    localPlayerId_ = 0;
    enemies_.clear();
    boss_ = nullptr;
}

void Combatants::beginWave()
{
    for (Player &player : players_) {
        if (player.state.active && player.state.alive) {
            player.beginWave();
        }
    }
    boss_ = nullptr;
}

void Combatants::updatePlayers(const Context &ctx, float dt)
{
    for (Player &player : players_) {
        if (player.state.active && player.state.alive) {
            player.update(ctx, dt);
        }
    }
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
    // 远程飞机先画，本机飞机最后画（保证自己的机身在最上层）。
    const Player *local = nullptr;
    for (const Player &player : players_) {
        if (!player.state.active) {
            continue;
        }
        if (isLocalPlayer(player.state.playerId)) {
            local = &player;
        } else {
            player.draw(world, glow, ctx);
        }
    }
    if (local != nullptr && local->state.alive) {
        local->draw(world, glow, ctx);
    }
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
    for (Player &player : players_) {
        if (player.state.active) {
            player.state.invuln = 3.0f;
        }
    }
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

int Combatants::playerCount() const
{
    int count = 0;
    for (const Player &player : players_) {
        if (player.state.active) {
            ++count;
        }
    }
    return count;
}

int Combatants::livingPlayerCount() const
{
    int count = 0;
    for (const Player &player : players_) {
        if (player.state.active && player.state.alive) {
            ++count;
        }
    }
    return count;
}

bool Combatants::hasPlayer(int playerId) const
{
    for (const Player &player : players_) {
        if (player.state.active && player.state.playerId == playerId) {
            return true;
        }
    }
    return false;
}

Player &Combatants::playerById(int playerId)
{
    for (Player &player : players_) {
        if (player.state.active && player.state.playerId == playerId) {
            return player;
        }
    }
    return players_.front();
}

const Player &Combatants::playerById(int playerId) const
{
    for (const Player &player : players_) {
        if (player.state.active && player.state.playerId == playerId) {
            return player;
        }
    }
    return players_.front();
}

Player &Combatants::playerAt(int index)
{
    const int clamped = std::clamp(index, 0, int(players_.size()) - 1);
    return players_[size_t(clamped)];
}

const Player &Combatants::playerAt(int index) const
{
    const int clamped = std::clamp(index, 0, int(players_.size()) - 1);
    return players_[size_t(clamped)];
}

const Player &Combatants::nearestLivingPlayer(const Math::Vec2 &from) const
{
    const Player *best = nullptr;
    float bestDistance = std::numeric_limits<float>::max();
    for (const Player &player : players_) {
        if (!player.state.active || !player.state.alive) {
            continue;
        }
        const float distance = from.distanceSquaredTo(player.state.pos);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = &player;
        }
    }
    if (best != nullptr) {
        return *best;
    }
    // 没有存活玩家：退回第一架 active 飞机（快照/绘制仍需要目标/引用）。
    for (const Player &player : players_) {
        if (player.state.active) {
            return player;
        }
    }
    return players_.front();
}

bool Combatants::isLocalPlayer(int playerId) const
{
    if (localPlayerId_ > 0) {
        return localPlayerId_ == playerId;
    }
    // 0/负数：没有明确的本地视角（单机/服务端）→ 第一架飞机算本地。
    for (const Player &player : players_) {
        if (player.state.active) {
            return player.state.playerId == playerId;
        }
    }
    return false;
}

bool Combatants::removePlayer(int playerId)
{
    for (Player &player : players_) {
        if (!player.state.active || player.state.playerId != playerId) {
            continue;
        }
        player.state.active = false;
        player.state.alive = false;
        player.input = InputState{};
        return true;
    }
    return false;
}

void Combatants::writeSnapshot(net::Snapshot &out) const
{
    out.players.clear();
    out.players.reserve(players_.size());
    for (const Player &player : players_) {
        if (!player.state.active) {
            continue;
        }
        const PlayerState &state = player.state;
        net::PlayerSnapshot data;
        data.playerId = state.playerId;
        data.color = state.color;
        data.pos = state.pos;
        data.vel = state.vel;
        data.dashDir = state.dashDir;
        data.muzzlePos = state.muzzlePos;
        data.radius = state.radius;
        data.speed = state.speed;
        data.maxHp = state.maxHp;
        data.hp = state.hp;
        data.hpDisplay = state.hpDisplay;
        data.damage = state.damage;
        data.fireCd = state.fireCd;
        data.shotTimer = state.shotTimer;
        data.bulletSpeed = state.bulletSpeed;
        data.multishot = state.multishot;
        data.pierce = state.pierce;
        data.critChance = state.critChance;
        data.shockLevel = state.shockLevel;
        data.dashCdMax = state.dashCdMax;
        data.dashCd = state.dashCd;
        data.dashTime = state.dashTime;
        data.dashBuffer = state.dashBuffer;
        data.invuln = state.invuln;
        data.fireFlash = state.fireFlash;
        data.aim = state.aim;
        data.alive = state.alive;
        data.active = state.active;
        data.spawnPulse = state.spawnPulse;
        data.respawnTimer = state.respawnTimer;
        out.players.push_back(std::move(data));
    }

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
    localPlayerId_ = snapshot.localPlayerId;
    if (!snapshot.players.empty()) {
        players_.clear();
        players_.reserve(snapshot.players.size());
        for (const net::PlayerSnapshot &data : snapshot.players) {
            Player player;
            PlayerState &state = player.state;
            state.playerId = data.playerId;
            state.color = data.color;
            state.pos = data.pos;
            state.vel = data.vel;
            state.dashDir = data.dashDir;
            state.muzzlePos = data.muzzlePos;
            state.radius = data.radius;
            state.speed = data.speed;
            state.maxHp = data.maxHp;
            state.hp = data.hp;
            state.hpDisplay = data.hpDisplay;
            state.damage = data.damage;
            state.fireCd = data.fireCd;
            state.shotTimer = data.shotTimer;
            state.bulletSpeed = data.bulletSpeed;
            state.multishot = data.multishot;
            state.pierce = data.pierce;
            state.critChance = data.critChance;
            state.shockLevel = data.shockLevel;
            state.dashCdMax = data.dashCdMax;
            state.dashCd = data.dashCd;
            state.dashTime = data.dashTime;
            state.dashBuffer = data.dashBuffer;
            state.invuln = data.invuln;
            state.fireFlash = data.fireFlash;
            state.aim = data.aim;
            state.alive = data.alive;
            state.active = data.active;
            state.spawnPulse = data.spawnPulse;
            state.respawnTimer = data.respawnTimer;
            player.input = InputState{};
            player.spawnPos = data.pos;
            players_.push_back(std::move(player));
        }
    }

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
