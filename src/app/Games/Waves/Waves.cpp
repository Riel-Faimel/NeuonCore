#include "Games/Waves/Waves.h"

#include "Arena/Arena.h"
#include "Random.h"

#include <algorithm>

namespace games {

Waves::Waves(Random &random, Arena &arena, const Script &script)
    : random_(random), arena_(arena), script_(script)
{
}

void Waves::start(int wave)
{
    currentWave_ = wave;
    queue_ = buildQueue(wave);
    waveTotal_ = int(queue_.size());
    spawnTimer_ = wave > 1 ? 1.15f : 0.95f;
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
    const QString kind = queue_.front();
    queue_.erase(queue_.begin());
    arena_.spawnPortal(kind, currentWave_);
    if (!queue_.empty()) {
        spawnTimer_ = random_.real(0.38f, 0.68f);
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
        if (spawn.kind == QStringLiteral("boss")) {
            return true;
        }
    }
    return false;
}

std::vector<QString> Waves::buildQueue(int wave) const
{
    std::vector<QString> plan;
    if (wave < 1 || wave > totalWaves()) {
        return plan;
    }
    for (const EnemySpawn &spawn : script_[size_t(wave - 1)].enemies) {
        for (int i = 0; i < spawn.count; ++i) {
            plan.push_back(spawn.kind);
        }
    }
    random_.shuffle(plan);
    // Boss 台本里如果有 Boss，永远让它第一个出场。
    if (waveHasBoss(wave)) {
        const auto it = std::find(plan.begin(), plan.end(), QStringLiteral("boss"));
        if (it != plan.end()) {
            plan.erase(it);
            plan.insert(plan.begin(), QStringLiteral("boss"));
        }
    }
    return plan;
}

} // namespace games
