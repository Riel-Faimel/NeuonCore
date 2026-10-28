#pragma once

#include "Debug.h"

#include "Arena/Arena.h"
#include "View/GameView.h"
#include "Renderer/Renderer.h"
#include "Simulation/GameSimulation.h"
#include "Net/Snapshot.h"
#include "Shop/Shop.h"

#include <QImage>
#include <QString>

DebugInfo(class SelfTest;)

// Games 模块：单机适配器。
// 它继承 GameSimulation（Arena / Waves / Upgrades、状态、计时、输入），
// 自己只负责表现：games::Renderer、BaseService 音效，以及把表现钩子接到渲染器上。
// Menu 在开局时构造它；局外只能读到这一局的结果（分数/波次/时间）。
class Games : public games::GameSimulation {
public:
    using State = games::GameSimulation::State;
    using Level = games::GameSimulation::Level;

    explicit Games(const Level &level);
    ~Games() override;

    Games(const Games &) = delete;
    Games &operator=(const Games &) = delete;

    void update(float dt);
    void render();

    int runScore() const { return ShopService.score().run(); }
    int kills() const { return ShopService.score().kills(); }
    const QImage &frame() const { return renderer_.frame(); }

    // 单机也走 GameSimulation -> Snapshot -> GameView；测试可检查重建后的视图。
    const client::GameView &view() const { return view_; }

    void keyPress(int key, bool autoRepeat = false);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mousePressRight(const Math::Vec2 &position);
    void mouseReleaseLeft();

private:
    DebugInfo(friend class SelfTest;)

    // 单机输入采集：BaseService -> arena::InputState -> GameSimulation。
    arena::InputState localInput() const;
    void handleKeydown(int key);

    // GameSimulation 的表现钩子：模拟层不依赖 Renderer / 音效。
    void onWaveStarted(int wave, bool boss, bool finalWave) override;
    void onUpgradeOpened() override;
    void onUpgradeSelected(const QColor &color) override;
    void onBossDown() override;
    void onWaveCleared(bool finalWave) override;
    void onVictory() override;
    void onVictoryFinished() override;
    void onDefeat() override;
    void onFeedback(const Arena::Feedback &feedback) override;

    games::Renderer renderer_;
    client::GameView view_;
    net::Snapshot snapshot_;
};
