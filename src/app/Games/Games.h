#pragma once

#include "Arena/Arena.h"
#include "Base/Base.h"
#include "Games/Upgrades/Upgrades.h"
#include "Games/Waves/Waves.h"
#include "Random.h"
#include "Renderer.h"
#include "Shop/Shop.h"

#include <QImage>
#include <QString>

class SelfTest;

// Games 模块：一局游戏的全部内容。
// Menu 在开局时构造它，并把这一局的安排（Level）交给它；构造即开局。
// Games 持有安排但不再对外暴露，局外只能读到这一局的结果（分数/波次/时间）。
class Games {
public:
    // 一局的安排：展示信息 + 起始波次 + 运行时提交的台本。
    struct Level {
        QString id;
        QString name;
        QString subtitle;
        int startWave = 1;
        games::Script script;
    };

    enum class State {
        Playing,
        Upgrade,
        Paused,
        Victory,
        Defeat,
    };

    Games(const Level &level, Base &base, Shop &shop);
    ~Games();

    Games(const Games &) = delete;
    Games &operator=(const Games &) = delete;

    void update(float dt);
    void render();

    State state() const { return state_; }
    float runTime() const { return runTime_; }
    int score() const { return shop_.score().total(); }
    int runScore() const { return shop_.score().run(); }
    int kills() const { return shop_.score().kills(); }
    int currentWave() const { return waves_.currentWave(); }
    int totalWaves() const { return waves_.totalWaves(); }
    const QImage &frame() const { return renderer_.frame(); }

    void keyPress(int key, bool autoRepeat = false);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mousePressRight(const Math::Vec2 &position);
    void mouseReleaseLeft();

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

    Level level_;
    Random random_;
    Base &base_;
    Shop &shop_;
    Arena arena_;
    games::Waves waves_;
    games::Upgrades upgrades_;
    Renderer renderer_;

    State state_ = State::Playing;
    float runTime_ = 0.0f;
    bool victoryPending_ = false;
    float victoryTimer_ = 0.0f;
    float waveClearTimer_ = 0.0f;
    bool waveClearAnnounced_ = false;
    bool bossDownAnnounced_ = false;
};
