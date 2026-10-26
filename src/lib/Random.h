#pragma once

#include <algorithm>
#include <random>
#include <vector>

// 随机数模块。所有随机行为都从同一个引擎取数，便于以后做固定 seed 重放。
class Random {
public:
    explicit Random(unsigned int seed = std::random_device{}());

    float real(float low, float high);
    int integer(int low, int high);

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
