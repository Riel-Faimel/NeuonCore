#pragma once

class Arena;
class SelfTest;

namespace arena {

class Enemy;

// 游戏状态组件：Boss 阶段、Boss 是否被击败、玩家是否失败、分数/击杀数。
class State {
public:
    void reset();
    void beginWave();
    void addKill(int points);
    void onBossDamage(Arena &arena, Enemy &boss);
    void markBossDefeated();
    void markPlayerDefeated();

    int score() const { return total_; }
    int kills() const { return kills_; }
    bool bossDefeated() const { return bossDefeated_; }
    bool playerDefeated() const { return playerDefeated_; }

private:
    friend class ::SelfTest;

    int total_ = 0;
    int kills_ = 0;
    bool phase2_ = false;
    bool phase3_ = false;
    bool bossDefeated_ = false;
    bool playerDefeated_ = false;
};

} // namespace arena
