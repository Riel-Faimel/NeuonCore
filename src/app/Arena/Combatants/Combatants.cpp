#include "Combatants.h"

#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Host.h"
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
    drawOrder_.clear();
    if (drawOrder_.capacity() < enemies_.size()) {
        drawOrder_.reserve(enemies_.size());
    }
    for (const auto &enemy : enemies_) {
        if (enemy->isAlive()) {
            drawOrder_.push_back(enemy.get());
        }
    }
    std::sort(drawOrder_.begin(), drawOrder_.end(),
              [](const Fighter *a, const Fighter *b) {
                  return a->position().y < b->position().y;
              });
    for (const Fighter *fighter : drawOrder_) {
        fighter->draw(world, glow, ctx);
    }
    player_.draw(world, glow, ctx);
}

void Combatants::spawnEnemy(const Context &ctx, const QString &kind,
                            const Math::Vec2 &pos, int wave)
{
    std::unique_ptr<Enemy> enemy = Enemy::create(kind, pos, wave);
    if (!enemy) {
        return;
    }
    Enemy *ptr = enemy.get();
    enemies_.push_back(std::move(enemy));
    if (ptr->boss) {
        boss_ = ptr;
        BaseService.playSound(QStringLiteral("boss"));
        ctx.host.addFeedback(Base::Magenta, 45, 10.0f);
    } else {
        ctx.entities.spawnBurst(ctx, pos, ptr->color, 8, 120.0f, 0.4f);
    }
}

void Combatants::spawnMinions(const Context &ctx, const Math::Vec2 &pos, int count, int wave)
{
    for (int i = 0; i < count; ++i) {
        const float angle = RandomService.real(0.0f, Math::Tau);
        const Math::Vec2 offset = Math::Vec2(std::cos(angle), std::sin(angle))
            * RandomService.real(60.0f, 90.0f);
        std::unique_ptr<Enemy> child = Enemy::create(QStringLiteral("mini"),
                                                     pos + offset, wave);
        if (!child) {
            continue;
        }
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

} // namespace arena
