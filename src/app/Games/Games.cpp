#include "Games/Games.h"

#include <Qt>

#include <algorithm>

// 构造即开局：一局的安排（含台本）在这里被接收，之后只对内使用。
// 新的一局就是一个新对象，因此不存在需要手工清空的跨局状态。
Games::Games(const Level &level, Base &base)
    : level_(level), base_(base), arena_(random_, base_),
      waves_(random_, arena_, level_.script), upgrades_(random_, arena_, base_),
      renderer_(random_, base_)
{
    renderer_.initialize();
    base_.setMouseActive(true);
    base_.playSound(QStringLiteral("ui"));
    renderer_.flash(Base::Cyan, 35);
    startWave(std::clamp(level_.startWave, 1, std::max(1, waves_.totalWaves())));
}

Games::~Games() = default;

void Games::startWave(int wave)
{
    waves_.start(wave);
    arena_.beginWave();
    waveClearTimer_ = 0.0f;
    waveClearAnnounced_ = false;
    if (waves_.isBossWave()) {
        renderer_.showBanner(QStringLiteral("最终波次"),
                             QStringLiteral("警告：检测到「虚空吞噬者」"),
                             2.8f, Base::Magenta, QStringLiteral("FINAL WAVE"));
        base_.playSound(QStringLiteral("boss"));
    } else {
        renderer_.showBanner(QStringLiteral("第 %1 波").arg(wave),
                             QStringLiteral("敌方信号正在接近"), 2.15f, Base::Cyan);
    }
}

void Games::enterUpgrade()
{
    arena_.clearProjectiles();
    if (!upgrades_.open()) {
        return;
    }
    state_ = State::Upgrade;
    renderer_.flash(Base::Cyan, 28);
}

void Games::selectUpgrade(int index)
{
    if (state_ != State::Upgrade || index < 0
        || index >= int(upgrades_.offers().size())) {
        return;
    }
    const QColor color = upgrades_.offers().at(size_t(index)).color;
    if (!upgrades_.apply(index)) {
        return;
    }
    renderer_.flash(color, 35);
    if (waves_.isFinalWave()) {
        finishVictory();
        return;
    }
    state_ = State::Playing;
    startWave(waves_.currentWave() + 1);
}

void Games::update(float dt)
{
    if (state_ != State::Paused) {
        renderer_.update(dt);
    }

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
        applyFeedback(arena_.takeFeedback());

        if (arena_.playerDefeated()) {
            triggerDefeat();
            break;
        }
        if (arena_.bossDefeated()) {
            triggerVictory();
            break;
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

void Games::render()
{
    renderer_.render(*this, arena_, waves_, upgrades_);
}

void Games::applyFeedback(const Arena::Feedback &feedback)
{
    if (feedback.hasFlash) {
        renderer_.flash(feedback.color, feedback.alpha);
    }
    if (feedback.shake > 0.0f) {
        renderer_.shake(feedback.shake);
    }
}

void Games::checkWaveClear(float dt)
{
    if (victoryPending_ || !waves_.isCleared()) {
        waveClearTimer_ = 0.0f;
        waveClearAnnounced_ = false;
        return;
    }
    if (!waveClearAnnounced_) {
        waveClearAnnounced_ = true;
        arena_.clearEnemyBullets();
        if (!waves_.isFinalWave()) {
            renderer_.showBanner(QStringLiteral("波次清理完毕"),
                                 QStringLiteral("正在同步强化协议"),
                                 1.4f, Base::Green, QStringLiteral("SECTOR CLEAR"));
        }
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

void Games::triggerVictory()
{
    if (victoryPending_ || state_ != State::Playing) {
        return;
    }
    victoryPending_ = true;
    victoryTimer_ = 1.65f;
    arena_.clearForVictory();
    arena_.celebrateVictory();
    renderer_.shake(18.0f);
    renderer_.flash(Base::White, 130);
    renderer_.showBanner(QStringLiteral("核心威胁已清除"),
                         QStringLiteral("虚空吞噬者正在崩解"),
                         1.6f, Base::Cyan, QStringLiteral("TARGET DESTROYED"));
    base_.playSound(QStringLiteral("victory"));
}

void Games::finishVictory()
{
    if (state_ == State::Victory) {
        return;
    }
    state_ = State::Victory;
    victoryPending_ = false;
    renderer_.burstConfetti();
}

void Games::triggerDefeat()
{
    if (state_ != State::Playing) {
        return;
    }
    state_ = State::Defeat;
    victoryPending_ = false;
    renderer_.shake(20.0f);
    renderer_.flash(Base::Red, 120);
    base_.playSound(QStringLiteral("defeat"));
}

void Games::keyPress(int key, bool autoRepeat)
{
    base_.keyPress(key);
    if (autoRepeat) {
        return;
    }
    handleKeydown(key);
}

void Games::keyRelease(int key)
{
    base_.keyRelease(key);
}

void Games::mouseMove(const Math::Vec2 &position)
{
    base_.mouseMove(position);
}

void Games::mousePressLeft(const Math::Vec2 &position)
{
    base_.mousePressLeft(position);
    if (state_ == State::Upgrade) {
        const int index = renderer_.upgradeIndexAt(position);
        if (index >= 0) {
            selectUpgrade(index);
        }
    }
}

void Games::mouseReleaseLeft()
{
    base_.mouseReleaseLeft();
}

void Games::handleKeydown(int key)
{
    if (state_ == State::Playing) {
        if (key == Qt::Key_Escape || key == Qt::Key_P) {
            state_ = State::Paused;
            base_.playSound(QStringLiteral("ui"));
        } else if (key == Qt::Key_Space) {
            arena_.requestDash();
        }
    } else if (state_ == State::Paused) {
        if (key == Qt::Key_Escape || key == Qt::Key_P) {
            state_ = State::Playing;
            base_.playSound(QStringLiteral("ui"));
        }
    } else if (state_ == State::Upgrade) {
        if (key == Qt::Key_1) {
            selectUpgrade(0);
        } else if (key == Qt::Key_2) {
            selectUpgrade(1);
        } else if (key == Qt::Key_3) {
            selectUpgrade(2);
        }
    }
}
