#pragma once

#include <QString>

#include <vector>

class Arena;
class Random;
class SelfTest;

namespace games {

// Waves 组件：封装波次表、出生队列和推进节奏。
// 工具：Random（洗牌）、Arena（生成传送门/统计剩余敌人）。
// 封装：敌人组合、Boss 波判定、出生间隔。
// 暴露：开始/推进波次、当前波、总波数、剩余威胁、是否清空。
class Waves {
public:
    Waves(Random &random, Arena &arena);

    void reset();
    void start(int wave);
    void update(float dt);

    int currentWave() const { return currentWave_; }
    int totalWaves() const { return TotalWaves; }
    int totalEnemies() const { return waveTotal_; }
    int remainingThreats() const;
    bool isCleared() const;
    bool isFinalWave() const;
    bool isBossWave() const;

private:
    friend class ::SelfTest;

    struct Composition {
        QString kind;
        int count = 0;
    };

    std::vector<QString> buildPlan(int wave) const;

    static constexpr int TotalWaves = 8;

    Random &random_;
    Arena &arena_;
    std::vector<QString> queue_;
    int currentWave_ = 0;
    int waveTotal_ = 0;
    float spawnTimer_ = 0.0f;
};

} // namespace games
