#pragma once

#include "Arena/Arena.h"
#include "Games/Upgrades/Upgrades.h"
#include "Net/Snapshot.h"

#include <vector>

class SelfTest;

namespace client {

// GameView：客户端的世界视图（阶段 2）。
//
// 它是 Arena 的只读镜像：apply() 从 Snapshot 重建玩家/敌人/子弹/实体，
// 然后由 games::Renderer 用现有的 Arena 绘制代码画出来。
// 它不推进模拟、不读 BaseService 输入、不修改权威状态；单机适配器每帧
// 用 GameSimulation -> Snapshot -> GameView 走同一条将来走 TCP 的路径。
class GameView {
public:
    GameView();
    ~GameView();

    GameView(const GameView &) = delete;
    GameView &operator=(const GameView &) = delete;

    // 用快照重建整个视图（不插值：本地路径每帧都是最新快照）。
    void apply(const net::Snapshot &snapshot);

    net::RunState state() const { return state_; }
    float runTime() const { return runTime_; }
    int score() const { return score_; }
    int kills() const { return kills_; }

    const Arena &world() const { return world_; }
    const arena::PlayerState &playerState() const { return world_.playerState(); }
    const arena::BossInfo &bossInfo() const { return boss_; }
    const net::WaveSnapshot &waves() const { return waves_; }
    const std::vector<games::Upgrades::Offer> &upgrades() const { return upgrades_; }

private:
    friend class ::SelfTest;

    Arena world_;
    arena::BossInfo boss_;
    net::WaveSnapshot waves_;
    std::vector<games::Upgrades::Offer> upgrades_;
    net::RunState state_ = net::RunState::Playing;
    float runTime_ = 0.0f;
    int score_ = 0;
    int kills_ = 0;
};

} // namespace client
