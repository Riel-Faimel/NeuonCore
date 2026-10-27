#pragma once

#include "Arena/Arena.h"
#include "Arena/Combatants/Player.h"
#include "Games/Upgrades/Upgrades.h"
#include "Games/Waves/Waves.h"
#include "Net/Snapshot.h"

#include <QColor>
#include <QString>

class SelfTest;

namespace games {

// GameSimulation 组件：一局游戏里可独立于表现层推进的模拟部分。
//
// 拥有 Arena / Waves / Upgrades、对局状态、运行时间、胜负与清波计时；
// 只推进模拟，不持有也不调用 Renderer，也不读取 BaseService 的键鼠状态。
// 表现（渲染、音效、横幅、震屏、彩带）通过 on* 虚函数钩子向上发出：
// 默认实现为空，因此 GameSimulation 可以脱离 Games/Renderer 单独 update。
// 单机时 Games 继承它、实现钩子；将来服务端可以直接使用它并广播快照。
class GameSimulation {
public:
    // 一局的安排：展示信息 + 起始波次 + 运行时提交的台本。
    struct Level {
        QString name;
        QString subtitle;
        int startWave = 1;
        games::Script script;
        int enemyLevel = 1;
    };

    enum class State {
        Playing,
        Upgrade,
        Paused,
        Victory,
        Defeat,
    };

    explicit GameSimulation(const Level &level);
    virtual ~GameSimulation();

    GameSimulation(const GameSimulation &) = delete;
    GameSimulation &operator=(const GameSimulation &) = delete;

    // 从 Level.startWave 开始第一波；由适配器在自身构造完成后调用一次。
    void start();

    // 推进模拟。只改模拟状态，不渲染。
    void update(float dt);

    // 输入从这里进入模拟：Player 只读这份状态，不再直接读 BaseService。
    void setInput(const arena::InputState &input);
    const arena::InputState &input() const { return input_; }

    // 请求冲刺；本地按键和测试都走这里（等价于输入里的 dashRequested）。
    void requestDash();

    // 网络服务端/适配器使用的权威动作入口；校验由模拟层负责。
    void placeMine(const Math::Vec2 &position);
    void setPaused(bool paused);
    // 0..2 对应三张强化卡；状态不是 Upgrade 或索引非法时返回 false。
    bool chooseUpgrade(int index);

    State state() const { return state_; }
    float runTime() const { return runTime_; }
    int currentWave() const { return waves_.currentWave(); }
    int totalWaves() const { return waves_.totalWaves(); }
    const Level &level() const { return level_; }

    // ----- 网络快照（阶段 2） -----
    // 服务端边界：把一局模拟提取成只读数据；不传任何对象指针/RTTI。
    net::Snapshot snapshot() const;
    // 复用 out 的 vector 容量，单机每帧调用不反复分配。
    void writeSnapshot(net::Snapshot &out) const;

protected:
    friend class ::SelfTest;

    // ----- 表现钩子（默认空实现，模拟层不依赖任何表现代码） -----
    virtual void onWaveStarted(int wave, bool boss, bool finalWave) {
        Q_UNUSED(wave);
        Q_UNUSED(boss);
        Q_UNUSED(finalWave);
    }
    virtual void onUpgradeOpened() {}
    virtual void onUpgradeSelected(const QColor &color) { Q_UNUSED(color); }
    virtual void onBossDown() {}
    virtual void onWaveCleared(bool finalWave) { Q_UNUSED(finalWave); }
    virtual void onVictory() {}
    virtual void onVictoryFinished() {}
    virtual void onDefeat() {}
    virtual void onFeedback(const Arena::Feedback &feedback) { Q_UNUSED(feedback); }

    // ----- 局内流程（测试通过 friend 直接调用） -----
    void startWave(int wave);
    void enterUpgrade();
    void selectUpgrade(int index);
    void checkWaveClear(float dt);
    void triggerVictory();
    void finishVictory();
    void triggerDefeat();

    Level level_;
    arena::InputState input_;
    Arena arena_;
    Waves waves_;
    Upgrades upgrades_;

    State state_ = State::Playing;
    float runTime_ = 0.0f;
    bool victoryPending_ = false;
    float victoryTimer_ = 0.0f;
    float waveClearTimer_ = 0.0f;
    bool waveClearAnnounced_ = false;
    bool bossDownAnnounced_ = false;
};

} // namespace games
