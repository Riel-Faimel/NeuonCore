#pragma once

#include "Arena/Arena.h"
#include "Random.h"
#include "Renderer.h"
#include "Games/Upgrades/Upgrades.h"
#include "Games/Waves/Waves.h"
#include "Base/Base.h"

#include <QImage>

class SelfTest;

// Games 模块：只做游戏状态编排。
// 工具：Arena（战斗）、Waves（波次）、Upgrades（强化）、Renderer（表现）、Base（IO）、Random。
// 封装：Title/Playing/Upgrade/Paused/GameOver/Victory 状态迁移、通关计时、输出调用顺序。
// 暴露：输入转发、update/render、状态与最终帧。
class Games {
public:
    enum class State {
        Title,
        Playing,
        Upgrade,
        Paused,
        GameOver,
        Victory,
    };

    explicit Games(bool enableAudio = true);
    ~Games();

    Games(const Games &) = delete;
    Games &operator=(const Games &) = delete;

    void update(float dt);
    void render();

    State state() const { return state_; }
    float runTime() const { return runTime_; }
    const QImage &frame() const { return renderer_.frame(); }

    void keyPress(int key, bool autoRepeat = false);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mouseReleaseLeft();
    void clearInput();
    void toggleMute();
    bool muted() const { return base_.muted(); }
    bool audioEnabled() const { return base_.audioEnabled(); }

private:
    friend class SelfTest;

    void showTitle();
    void startRun();
    void startWave(int wave);
    void enterUpgrade();
    void selectUpgrade(int index);
    void checkWaveClear(float dt);
    void triggerVictory();
    void finishVictory();
    void triggerGameOver();
    void handleKeydown(int key);
    void applyFeedback(const Arena::Feedback &feedback);

    Random random_;
    Base base_;
    Arena arena_;
    games::Waves waves_;
    games::Upgrades upgrades_;
    Renderer renderer_;

    State state_ = State::Title;
    float runTime_ = 0.0f;
    bool victoryPending_ = false;
    float victoryTimer_ = 0.0f;
    float waveClearTimer_ = 0.0f;
    bool waveClearAnnounced_ = false;
};
