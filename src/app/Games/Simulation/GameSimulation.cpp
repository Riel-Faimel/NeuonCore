#include "Games/Simulation/GameSimulation.h"

#include <algorithm>

namespace games {

GameSimulation::GameSimulation(const Level &level)
    : level_(level), input_(), arena_(),
      waves_(arena_, level_.script, level_.enemyLevel), upgrades_(arena_)
{
}

GameSimulation::~GameSimulation() = default;

void GameSimulation::start()
{
    startWave(std::clamp(level_.startWave, 1, std::max(1, waves_.totalWaves())));
}

void GameSimulation::setInput(const arena::InputState &input)
{
    input_ = input;
    arena_.setInput(input_);
}

void GameSimulation::requestDash()
{
    arena_.requestDash();
}

void GameSimulation::update(float dt)
{
    switch (state_) {
    case State::Playing: {
        if (victoryPending_) {
            victoryTimer_ -= dt;
            arena_.updateVisuals(dt);
            if (victoryTimer_ <= 0.0f) {
                finishVictory();
            }
            break;
        }

        runTime_ += dt;
        arena_.update(dt);
        waves_.update(dt);
        onFeedback(arena_.takeFeedback());

        if (arena_.playerDefeated()) {
            triggerDefeat();
            break;
        }
        // Boss 只是一个强敌：击杀它不代表这一局结束，胜负只由台本是否打完决定。
        if (arena_.bossDefeated() && !bossDownAnnounced_) {
            bossDownAnnounced_ = true;
            onBossDown();
        }
        checkWaveClear(dt);
        break;
    }
    case State::Upgrade:
        break;
    case State::Paused:
        break;
    case State::Defeat:
    case State::Victory:
        arena_.updateVisuals(dt);
        break;
    }
}

void GameSimulation::startWave(int wave)
{
    waves_.start(wave);
    arena_.beginWave();
    waveClearTimer_ = 0.0f;
    waveClearAnnounced_ = false;
    bossDownAnnounced_ = false;
    // 横幅按“是不是最后一波”和“这一波有没有 Boss”两件事分别决定，互不绑定。
    onWaveStarted(wave, waves_.isBossWave(), waves_.isFinalWave());
}

void GameSimulation::enterUpgrade()
{
    arena_.clearProjectiles();
    if (!upgrades_.open()) {
        return;
    }
    state_ = State::Upgrade;
    onUpgradeOpened();
}

void GameSimulation::selectUpgrade(int index)
{
    if (state_ != State::Upgrade || index < 0
        || index >= int(upgrades_.offers().size())) {
        return;
    }
    const QColor color = upgrades_.offers().at(size_t(index)).color;
    if (!upgrades_.apply(index)) {
        return;
    }
    state_ = State::Playing;
    onUpgradeSelected(color);
    startWave(waves_.currentWave() + 1);
}

void GameSimulation::checkWaveClear(float dt)
{
    if (victoryPending_ || !waves_.isCleared()) {
        waveClearTimer_ = 0.0f;
        waveClearAnnounced_ = false;
        return;
    }
    if (!waveClearAnnounced_) {
        waveClearAnnounced_ = true;
        arena_.clearEnemyBullets();
        onWaveCleared(waves_.isFinalWave());
    }
    waveClearTimer_ += dt;
    if (waveClearTimer_ < 0.85f) {
        return;
    }
    if (waves_.isFinalWave()) {
        triggerVictory();
    } else {
        enterUpgrade();
    }
}

void GameSimulation::triggerVictory()
{
    if (victoryPending_ || state_ != State::Playing) {
        return;
    }
    victoryPending_ = true;
    victoryTimer_ = 1.65f;
    arena_.clearForVictory();
    arena_.celebrateVictory();
    onVictory();
}

void GameSimulation::finishVictory()
{
    if (state_ == State::Victory) {
        return;
    }
    state_ = State::Victory;
    victoryPending_ = false;
    onVictoryFinished();
}

void GameSimulation::triggerDefeat()
{
    if (state_ != State::Playing) {
        return;
    }
    state_ = State::Defeat;
    victoryPending_ = false;
    onDefeat();
}

} // namespace games
