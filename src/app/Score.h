#pragma once

class SelfTest;

// Score 模块：一局内的积分与击杀统计。
// 全局模块（和 Arena / Games 平级），由 Games 持有；
// Arena 在击杀结算时调用 addKill()，HUD 和结算只读 total() / kills()。
class Score {
public:
    void reset();
    void addKill(int points);

    int total() const { return total_; }
    int kills() const { return kills_; }

private:
    friend class ::SelfTest;

    int total_ = 0;
    int kills_ = 0;
};
