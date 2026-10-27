#include "Games/Simulation/GameSimulation.h"

#include "Net/Snapshot.h"

#include <algorithm>

namespace games {

namespace {

net::RunState toRunState(GameSimulation::State state)
{
    switch (state) {
    case GameSimulation::State::Playing:
        return net::RunState::Playing;
    case GameSimulation::State::Upgrade:
        return net::RunState::Upgrade;
    case GameSimulation::State::Paused:
        return net::RunState::Paused;
    case GameSimulation::State::Victory:
        return net::RunState::Victory;
    case GameSimulation::State::Defeat:
        return net::RunState::Defeat;
    }
    return net::RunState::Playing;
}

} // namespace

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

net::Snapshot GameSimulation::snapshot() const
{
    net::Snapshot out;
    writeSnapshot(out);
    return out;
}

void GameSimulation::writeSnapshot(net::Snapshot &out) const
{
    out.clear();
    out.state = toRunState(state_);
    out.runTime = runTime_;
    out.score = arena_.score();
    out.kills = arena_.kills();

    arena_.writeSnapshot(out);

    out.waves.currentWave = waves_.currentWave();
    out.waves.totalWaves = waves_.totalWaves();
    out.waves.totalEnemies = waves_.totalEnemies();
    out.waves.remainingThreats = waves_.remainingThreats();
    out.waves.bossWave = waves_.isBossWave();
    out.waves.finalWave = waves_.isFinalWave();
    out.waves.cleared = waves_.isCleared();

    out.upgrades.clear();
    out.upgrades.reserve(upgrades_.offers().size());
    for (const Upgrades::Offer &offer : upgrades_.offers()) {
        net::UpgradeOfferSnapshot data;
        data.key = offer.key;
        data.name = offer.name;
        data.tag = offer.tag;
        data.desc = offer.desc;
        data.valueText = offer.valueText;
        data.color = offer.color;
        data.level = offer.level;
        data.value = offer.value;
        out.upgrades.push_back(std::move(data));
    }
}

} // namespace games
