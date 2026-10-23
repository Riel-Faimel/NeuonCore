#include "Games/Waves/Waves.h"

#include "Arena/Arena.h"
#include "Random.h"

#include <algorithm>

namespace games {

Waves::Waves(Random &random, Arena &arena) : random_(random), arena_(arena) {}

void Waves::reset()
{
    queue_.clear();
    currentWave_ = 0;
    waveTotal_ = 0;
    spawnTimer_ = 0.0f;
}

void Waves::start(int wave)
{
    currentWave_ = wave;
    queue_ = buildPlan(wave);
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
    return currentWave_ >= TotalWaves;
}

bool Waves::isBossWave() const
{
    return currentWave_ == TotalWaves;
}

std::vector<QString> Waves::buildPlan(int wave) const
{
    std::vector<Composition> composition;
    switch (wave) {
    case 1:
        composition = {{QStringLiteral("drone"), 6}};
        break;
    case 2:
        composition = {{QStringLiteral("drone"), 6}, {QStringLiteral("spitter"), 3}};
        break;
    case 3:
        composition = {{QStringLiteral("drone"), 7}, {QStringLiteral("spitter"), 3},
                       {QStringLiteral("tank"), 2}};
        break;
    case 4:
        composition = {{QStringLiteral("drone"), 7}, {QStringLiteral("spitter"), 4},
                       {QStringLiteral("tank"), 2}, {QStringLiteral("splitter"), 2}};
        break;
    case 5:
        composition = {{QStringLiteral("drone"), 8}, {QStringLiteral("spitter"), 4},
                       {QStringLiteral("tank"), 3}, {QStringLiteral("splitter"), 3}};
        break;
    case 6:
        composition = {{QStringLiteral("drone"), 9}, {QStringLiteral("spitter"), 5},
                       {QStringLiteral("tank"), 3}, {QStringLiteral("splitter"), 4}};
        break;
    case 7:
        composition = {{QStringLiteral("drone"), 10}, {QStringLiteral("spitter"), 5},
                       {QStringLiteral("tank"), 4}, {QStringLiteral("splitter"), 4}};
        break;
    case 8:
        composition = {{QStringLiteral("boss"), 1}, {QStringLiteral("drone"), 6},
                       {QStringLiteral("spitter"), 4}, {QStringLiteral("tank"), 2},
                       {QStringLiteral("splitter"), 3}};
        break;
    default:
        composition = {{QStringLiteral("drone"), 10}, {QStringLiteral("spitter"), 5},
                       {QStringLiteral("tank"), 4}, {QStringLiteral("splitter"), 4}};
        break;
    }

    std::vector<QString> plan;
    for (const Composition &entry : composition) {
        for (int i = 0; i < entry.count; ++i) {
            plan.push_back(entry.kind);
        }
    }
    random_.shuffle(plan);
    if (wave == TotalWaves) {
        const auto it = std::find(plan.begin(), plan.end(), QStringLiteral("boss"));
        if (it != plan.end()) {
            plan.erase(it);
            plan.insert(plan.begin(), QStringLiteral("boss"));
        }
    }
    return plan;
}

} // namespace games
