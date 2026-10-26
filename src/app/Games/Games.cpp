#include "Games/Games.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Shop/Shop.h"

#include <Qt>

#include <algorithm>

// 构造即开局：一局的安排（含台本）在这里被接收，之后只对内使用。
// 新的一局就是一个新对象，因此不存在需要手工清空的跨局状态。
// Base / Random / Shop 都是进程级单例（BaseService / RandomService / ShopService），
// 不再作为构造参数传入；Games 只剩"这一局的安排"这一个输入。
Games::Games(const Level &level)
    : level_(level), arena_(), waves_(arena_, level_.script), upgrades_(arena_),
      renderer_()
{
    ShopService.score().beginRun();
    renderer_.initialize();
    BaseService.setMouseActive(true);
    BaseService.playSound(QStringLiteral("ui"));
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
    bossDownAnnounced_ = false;
    // 横幅按“是不是最后一波”和“这一波有没有 Boss”两件事分别决定，互不绑定。
    const bool boss = waves_.isBossWave();
    const bool finalWave = waves_.isFinalWave();
    const QString title = finalWave ? QStringLiteral("最终波次")
                                    : QStringLiteral("第 %1 波").arg(wave);
    if (boss) {
        renderer_.showBanner(title, QStringLiteral("警告：检测到「虚空吞噬者」"),
                             2.8f, Base::Magenta,
                             finalWave ? QStringLiteral("FINAL WAVE")
                                       : QStringLiteral("BOSS SIGNAL"));
        BaseService.playSound(QStringLiteral("boss"));
    } else {
        renderer_.showBanner(title, QStringLiteral("敌方信号正在接近"),
                             2.15f, Base::Cyan);
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
        // Boss 只是一个强敌：击杀它不代表这一局结束，胜负只由台本是否打完决定。
        if (arena_.bossDefeated() && !bossDownAnnounced_) {
            bossDownAnnounced_ = true;
            renderer_.shake(12.0f);
            renderer_.flash(Base::White, 90);
            renderer_.showBanner(QStringLiteral("目标已摧毁"),
                                 QStringLiteral("虚空吞噬者停止响应"), 1.8f,
                                 Base::Cyan, QStringLiteral("BOSS DOWN"));
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
    renderer_.showBanner(QStringLiteral("区域已肃清"),
                         QStringLiteral("台本中的敌方信号全部消失"),
                         1.6f, Base::Cyan, QStringLiteral("SECTOR CLEAR"));
    BaseService.playSound(QStringLiteral("victory"));
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
    BaseService.playSound(QStringLiteral("defeat"));
}

void Games::keyPress(int key, bool autoRepeat)
{
    BaseService.keyPress(key);
    if (autoRepeat) {
        return;
    }
    handleKeydown(key);
}

void Games::keyRelease(int key)
{
    BaseService.keyRelease(key);
}

void Games::mouseMove(const Math::Vec2 &position)
{
    BaseService.mouseMove(position);
}

void Games::mousePressRight(const Math::Vec2 &position)
{
    if (state_ != State::Playing) {
        return;
    }
    if (position.x < GameConfig::FieldLeft || position.x > GameConfig::FieldRight
        || position.y < GameConfig::FieldTop || position.y > GameConfig::FieldBottom) {
        return;
    }
    if (!ShopService.inventory().takeMine()) {
        BaseService.playSound(QStringLiteral("ui"));
        return;
    }
    ShopService.inventory().save();
    arena_.placeMine(position);
}

void Games::mousePressLeft(const Math::Vec2 &position)
{
    BaseService.mousePressLeft(position);
    if (state_ == State::Upgrade) {
        const int index = renderer_.upgradeIndexAt(position);
        if (index >= 0) {
            selectUpgrade(index);
        }
    }
}

void Games::mouseReleaseLeft()
{
    BaseService.mouseReleaseLeft();
}

void Games::handleKeydown(int key)
{
    if (state_ == State::Playing) {
        if (key == Qt::Key_Escape || key == Qt::Key_P) {
            state_ = State::Paused;
            BaseService.playSound(QStringLiteral("ui"));
        } else if (key == Qt::Key_Space) {
            arena_.requestDash();
        }
    } else if (state_ == State::Paused) {
        if (key == Qt::Key_Escape || key == Qt::Key_P) {
            state_ = State::Playing;
            BaseService.playSound(QStringLiteral("ui"));
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
