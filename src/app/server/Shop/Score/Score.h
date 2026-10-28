#pragma once

#include "Debug.h"

#include <QString>

DebugInfo(class SelfTest;)

namespace shop {

// Score 组件：全局积分（商店货币）与单局击杀统计。
// 属于 Shop 全局模块；积分跨局累计，写入 exe 同级的 settings.ini（[score] total）。
class Score {
public:
    void load();
    void save();

    void beginRun();
    void addKill(int points);   // 击杀：全局 + 本局 + 击杀数
    void add(int points);       // 直接加分（奖励 / 调试）
    bool spend(int points);     // 商店消费；余额不足返回 false

    int total() const { return total_; }             // 全局积分（钱包）
    int run() const { return total_ - runStart_; }   // 本局获得
    int kills() const { return kills_; }             // 本局击杀

private:
    DebugInfo(friend class ::SelfTest;)


    int total_ = 0;
    int runStart_ = 0;
    int kills_ = 0;
};

} // namespace shop
