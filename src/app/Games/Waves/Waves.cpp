#include "Games/Waves/Waves.h"

#include "Arena/Arena.h"
#include "Arena/Combatants/Enemy.h"
#include "Data/GameData.h"
#include "Random.h"

#include <algorithm>

namespace games {

Waves::Waves(Arena &arena, const Script &script)
    : arena_(arena), script_(script)
{
}

void Waves::start(int wave)
{
    currentWave_ = wave;
    queue_ = buildQueue(wave);
    waveTotal_ = int(queue_.size());
    spawnTimer_ = wave > 1 ? data::kWaveTiming.laterSpawn : data::kWaveTiming.firstSpawn;
}

void Waves::update(float dt)
{
    if (queue_.empty()) {
        return;
    }
    spawnTimer_ -= dt;
    if (spawnTimer_ > 0.0f) {
        return;
    }
    const arena::EnemyType type = queue_.front();
    queue_.erase(queue_.begin());
    arena_.spawnPortal(type, currentWave_);
    if (!queue_.empty()) {
        spawnTimer_ = RandomService.real(data::kWaveTiming.gapMin, data::kWaveTiming.gapMax);
    }
}

int Waves::remainingThreats() const
{
    return int(queue_.size()) + arena_.portalCount() + arena_.aliveEnemyCount();
}

bool Waves::isCleared() const
{
    return queue_.empty() && arena_.portalCount() == 0 && arena_.aliveEnemyCount() == 0;
}

bool Waves::isFinalWave() const
{
    return currentWave_ >= totalWaves();
}

bool Waves::isBossWave() const
{
    return waveHasBoss(currentWave_);
}

bool Waves::waveHasBoss(int wave) const
{
    if (wave < 1 || wave > totalWaves()) {
        return false;
    }
    for (const EnemySpawn &spawn : script_[size_t(wave - 1)].enemies) {
        if (arena::Enemy::isBoss(spawn.type)) {
            return true;
        }
    }
    return false;
}

std::vector<arena::EnemyType> Waves::buildQueue(int wave) const
{
    std::vector<arena::EnemyType> plan;
    if (wave < 1 || wave > totalWaves()) {
        return plan;
    }
    for (const EnemySpawn &spawn : script_[size_t(wave - 1)].enemies) {
        for (int i = 0; i < spawn.count; ++i) {
            plan.push_back(spawn.type);
        }
    }
    RandomService.shuffle(plan);
    // Boss 台本里如果有 Boss，永远让它第一个出场。
    const auto boss = std::find_if(plan.begin(), plan.end(),
                                   [](arena::EnemyType type) {
                                       return arena::Enemy::isBoss(type);
                                   });
    if (boss != plan.end()) {
        const arena::EnemyType type = *boss;
        plan.erase(boss);
        plan.insert(plan.begin(), type);
    }
    return plan;
}

} // namespace games
