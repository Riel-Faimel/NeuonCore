#pragma once

#include <algorithm>
#include <random>
#include <vector>

// 随机数模块。所有随机行为都从同一个引擎取数，便于以后做固定 seed 重放。
class Random {
public:
    // 表现层重建（GameView::apply）会临时调用可能取随机数的构造函数；
    // 用这个快照/恢复对把视图重建变成对模拟随机流无影响的操作。
    using EngineState = std::mt19937;

    explicit Random(unsigned int seed = std::random_device{}());

    float real(float low, float high);
    int integer(int low, int high);
    EngineState engineState() const { return engine_; }
    void setEngineState(const EngineState &state) { engine_ = state; }

    template <typename T>
    void shuffle(std::vector<T> &values)
    {
        std::shuffle(values.begin(), values.end(), engine_);
    }

private:
    std::mt19937 engine_;
};

// 进程内唯一的随机引擎：开局、掉落、强化、画面抖动都从它取数。
// 定义在 Random.cpp；需要随机数的模块直接使用 RandomService。
extern Random RandomService;
