#pragma once

#include "Debug.h"

#include "Arena/Combatants/EnemyType.h"

#include <vector>

class Arena;
DebugInfo(class SelfTest;)

namespace games {

// 台本：一局里每一波的敌人组成。
// 它是运行时提交的数据，Waves 只负责按它生成出生队列，不再内置任何固定波次表。
// 类型在数据边界（JSON/编辑器）解析成 EnemyType，游戏逻辑不再比较字符串。
struct EnemySpawn {
    arena::EnemyType type;
    int count = 1;
};

struct Wave {
    std::vector<EnemySpawn> enemies;
};

using Script = std::vector<Wave>;

// Waves 组件：按提交的台本生成出生队列、推进出生节奏。
// 工具：Random（洗牌）、Arena（生成传送门/统计剩余敌人）。
// 封装：出生队列、Boss 波判定、出生间隔。
// 暴露：开始/推进波次、当前波、总波数、剩余威胁、是否清空。
class Waves {
public:
    Waves(Arena &arena, const Script &script, int enemyLevel = 1);

    void start(int wave);
    void update(float dt);

    int currentWave() const { return currentWave_; }
    int totalWaves() const { return int(script_.size()); }
    int totalEnemies() const { return waveTotal_; }
    int remainingThreats() const;
    bool isCleared() const;
    bool isFinalWave() const;
    bool isBossWave() const;

private:
    DebugInfo(friend class ::SelfTest;)

    std::vector<arena::EnemyType> buildQueue(int wave) const;
    bool waveHasBoss(int wave) const;

    Arena &arena_;
    const Script &script_;
    std::vector<arena::EnemyType> queue_;
    int currentWave_ = 0;
    int enemyLevel_ = 1;
    int waveTotal_ = 0;
    float spawnTimer_ = 0.0f;
};

} // namespace games
