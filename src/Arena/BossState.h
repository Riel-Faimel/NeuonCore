#pragma once

class Arena;

namespace arena {

class Enemy;

// Boss 阶段、是否已被击败，属于 Arena 的持久战斗状态。
class BossState {
public:
    void reset();
    void onDamage(Arena &arena, Enemy &boss);
    void markDefeated();

    bool defeated() const { return defeated_; }

private:
    bool phase2_ = false;
    bool phase3_ = false;
    bool defeated_ = false;
};

} // namespace arena
