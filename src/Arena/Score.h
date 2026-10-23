#pragma once

class SelfTest;

namespace arena {

// 分数与击杀计数是 Arena 模块的持久状态组件。
class Score {
public:
    void reset();
    void addKill(int points);

    int score() const { return total_; }
    int kills() const { return kills_; }

private:
    friend class ::SelfTest;

    int total_ = 0;
    int kills_ = 0;
};

} // namespace arena
