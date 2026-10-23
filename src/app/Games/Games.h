#pragma once

#include "Arena/Arena.h"
#include "Random.h"
#include "Renderer.h"
#include "Games/Upgrades/Upgrades.h"
#include "Games/Waves/Waves.h"
#include "Base/Base.h"

#include <QImage>
#include <QString>

class SelfTest;

// Games 模块：只负责一局游戏的逻辑。
// 主菜单、关卡选择、结算界面由 Menu 模块负责。
class Games {
public:
    struct Level {
        QString id;
        QString name;
        QString subtitle;
        int startWave = 1;
    };

    enum class State {
        Idle,
        Playing,
        Upgrade,
        Paused,
        Victory,
        Defeat,
    };

    explicit Games(bool enableAudio = true);
    ~Games();

    Games(const Games &) = delete;
    Games &operator=(const Games &) = delete;

    void start(const Level &level);
    void update(float dt);
    void render();

    State state() const { return state_; }
    float runTime() const { return runTime_; }
    const Level &level() const { return level_; }
    int score() const { return arena_.score(); }
    int kills() const { return arena_.kills(); }
    int currentWave() const { return waves_.currentWave(); }
    int totalWaves() const { return waves_.totalWaves(); }
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

    void startWave(int wave);
    void enterUpgrade();
    void selectUpgrade(int index);
    void checkWaveClear(float dt);
    void triggerVictory();
    void finishVictory();
    void triggerDefeat();
    void handleKeydown(int key);
    void applyFeedback(const Arena::Feedback &feedback);

    Random random_;
    Base base_;
    Arena arena_;
    games::Waves waves_;
    games::Upgrades upgrades_;
    Renderer renderer_;

    Level level_;
    State state_ = State::Idle;
    float runTime_ = 0.0f;
    bool victoryPending_ = false;
    float victoryTimer_ = 0.0f;
    float waveClearTimer_ = 0.0f;
    bool waveClearAnnounced_ = false;
};
