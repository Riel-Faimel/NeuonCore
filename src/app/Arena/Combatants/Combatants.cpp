#include "Combatants.h"

#include "Arena/Arena.h"
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

void Combatants::updatePlayer(Arena &arena, float dt)
{
    player_.update(arena, dt);
}

void Combatants::updateEnemies(Arena &arena, float dt)
{
    const size_t count = enemies_.size();
    for (size_t i = 0; i < count; ++i) {
        if (i < enemies_.size()) {
            Fighter &fighter = *enemies_[i];
            if (fighter.isAlive()) {
                fighter.update(arena, dt);
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

void Combatants::draw(QPainter &world, QPainter &glow, const Arena &arena) const
{
    std::vector<const Fighter *> sorted;
    sorted.reserve(enemies_.size());
    for (const auto &enemy : enemies_) {
        if (enemy->isAlive()) {
            sorted.push_back(enemy.get());
        }
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const Fighter *a, const Fighter *b) {
                  return a->position().y < b->position().y;
              });
    for (const Fighter *fighter : sorted) {
        fighter->draw(world, glow, arena);
    }
    player_.draw(world, glow, arena);
}

void Combatants::spawnEnemy(Arena &arena, const QString &kind,
                            const Math::Vec2 &pos, int wave)
{
    auto enemy = std::make_unique<Enemy>(kind, pos, wave, arena.random_);
    Enemy *ptr = enemy.get();
    enemies_.push_back(std::move(enemy));
    if (kind == QStringLiteral("boss")) {
        boss_ = ptr;
        arena.base_.playSound(QStringLiteral("boss"));
        arena.addFeedback(Base::Magenta, 45, 10.0f);
    } else {
        arena.entities_.spawnBurst(arena, pos, ptr->color, 8, 120.0f, 0.4f);
    }
}

void Combatants::spawnMinions(Arena &arena, const Math::Vec2 &pos, int count, int wave)
{
    for (int i = 0; i < count; ++i) {
        const float angle = arena.random_.real(0.0f, Math::Tau);
        const Math::Vec2 offset = Math::Vec2(std::cos(angle), std::sin(angle))
            * arena.random_.real(60.0f, 90.0f);
        auto child = std::make_unique<Enemy>(QStringLiteral("mini"),
                                             pos + offset, wave, arena.random_);
        arena.entities_.spawnBurst(arena, child->pos, child->color, 6, 110.0f, 0.35f);
        enemies_.push_back(std::move(child));
    }
}

void Combatants::clearForVictory(Arena &arena)
{
    for (const auto &enemy : enemies_) {
        if (enemy->isAlive() && !enemy->boss) {
            enemy->alive = false;
            arena.entities_.spawnBurst(arena, enemy->pos, enemy->color,
                                       8, 130.0f, 0.45f);
        }
    }
    std::erase_if(enemies_, [](const std::unique_ptr<Enemy> &enemy) {
        return !enemy->boss;
    });
    boss_ = nullptr;
    player_.state.invuln = 3.0f;
}

void Combatants::damageEnemiesInRadius(Arena &arena, const Math::Vec2 &center,
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
            enemy->takeDamage(arena, Damage{damage, false, source});
            if (arena.state_.bossDefeated()) {
                break;
            }
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
