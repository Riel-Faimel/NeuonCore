#pragma once

class Arena;
class SelfTest;

namespace arena {

class Enemy;

// 游戏状态组件：Boss 阶段、Boss 是否被击败、玩家是否失败（积分在 Score 模块）。
class State {
public:
    void reset();
    void beginWave();
    void onBossDamage(Arena &arena, Enemy &boss);
    void markBossDefeated();
    void markPlayerDefeated();

    bool bossDefeated() const { return bossDefeated_; }
    bool playerDefeated() const { return playerDefeated_; }

private:
    friend class ::SelfTest;

    bool phase2_ = false;
    bool phase3_ = false;
    bool bossDefeated_ = false;
    bool playerDefeated_ = false;
};

} // namespace arena
