#include "Games/Games.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Shop/Shop.h"

#include <Qt>

#include <algorithm>

// 构造即开局：GameSimulation 接收一局的安排，Games 只负责表现层初始化。
// 新的一局就是一个新对象，因此不存在需要手工清空的跨局状态。
// Base / Random / Shop 都是进程级单例（BaseService / RandomService / ShopService）。
Games::Games(const Level &level)
    : GameSimulation(level), renderer_()
{
    ShopService.score().beginRun();
    renderer_.initialize();
    BaseService.setMouseActive(true);
    BaseService.playSound(QStringLiteral("ui"));
    renderer_.flash(Base::Cyan, 35);
    start();
}

Games::~Games() = default;

void Games::update(float dt)
{
    if (state_ != State::Paused) {
        renderer_.update(dt);
    }
    setInput(localInput());
    GameSimulation::update(dt);
}

void Games::render()
{
    renderer_.render(*this, arena_, waves_, upgrades_);
}

arena::InputState Games::localInput() const
{
    arena::InputState input;
    input.moveAxis = BaseService.movementAxis();
    input.mouseActive = BaseService.mouseActive();
    input.mousePosition = BaseService.mousePosition();
    input.shooting = BaseService.shootingHeld();
    return input;
}

void Games::onWaveStarted(int wave, bool boss, bool finalWave)
{
    // 横幅按“是不是最后一波”和“这一波有没有 Boss”两件事分别决定，互不绑定。
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

void Games::onUpgradeOpened()
{
    renderer_.flash(Base::Cyan, 28);
}

void Games::onUpgradeSelected(const QColor &color)
{
    renderer_.flash(color, 35);
}

void Games::onBossDown()
{
    renderer_.shake(12.0f);
    renderer_.flash(Base::White, 90);
    renderer_.showBanner(QStringLiteral("目标已摧毁"),
                         QStringLiteral("虚空吞噬者停止响应"), 1.8f,
                         Base::Cyan, QStringLiteral("BOSS DOWN"));
}

void Games::onWaveCleared(bool finalWave)
{
    if (finalWave) {
        return;
    }
    renderer_.showBanner(QStringLiteral("波次清理完毕"),
                         QStringLiteral("正在同步强化协议"),
                         1.4f, Base::Green, QStringLiteral("SECTOR CLEAR"));
}

void Games::onVictory()
{
    renderer_.shake(18.0f);
    renderer_.flash(Base::White, 130);
    renderer_.showBanner(QStringLiteral("区域已肃清"),
                         QStringLiteral("台本中的敌方信号全部消失"),
                         1.6f, Base::Cyan, QStringLiteral("SECTOR CLEAR"));
    BaseService.playSound(QStringLiteral("victory"));
}

void Games::onVictoryFinished()
{
    renderer_.burstConfetti();
}

void Games::onDefeat()
{
    renderer_.shake(20.0f);
    renderer_.flash(Base::Red, 120);
    BaseService.playSound(QStringLiteral("defeat"));
}

void Games::onFeedback(const Arena::Feedback &feedback)
{
    if (feedback.hasFlash) {
        renderer_.flash(feedback.color, feedback.alpha);
    }
    if (feedback.shake > 0.0f) {
        renderer_.shake(feedback.shake);
    }
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
            requestDash();
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
