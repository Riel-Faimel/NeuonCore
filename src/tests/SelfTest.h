#pragma once

// 测试专用代码：只在 -DDEBUG 构建中由 main.cpp 包含。
// 交付的 NeonCore.exe 不包含这里的任何逻辑。

#include "Menu/Menu.h"
#include "Base/Base.h"
#include "View/GameView.h"
#include "Data/GameData.h"
#include "Net/Client.h"
#include "Net/Protocol.h"
#include "Net/Server.h"
#include "Net/Snapshot.h"
#include "Random.h"
#include "Arena/Bullets/StandardBullet.h"
#include "Arena/Combatants/Enemies/EnemyTypes.h"
#include "Arena/Combatants/Enemies/BossEnemy.h"
#include "Loadout/Loadout.h"
#include "Arena/Combatants/Enemies/DroneEnemy.h"
#include "Arena/Combatants/Enemies/MiniEnemy.h"
#include "Arena/Combatants/Enemies/SpitterEnemy.h"
#include "Arena/Combatants/Enemies/TankEnemy.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QHostAddress>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <utility>

#ifdef DEBUG

// 测试专用 TCP 延迟代理：在客户端和真实服务端之间逐字节转发，双向各自延迟。
// 只在 -DDEBUG 的测试控制台里编译，release/交付构建完全不包含这段代码。
// 用它可以把握手、开局、输入和快照都放进“高延迟网络”里跑，且不引入生产代码开关。
class LatencyProxy : public QObject {
public:
    explicit LatencyProxy(QObject *parent = nullptr) : QObject(parent)
    {
        clock_.start();
        flushTimer_.setInterval(2);
        flushTimer_.setTimerType(Qt::PreciseTimer);
        connect(&flushTimer_, &QTimer::timeout, this, [this] { flushDue(); });
        connect(&server_, &QTcpServer::newConnection, this, [this] {
            acceptClients();
        });
    }

    ~LatencyProxy() override { close(); }

    LatencyProxy(const LatencyProxy &) = delete;
    LatencyProxy &operator=(const LatencyProxy &) = delete;

    bool listen(quint16 upstreamPort, int delayMs, QString *error = nullptr)
    {
        close();
        upstreamPort_ = upstreamPort;
        delayMs_ = std::max(0, delayMs);
        if (!server_.listen(QHostAddress::LocalHost, 0)) {
            if (error != nullptr) {
                *error = server_.errorString();
            }
            return false;
        }
        flushTimer_.start();
        return true;
    }

    void close()
    {
        flushTimer_.stop();
        server_.close();
        while (QTcpSocket *pending = server_.nextPendingConnection()) {
            pending->abort();
            pending->deleteLater();
        }
        QTcpSocket *sockets[2] = {client_, upstream_};
        client_ = nullptr;
        upstream_ = nullptr;
        for (QTcpSocket *socket : sockets) {
            if (socket != nullptr) {
                socket->disconnect(this);
                socket->abort();
                socket->deleteLater();
            }
        }
        toClient_.clear();
        toServer_.clear();
        clientGone_ = false;
        upstreamGone_ = false;
        accepted_ = 0;
        bytesToServer_ = 0;
        bytesToClient_ = 0;
    }

    quint16 port() const { return server_.serverPort(); }
    int delayMs() const { return delayMs_; }
    void setDelayMs(int delayMs) { delayMs_ = std::max(0, delayMs); }
    int accepted() const { return accepted_; }
    int bytesToServer() const { return bytesToServer_; }
    int bytesToClient() const { return bytesToClient_; }

private:
    struct Pending {
        QByteArray bytes;
        qint64 due = 0;
    };

    qint64 nowMs() const { return clock_.elapsed(); }

    void acceptClients()
    {
        while (QTcpSocket *socket = server_.nextPendingConnection()) {
            if (client_ != nullptr) {
                socket->abort();
                socket->deleteLater();
                continue;
            }
            ++accepted_;
            clientGone_ = false;
            upstreamGone_ = false;
            client_ = socket;
            client_->setParent(this);
            upstream_ = new QTcpSocket(this);
            connect(client_, &QTcpSocket::readyRead, this, [this] {
                queueFrom(client_, toServer_, &bytesToServer_);
            });
            connect(client_, &QTcpSocket::disconnected, this, [this] {
                clientGone_ = true;
            });
            connect(upstream_, &QTcpSocket::connected, this, [this] {
                // 上游连接建立前到达的字节仍在排队：从连接建立时开始计延迟，
                // 保证慢连接不会被“已经到期”的队列提前放行。
                const qint64 base = nowMs();
                for (Pending &pending : toServer_) {
                    pending.due = std::max(pending.due, base + delayMs_);
                }
            });
            connect(upstream_, &QTcpSocket::readyRead, this, [this] {
                queueFrom(upstream_, toClient_, &bytesToClient_);
            });
            connect(upstream_, &QTcpSocket::disconnected, this, [this] {
                upstreamGone_ = true;
            });
            connect(upstream_, &QTcpSocket::errorOccurred, this,
                    [this](QAbstractSocket::SocketError) {
                        if (upstream_ != nullptr
                            && upstream_->state() == QAbstractSocket::UnconnectedState) {
                            upstreamGone_ = true;
                        }
                    });
            upstream_->connectToHost(QHostAddress::LocalHost, upstreamPort_);
            if (!flushTimer_.isActive()) {
                flushTimer_.start();
            }
            // 客户端的 Hello 可能在 accept 事件之前就已经到达缓冲区；先排空，
            // 否则会等一个永远不会再来的 readyRead。
            if (client_->bytesAvailable() > 0) {
                queueFrom(client_, toServer_, &bytesToServer_);
            }
        }
    }

    void queueFrom(QTcpSocket *source, std::deque<Pending> &queue, int *counter)
    {
        if (source == nullptr) {
            return;
        }
        const QByteArray bytes = source->readAll();
        if (bytes.isEmpty()) {
            return;
        }
        if (counter != nullptr) {
            *counter += bytes.size();
        }
        queue.push_back(Pending{bytes, nowMs() + delayMs_});
    }

    void flushQueue(std::deque<Pending> &queue, QTcpSocket *destination)
    {
        if (destination == nullptr
            || destination->state() != QAbstractSocket::ConnectedState) {
            return;
        }
        const qint64 now = nowMs();
        while (!queue.empty() && queue.front().due <= now) {
            destination->write(queue.front().bytes);
            destination->flush();
            queue.pop_front();
        }
    }

    void flushDue()
    {
        flushQueue(toServer_, upstream_);
        flushQueue(toClient_, client_);
        // 一端关闭后先把已排队的字节送完，再关闭另一端；否则解散房间的
        // MatchEnd 会在代理里被丢掉，客户端只能看到“连接已关闭”。
        if (upstreamGone_ && toClient_.empty() && client_ != nullptr
            && client_->state() == QAbstractSocket::ConnectedState) {
            client_->disconnectFromHost();
        }
        if (clientGone_ && toServer_.empty() && upstream_ != nullptr
            && upstream_->state() == QAbstractSocket::ConnectedState) {
            upstream_->disconnectFromHost();
        }
        const bool clientDown = client_ == nullptr
                                || client_->state() == QAbstractSocket::UnconnectedState;
        const bool upstreamDown = upstream_ == nullptr
                                  || upstream_->state()
                                         == QAbstractSocket::UnconnectedState;
        // accept 之前不要停表：listen() 之后、第一个客户端到来之前队列本来就是空的。
        if (accepted_ > 0 && clientDown && upstreamDown
            && toClient_.empty() && toServer_.empty()) {
            flushTimer_.stop();
        }
    }

    QTcpServer server_;
    QTcpSocket *client_ = nullptr;
    QTcpSocket *upstream_ = nullptr;
    QTimer flushTimer_;
    QElapsedTimer clock_;
    std::deque<Pending> toClient_;
    std::deque<Pending> toServer_;
    quint16 upstreamPort_ = 0;
    int delayMs_ = 0;
    int accepted_ = 0;
    int bytesToServer_ = 0;
    int bytesToClient_ = 0;
    bool clientGone_ = false;
    bool upstreamGone_ = false;
};

class SelfTest {
public:
    static void simulate(Games &game, float seconds, bool shoot)
    {
        const int steps = int(seconds * 60.0f);
        for (int i = 0; i < steps; ++i) {
            if (game.state() == Games::State::Playing && shoot
                && game.arena_.playerState().alive) {
                game.arena_.player().state.invuln =
                    std::max(game.arena_.player().state.invuln, 2.0f);
                arena::Enemy *target = nullptr;
                float best = 1e9f;
                for (const auto &enemy : game.arena_.enemies()) {
                    if (!enemy->alive) {
                        continue;
                    }
                    const float distance = enemy->pos.distanceSquaredTo(
                        game.arena_.playerState().pos);
                    if (distance < best) {
                        best = distance;
                        target = enemy.get();
                    }
                }
                if (target != nullptr) {
                    const Math::Vec2 aim = target->pos + target->vel * 0.15f;
                    BaseService.mouseMove(aim);
                    BaseService.mousePressLeft(aim);
                }
            }
            game.update(1.0f / 60.0f);
        }
    }

    static bool autoplay(Games &game, float maxSeconds)
    {
        const int maxSteps = int(maxSeconds * 60.0f);
        for (int i = 0; i < maxSteps; ++i) {
            if (game.state() == Games::State::Victory) {
                return true;
            }
            if (game.state() == Games::State::Defeat) {
                return false;
            }
            if (game.state() == Games::State::Upgrade) {
                game.selectUpgrade(0);
                continue;
            }
            if (game.state() == Games::State::Playing) {
                game.arena_.player().state.invuln = 999.0f;
                arena::Enemy *target = nullptr;
                float best = 1e9f;
                Math::Vec2 dodge;
                for (const auto &enemy : game.arena_.enemies()) {
                    if (!enemy->alive) {
                        continue;
                    }
                    const float distance = enemy->pos.distanceSquaredTo(
                        game.arena_.playerState().pos);
                    if (distance < best) {
                        best = distance;
                        target = enemy.get();
                    }
                    const float realDistance = std::sqrt(distance);
                    if (realDistance > 1.0f && realDistance < 250.0f) {
                        const Math::Vec2 away =
                            (game.arena_.playerState().pos - enemy->pos).normalized();
                        dodge += away * ((250.0f - realDistance) / 250.0f);
                    }
                }
                if (target != nullptr) {
                    const Math::Vec2 aim = target->pos + target->vel * 0.15f;
                    BaseService.mouseMove(aim);
                    BaseService.mousePressLeft(aim);
                }
                const Math::Vec2 center(GameConfig::ScreenWidth / 2.0f,
                                        GameConfig::ScreenHeight / 2.0f);
                dodge += (center - game.arena_.playerState().pos) * 0.02f;
                if (dodge.lengthSquared() < 0.01f) {
                    dodge = Math::Vec2(1.0f, 0.0f).rotated(game.runTime());
                }
                BaseService.keyRelease(Qt::Key_A);
                BaseService.keyRelease(Qt::Key_D);
                BaseService.keyRelease(Qt::Key_W);
                BaseService.keyRelease(Qt::Key_S);
                if (dodge.x < -0.12f) {
                    BaseService.keyPress(Qt::Key_A);
                } else if (dodge.x > 0.12f) {
                    BaseService.keyPress(Qt::Key_D);
                }
                if (dodge.y < -0.12f) {
                    BaseService.keyPress(Qt::Key_W);
                } else if (dodge.y > 0.12f) {
                    BaseService.keyPress(Qt::Key_S);
                }
                if (std::sqrt(best) < 95.0f && game.arena_.playerState().dashCd <= 0.0f) {
                    game.arena_.requestDash();
                }
            }
            game.update(1.0f / 60.0f);
        }
        return false;
    }

    // 网络测试共用的一份最小台本：单波、单架无人机，保证对局状态可预期。
    static Games::Level networkTestLevel(const QString &name)
    {
        Games::Level level;
        level.name = name;
        level.startWave = 1;
        level.enemyLevel = 1;
        level.script.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});
        return level;
    }

    // 不进入 Menu 事件循环也能等待异步 socket/timer：只泵 Qt 事件，时间有上界。
    static bool pumpNetworkUntil(const std::function<bool()> &predicate, int timeoutMs)
    {
        QElapsedTimer timer;
        timer.start();
        while (!predicate() && timer.elapsed() < timeoutMs) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            QThread::msleep(1);
        }
        return predicate();
    }

    static int run()
    {
        QFile logFile(QStringLiteral("smoke.log"));
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            std::fprintf(stderr, "Cannot open smoke.log\n");
        }
        QTextStream stream(&logFile);

        auto log = [&](const QString &message) {
            stream << message << Qt::endl;
            stream.flush();
            std::printf("%s\n", message.toUtf8().constData());
            std::fflush(stdout);
        };
        auto require = [&](bool condition, const QString &message) {
            if (!condition) {
                log(QStringLiteral("  FAIL  ") + message);
                return false;
            }
            log(QStringLiteral("  ok    ") + message);
            return true;
        };

        bool success = true;
        const bool audioRequested = qEnvironmentVariableIsSet("NEON_AUDIO_TEST");
        log(QStringLiteral("Creating game (offscreen, audio %1)")
                .arg(audioRequested ? QStringLiteral("enabled") : QStringLiteral("disabled")));
        LoadoutService.select(0);
        Menu menu(audioRequested, true);
        Games *game = nullptr;
        log(QStringLiteral("Audio initialized: %1")
                .arg(menu.audioEnabled() ? QStringLiteral("yes") : QStringLiteral("no")));
        if (audioRequested) {
            menu.toggleMute();
        }

        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("boots into main menu"));
        menu.update(1.0f / 60.0f);
        menu.render();

        log(QStringLiteral("Input flow"));
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        success &= require(menu.screen() == Menu::Screen::Map,
                           QStringLiteral("enter opens the sector map"));
        menu.keyPress(Qt::Key_Right);
        menu.keyRelease(Qt::Key_Right);
        success &= require(menu.map_.selected() == 1,
                           QStringLiteral("map arrows move the selection"));
        success &= require(data::kCampaigns[0].count == 8
                               && data::kCampaigns[1].count == 10
                               && data::kCampaigns[2].count == 12
                               && data::kCampaigns[3].count == 12,
                           QStringLiteral("later campaigns get longer"));
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        game = menu.match_.game();
        success &= require(game != nullptr && game->totalWaves() == 10,
                           QStringLiteral("second region starts the level 2 campaign"));
        success &= require(game != nullptr && game->waves_.enemyLevel_ == 2,
                           QStringLiteral("level 2 enemies use level 2 scaling"));
        menu.startCampaign(3);
        game = menu.match_.game();
        success &= require(game != nullptr && game->totalWaves() == 12
                               && game->waves_.enemyLevel_ == 4,
                           QStringLiteral("final region uses the level 4 campaign"));
        menu.returnToMenu();
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game != nullptr && game->state() == Games::State::Playing,
                           QStringLiteral("enter on the first node starts the run"));
        menu.keyPress(Qt::Key_Escape);
        menu.keyRelease(Qt::Key_Escape);
        success &= require(game->state() == Games::State::Paused,
                           QStringLiteral("escape pauses"));
        menu.keyPress(Qt::Key_P);
        menu.keyRelease(Qt::Key_P);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("p resumes"));

        game->upgrades_.open();
        game->state_ = Games::State::Upgrade;
        game->renderer_.upgradeIndexAt(Math::Vec2(300, 400));
        menu.mousePressLeft(Math::Vec2(300, 400));
        menu.mouseReleaseLeft();
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("mouse click selects an upgrade"));

        menu.returnToMenu();
        log(QStringLiteral("Starting run"));
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("run starts"));
        success &= require(game->waves_.currentWave() == 1,
                           QStringLiteral("starts at wave 1"));
        simulate(*game, 7.0f, true);
        game->render();

        game->waves_.queue_.clear();
        game->arena_.portals().clear();
        game->arena_.enemies().clear();
        game->arena_.clearEnemy();
        for (int i = 0; i < 70; ++i) {
            game->update(1.0f / 60.0f);
            if (game->state() == Games::State::Upgrade) {
                break;
            }
        }
        success &= require(game->state() == Games::State::Upgrade,
                           QStringLiteral("wave clear opens upgrade screen"));
        success &= require(game->upgrades_.offers().size() == 3,
                           QStringLiteral("offers three upgrades"));
        bool rolled = true;
        for (const games::Upgrades::Offer &offer : game->upgrades_.offers()) {
            rolled = rolled && !offer.valueText.isEmpty();
            for (const games::Upgrades::Definition &definition
                 : games::Upgrades::catalog()) {
                if (definition.key != offer.key) {
                    continue;
                }
                rolled = rolled && offer.value >= definition.minValue - 0.001f
                         && offer.value <= definition.maxValue + 0.001f;
            }
        }
        success &= require(rolled,
                           QStringLiteral("upgrade values roll inside their ranges"));
        game->render();

        const Arena::PlayerState before = game->arena_.playerState();
        game->selectUpgrade(0);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("upgrade resumes play"));
        success &= require(game->waves_.currentWave() == 2,
                           QStringLiteral("advances to wave 2"));
        success &= require(before.damage != game->arena_.playerState().damage
                               || before.fireCd != game->arena_.playerState().fireCd
                               || before.maxHp != game->arena_.playerState().maxHp
                               || before.hp != game->arena_.playerState().hp
                               || before.speed != game->arena_.playerState().speed
                               || before.multishot != game->arena_.playerState().multishot
                               || before.pierce != game->arena_.playerState().pierce
                               || before.critChance != game->arena_.playerState().critChance
                               || before.shockLevel != game->arena_.playerState().shockLevel,
                           QStringLiteral("upgrade changes a stat"));

        log(QStringLiteral("Support enemies"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 1;
        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        const arena::Context supportCtx = game->arena_.context();
        supportCtx.combatants.player().state.pos = {640, 420};
        supportCtx.combatants.player().state.invuln = 99.0f;
        auto spawn = [&](const QString &kind, const Math::Vec2 &at) {
            game->arena_.spawnEnemy(supportCtx, kind, at, 1);
            return game->arena_.enemies().back().get();
        };

        arena::Enemy *amplifier = spawn(QStringLiteral("amplifier"), {500, 400});
        arena::Enemy *drone = spawn(QStringLiteral("drone"), {545, 400});
        for (int i = 0; i < 8; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(drone->damageBuffTimer > 0.0f
                               && drone->contactDamage()
                                      > arena::Enemy::statsFor(QStringLiteral("drone")).damage,
                           QStringLiteral("amplifier boosts nearby ally damage"));

        arena::Enemy *linker = spawn(QStringLiteral("linker"), {600, 400});
        drone->pos = linker->pos + Math::Vec2(40.0f, 0.0f);
        const float linkerHp = linker->hp;
        drone->takeDamage(supportCtx,
                          arena::Damage{10.0f, false, arena::DamageSource::Bullet});
        success &= require(linker->hp < linkerHp,
                           QStringLiteral("linker shares damage for nearby allies"));

        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        menu.clearInput();
        arena::Enemy *mender = spawn(QStringLiteral("mender"), {600, 470});
        arena::Enemy *tank = spawn(QStringLiteral("tank"), {640, 470});
        tank->hp = tank->maxHp * 0.3f;
        for (int i = 0; i < 60; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(tank->hp > tank->maxHp * 0.3f + 1.0f,
                           QStringLiteral("mender heals nearby allies"));

        arena::Enemy *rusher = spawn(QStringLiteral("rusher"), {800, 300});
        success &= require(
            rusher->speed > arena::Enemy::statsFor(QStringLiteral("drone")).speed * 1.8f
                && rusher->lungeInterval > 0.0f
                && rusher->contactDamage()
                       > arena::Enemy::statsFor(QStringLiteral("drone")).damage,
            QStringLiteral("rusher is fast and aggressive"));

        arena::Enemy *necromancer = spawn(QStringLiteral("necromancer"), {900, 400});
        necromancer->supportRadius = 400.0f;
        necromancer->supportValue = 1.1f;
        arena::Enemy *victim = spawn(QStringLiteral("drone"), {920, 400});
        victim->hp = 1.0f;
        victim->takeDamage(supportCtx,
                           arena::Damage{10.0f, false, arena::DamageSource::Bullet});
        bool revived = false;
        for (const auto &other : game->arena_.enemies()) {
            if (other->revived) {
                revived = true;
            }
        }
        success &= require(revived,
                           QStringLiteral("necromancer revives a fallen ally"));

        necromancer->supportRadius = 0.0f;   // 避免复活干扰召唤计数
        arena::Enemy *summoner = spawn(QStringLiteral("summoner"), {1080, 560});
        summoner->charge = 0;
        summoner->chargeRequired = 3;
        summoner->summonCount = 2;
        int minisBefore = 0;
        for (const auto &other : game->arena_.enemies()) {
            if (other->type() == typeid(arena::MiniEnemy)) {
                ++minisBefore;
            }
        }
        for (int i = 0; i < 3; ++i) {
            const auto chargedVictim =
                arena::Enemy::create(QStringLiteral("drone"), summoner->pos, 1);
            supportCtx.events.handleKill(supportCtx, *chargedVictim);
        }
        int minisAfter = 0;
        for (const auto &other : game->arena_.enemies()) {
            if (other->type() == typeid(arena::MiniEnemy)) {
                ++minisAfter;
            }
        }
        success &= require(minisAfter == minisBefore + 2 && summoner->charge == 0,
                           QStringLiteral("summoner summons allies after enough deaths"));

        summoner->charge = 0;
        for (int i = 0; i < 3; ++i) {
            const auto miniVictim =
                arena::Enemy::create(QStringLiteral("mini"), summoner->pos, 1);
            supportCtx.events.handleKill(supportCtx, *miniVictim);
        }
        int minisAfterMiniDeaths = 0;
        for (const auto &other : game->arena_.enemies()) {
            if (other->type() == typeid(arena::MiniEnemy)) {
                ++minisAfterMiniDeaths;
            }
        }
        success &= require(summoner->charge == 0
                               && minisAfterMiniDeaths == minisAfter,
                           QStringLiteral("minion deaths do not charge the summoner"));

        success &= require(
            dynamic_cast<arena::OffensiveEnemy *>(
                arena::Enemy::create(QStringLiteral("drone"), {0, 0}, 1).get())
                    != nullptr
                && dynamic_cast<arena::DefensiveEnemy *>(
                       arena::Enemy::create(QStringLiteral("tank"), {0, 0}, 1).get())
                       != nullptr,
            QStringLiteral("vtable: offensive and defensive enemies"));
        success &= require(
            dynamic_cast<arena::RangedEnemy *>(
                arena::Enemy::create(QStringLiteral("spitter"), {0, 0}, 1).get())
                    != nullptr
                && dynamic_cast<arena::BossEnemy *>(
                       arena::Enemy::create(QStringLiteral("boss"), {0, 0}, 1).get())
                       != nullptr,
            QStringLiteral("vtable: ranged enemies and boss"));
        success &= require(
            dynamic_cast<arena::SupportEnemy *>(
                arena::Enemy::create(QStringLiteral("amplifier"), {0, 0}, 1).get())
                    != nullptr
                && dynamic_cast<arena::SupportEnemy *>(
                       arena::Enemy::create(QStringLiteral("necromancer"), {0, 0}, 1).get())
                       != nullptr,
            QStringLiteral("vtable: support enemies"));
        success &= require(
            dynamic_cast<arena::SelfDestructEnemy *>(
                arena::Enemy::create(QStringLiteral("splitter"), {0, 0}, 1).get())
                != nullptr,
            QStringLiteral("vtable: self-destruct enemies"));

        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        spawn(QStringLiteral("drone"), {480, 400});
        spawn(QStringLiteral("drone"), {500, 420});
        arena::Enemy *clusterSupport = spawn(QStringLiteral("amplifier"), {920, 400});
        const float supportStartX = clusterSupport->pos.x;
        for (int i = 0; i < 45; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(clusterSupport->pos.x < supportStartX - 8.0f,
                           QStringLiteral("support enemies drift toward the ally cluster"));

        // 防御线：防御者站到后排与玩家之间，远程躲到防御者背后，辅助跟后排。
        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        menu.clearInput();
        supportCtx.combatants.player().state.invuln = 99.0f;
        const Math::Vec2 formationPlayer = supportCtx.combatants.player().state.pos;
        arena::Enemy *frontTank = spawn(QStringLiteral("tank"), {700, 420});
        arena::Enemy *backSpitter = spawn(QStringLiteral("spitter"), {900, 420});
        arena::Enemy *backMender = spawn(QStringLiteral("mender"), {900, 520});
        spawn(QStringLiteral("splitter"), {500, 420});
        for (int i = 0; i < 60; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(
            frontTank->pos.x > formationPlayer.x && frontTank->pos.x < backSpitter->pos.x,
            QStringLiteral("defensive enemies stand between backline and player"));
        success &= require(
            backSpitter->pos.x > frontTank->pos.x + 10.0f
                && backSpitter->pos.distanceTo(formationPlayer)
                       > frontTank->pos.distanceTo(formationPlayer),
            QStringLiteral("ranged enemies hide behind the defensive line"));
        success &= require(backMender->pos.x > 850.0f,
                           QStringLiteral("support enemies stay with the backline"));

        log(QStringLiteral("Boss and victory"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->arena_.beginWave();
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 8);
        success &= require(game->arena_.boss() != nullptr, QStringLiteral("boss spawns"));
        game->arena_.boss()->takeDamage(
            game->arena_.context(), arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
        success &= require(game->arena_.bossDefeated(),
                           QStringLiteral("boss death is recorded"));
        simulate(*game, 3.0f, false);
        success &= require(game->state() == Games::State::Victory,
                           QStringLiteral("victory screen reached"));
        menu.update(0.0f);
        success &= require(menu.screen() == Menu::Screen::Result,
                           QStringLiteral("menu enters victory result"));
        menu.render();

        {
            // 多人联机时最后一帧快照可能仍是 Playing/Defeat；结算必须信 MatchEnd。
            menu::Match networkResult;
            networkResult.client_ = std::make_unique<client::GameClient>();
            net::MatchEndMessage end;
            end.victory = true;
            networkResult.client_->matchEnd_ = end;
            networkResult.client_->matchEnded_ = true;
            success &= require(networkResult.state() == Games::State::Victory,
                               QStringLiteral("network result uses authoritative MatchEnd victory"));
        }

        log(QStringLiteral("Mid-script boss"));
        games::Script bossMid;
        bossMid.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});
        bossMid.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::BossEnemy)), 1}}});
        bossMid.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});
        menu.levels_.push_back({QStringLiteral("中局 Boss"),
                                QStringLiteral("测试台本"), 2, bossMid});
        menu.startLevel(int(menu.levels_.size()) - 1);
        game = menu.match_.game();
        success &= require(game->currentWave() == 2 && game->waves_.isBossWave(),
                           QStringLiteral("boss can sit in a middle wave"));
        success &= require(game->renderer_.bannerTitle_ == QStringLiteral("第 2 波")
                               && game->renderer_.bannerKicker_
                                      == QStringLiteral("BOSS SIGNAL"),
                           QStringLiteral("middle boss banner is not the final wave"));
        game->waves_.queue_.clear();
        game->arena_.enemies().clear();
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 2);
        game->arena_.boss()->takeDamage(
            game->arena_.context(), arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
        game->update(1.0f / 60.0f);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("a boss in a middle wave does not end the run"));
        simulate(*game, 1.2f, false);
        success &= require(game->state() == Games::State::Upgrade,
                           QStringLiteral("cleared boss wave opens the upgrade screen"));
        menu.returnToMenu();

        log(QStringLiteral("Game over"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->arena_.player().state.invuln = 0.0f;
        game->arena_.player().takeDamage(
            game->arena_.context(), arena::Damage{9999.0f, false, arena::DamageSource::Bullet});
        game->update(1.0f / 60.0f);
        success &= require(game->state() == Games::State::Defeat,
                           QStringLiteral("lethal damage ends run"));
        menu.update(0.0f);
        success &= require(menu.screen() == Menu::Screen::Result,
                           QStringLiteral("menu enters defeat result"));
        menu.render();
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game->state() == Games::State::Playing
                               && game->waves_.currentWave() == 1,
                           QStringLiteral("restart works"));

        log(QStringLiteral("Autoplay full 8-wave run"));
        menu.startLevel(0);
        game = menu.match_.game();
        const bool autoplayVictory = autoplay(*game, 240.0f);
        success &= require(autoplayVictory,
                           QStringLiteral("autoplay clears all 8 waves and the boss"));

        menu.returnToMenu();
        menu.update(1.0f / 60.0f);
        menu.render();
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("returns to main menu"));

        log(QStringLiteral("Custom level"));
        menu.keyPress(Qt::Key_2);
        success &= require(menu.screen() == Menu::Screen::Editor,
                           QStringLiteral("custom card opens the script editor"));
        const int editedWaves = int(menu.editor_.script().size());
        menu.keyPress(Qt::Key_N);
        menu.keyPress(Qt::Key_N);
        success &= require(int(menu.editor_.script().size()) == editedWaves + 2,
                           QStringLiteral("N appends waves"));
        menu.keyPress(Qt::Key_Right);
        const int beforeCount = menu.editor_.enemyCount(menu.editor_.row(), menu.editor_.column());
        for (int i = 0; i < 3; ++i) {
            menu.keyPress(Qt::Key_Plus);
        }
        success &= require(menu.editor_.enemyCount(menu.editor_.row(), menu.editor_.column())
                               == beforeCount + 3,
                           QStringLiteral("+ raises a cell count"));
        const int beforeHold = menu.editor_.enemyCount(menu.editor_.row(),
                                                      menu.editor_.column());
        menu.keyPress(Qt::Key_Plus);
        for (int i = 0; i < 40; ++i) {
            menu.update(1.0f / 60.0f);
        }
        menu.keyRelease(Qt::Key_Plus);
        success &= require(menu.editor_.enemyCount(menu.editor_.row(), menu.editor_.column())
                               > beforeHold + 2,
                           QStringLiteral("holding + repeats the adjustment"));
        menu.keyPress(Qt::Key_Minus);
        for (int i = 0; i < 40; ++i) {
            menu.update(1.0f / 60.0f);
        }
        menu.keyRelease(Qt::Key_Minus);

        const QRectF spitterChip = menu.editor_.paletteChipRect(1);
        const QRectF dropRow = menu.editor_.rowRect(1);
        const int beforeDragTotal = menu.editor_.totalEnemies();
        menu.mousePressLeft({float(spitterChip.center().x()),
                             float(spitterChip.center().y())});
        menu.mouseMove({float(dropRow.center().x()), float(dropRow.center().y())});
        menu.mouseReleaseLeft();
        success &= require(menu.editor_.columnCount() >= 2
                               && menu.editor_.totalEnemies() == beforeDragTotal + 1,
                           QStringLiteral("dragging from the palette adds an enemy column"));

        const menu::Editor::Config defaultSetup = menu.editor_.config();
        success &= require(menu.screen() == Menu::Screen::Editor
                               && defaultSetup.mode == menu::Editor::Mode::Single
                               && defaultSetup.port == 0
                               && defaultSetup.host.isEmpty(),
                           QStringLiteral("custom tabs default to single without a server"));

        menu.keyPress(Qt::Key_F2);
        menu.editor_.setPort(8899);
        const menu::Editor::Config hostSetup = menu.editor_.config();
        success &= require(menu.editor_.mode() == menu::Editor::Mode::Host
                               && hostSetup.mode == menu::Editor::Mode::Host
                               && hostSetup.port == 8899
                               && hostSetup.host.isEmpty(),
                           QStringLiteral("host tab exposes the server port"));
        menu.keyPress(Qt::Key_F3);
        menu.editor_.setHost(QStringLiteral("192.168.1.9"));
        menu.editor_.setPort(8899);
        const menu::Editor::Config joinSetup = menu.editor_.config();
        success &= require(menu.editor_.mode() == menu::Editor::Mode::Join
                               && joinSetup.mode == menu::Editor::Mode::Join
                               && joinSetup.port == 8899
                               && joinSetup.host == QStringLiteral("192.168.1.9"),
                           QStringLiteral("join tab exposes host and port"));

        menu.keyPress(Qt::Key_F1);
        menu.keyPress(Qt::Key_Return);
        game = menu.match_.game();
        success &= require(menu.screen() == Menu::Screen::Match
                               && game->totalWaves() == editedWaves + 2
                               && game->currentWave() == 1,
                           QStringLiteral("edited script drives the run from wave 1"));
        menu.returnToMenu();

        QTcpServer probe;
        success &= require(probe.listen(QHostAddress::LocalHost, 0),
                           QStringLiteral("custom host can reserve a test port"));
        const quint16 customHostPort = probe.serverPort();
        probe.close();
        menu.editor_.setMode(menu::Editor::Mode::Host);
        menu.editor_.setPort(customHostPort);
        menu.startCustom(menu.editor_.config());
        success &= require(menu.screen() == Menu::Screen::Room
                               && menu.match_.inRoom()
                               && menu.match_.localServer() != nullptr
                               && menu.match_.localServer()->isListening()
                               && menu.match_.localServer()->port() == customHostPort
                               && !menu.match_.localServer()->running(),
                           QStringLiteral("custom host enters a room instead of starting"));
        menu.returnToMenu();

        log(QStringLiteral("Script editor"));
        menu.keyPress(Qt::Key_2);
        success &= require(menu.editor_.columnCount() < 5,
                           QStringLiteral("editor shows only used columns"));
        menu.keyPress(Qt::Key_5);
        success &= require(menu.editor_.waveHasBoss(menu.editor_.row()),
                           QStringLiteral("editor adds the boss column"));
        menu.keyPress(Qt::Key_Minus);
        success &= require(!menu.editor_.waveHasBoss(menu.editor_.row()),
                           QStringLiteral("editor clears the boss wave"));
        menu.keyPress(Qt::Key_X);
        success &= require(int(menu.editor_.script().size()) == editedWaves + 1,
                           QStringLiteral("X removes the last wave"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves the editor"));

        const QRectF customCard = menu.main_.levelRect(Menu::CustomLevelIndex, int(menu.levels_.size()));
        menu.mousePressLeft({float(customCard.center().x()),
                             float(customCard.center().y())});
        success &= require(menu.screen() == Menu::Screen::Editor,
                           QStringLiteral("clicking the custom card opens the editor"));
        menu.keyPress(Qt::Key_Escape);

        log(QStringLiteral("Settings"));
        const QRectF gear = menu.main_.settingsButtonRect();
        menu.mousePressLeft({float(gear.center().x()), float(gear.center().y())});
        menu.mouseReleaseLeft();
        success &= require(menu.screen() == Menu::Screen::Settings,
                           QStringLiteral("settings button opens settings"));
        BaseService.setSoundVolume(0.5f);
        BaseService.setMusicVolume(0.5f);
        menu.keyPress(Qt::Key_Down);
        menu.keyPress(Qt::Key_Left);
        success &= require(BaseService.musicVolume() < 0.5f,
                           QStringLiteral("left lowers the music volume"));
        menu.keyPress(Qt::Key_Right);
        success &= require(std::abs(BaseService.musicVolume() - 0.5f) < 0.001f,
                           QStringLiteral("right raises the music volume"));
        menu.keyPress(Qt::Key_Up);
        menu.keyPress(Qt::Key_Left);
        success &= require(BaseService.soundVolume() < 0.5f,
                           QStringLiteral("left lowers the sound volume"));
        const QRectF musicTrack = menu.settings_.trackRect(1);
        menu.keyPress(Qt::Key_Down);
        menu.mousePressLeft({float(musicTrack.left()) + float(musicTrack.width()) * 0.25f,
                             float(musicTrack.center().y())});
        menu.mouseReleaseLeft();
        success &= require(std::abs(BaseService.musicVolume() - 0.25f) < 0.02f,
                           QStringLiteral("clicking the track sets the volume"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves settings"));

        log(QStringLiteral("Armory"));
        LoadoutService.select(0);
        const QRectF armoryEntry = menu.main_.armoryButtonRect();
        menu.mousePressLeft({float(armoryEntry.center().x()),
                             float(armoryEntry.center().y())});
        menu.mouseReleaseLeft();
        success &= require(menu.screen() == Menu::Screen::Armory,
                           QStringLiteral("armory button opens war prep"));
        menu.keyPress(Qt::Key_Right);
        menu.keyRelease(Qt::Key_Right);
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        success &= require(LoadoutService.selected() == 1,
                           QStringLiteral("armory equips the scatter launcher"));
        LoadoutService.select(2);
        arena::Player railPlayer;
        railPlayer.reset();
        success &= require(railPlayer.state.damage > data::kPlayer.damage
                               && railPlayer.state.fireCd > data::kPlayer.fireCd,
                           QStringLiteral("launcher changes player damage and fire rate"));
        LoadoutService.select(0);
        menu.keyPress(Qt::Key_Escape);
        menu.keyRelease(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves war prep"));

        log(QStringLiteral("Enemy levels"));
        const auto levelOne = arena::Enemy::create(
            QStringLiteral("drone"), {0, 0}, 1);
        const auto levelThree = arena::Enemy::create(
            QStringLiteral("drone"), {0, 0}, 1);
        levelThree->setLevel(3);
        success &= require(levelThree->maxHp > levelOne->maxHp * 1.3f
                               && levelThree->damage > levelOne->damage,
                           QStringLiteral("enemy levels scale stats without a level readout"));

        log(QStringLiteral("Score"));
        const int wallet = ShopService.score().total();
        ShopService.score().beginRun();
        ShopService.score().addKill(120);
        success &= require(ShopService.score().total() == wallet + 120
                               && ShopService.score().run() == 120 && ShopService.score().kills() == 1,
                           QStringLiteral("kill adds to global wallet and run total"));
        success &= require(ShopService.score().spend(50) && ShopService.score().total() == wallet + 70,
                           QStringLiteral("spend deducts from the wallet"));
        success &= require(!ShopService.score().spend(999999),
                           QStringLiteral("overdraft is rejected"));
        ShopService.score().beginRun();
        success &= require(ShopService.score().kills() == 0 && ShopService.score().run() == 0
                               && ShopService.score().total() == wallet + 70,
                           QStringLiteral("beginRun resets only the run stats"));
        ShopService.score().total_ = wallet;   // 还原，避免测试改动持久化余额

        log(QStringLiteral("Shop"));
        ShopService.score().total_ = 500;
        const int startMines = ShopService.inventory().mines();
        const QRectF shopButton = menu.main_.shopButtonRect();
        menu.mousePressLeft({float(shopButton.center().x()),
                             float(shopButton.center().y())});
        success &= require(menu.screen() == Menu::Screen::Shop,
                           QStringLiteral("shop button opens shop"));
        const QRectF buyButton = menu.depot_.buyRect();
        menu.mousePressLeft({float(buyButton.center().x()),
                             float(buyButton.center().y())});
        success &= require(ShopService.score().total() == 500 - data::kMine.price
                               && ShopService.inventory().mines() == startMines + 1,
                           QStringLiteral("buying a mine spends score and adds stock"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves shop"));

        log(QStringLiteral("Mine"));
        menu.startLevel(0);
        Games *mineGame = menu.match_.game();
        success &= require(mineGame != nullptr, QStringLiteral("mine test starts a run"));
        if (mineGame != nullptr) {
            for (int i = 0; i < 240 && mineGame->arena_.enemies().empty(); ++i) {
                mineGame->update(1.0f / 60.0f);
            }
            success &= require(!mineGame->arena_.enemies().empty(),
                               QStringLiteral("wave spawns enemies for the mine test"));
            if (!mineGame->arena_.enemies().empty()) {
                arena::Enemy *victim = mineGame->arena_.enemies().front().get();
                const Math::Vec2 spot = victim->pos;
                const int stockBefore = ShopService.inventory().mines();
                menu.mousePressRight(spot);
                success &= require(mineGame->arena_.mineCount() == 1
                                       && ShopService.inventory().mines() == stockBefore - 1,
                                   QStringLiteral("right click places a mine"));
                const int killsBefore = mineGame->kills();
                for (int i = 0; i < 60 && mineGame->arena_.mineCount() > 0; ++i) {
                    victim->pos = spot;
                    mineGame->update(1.0f / 60.0f);
                }
                success &= require(mineGame->arena_.mineCount() == 0
                                       && mineGame->kills() > killsBefore,
                                   QStringLiteral("enemy contact detonates the mine"));
            }
            menu.returnToMenu();
        }

        log(QStringLiteral("Bullet pool"));
        menu.startLevel(0);
        game = menu.match_.game();
        if (game != nullptr) {
            game->arena_.playerState().invuln = 999.0f;
            const int beforePool = game->arena_.pooledBulletCount();
            game->arena_.spawnStandard({100.0f, 100.0f}, {0.0f, 0.0f}, 1.0f,
                                       Base::Cyan, Bullet::Standpoint::Player, 3.0f);
            for (int i = 0; i < 160; ++i) {
                game->arena_.playerState().invuln = 999.0f;
                game->arena_.update(1.0f / 60.0f);
            }
            success &= require(game->arena_.pooledBulletCount() > beforePool,
                               QStringLiteral("dead standard bullets return to the pool"));
            menu.returnToMenu();
        }

        log(QStringLiteral("Submitted script"));
        games::Script shortScript;
        shortScript.push_back(games::Wave{{games::EnemySpawn{
                                               arena::EnemyType(typeid(arena::DroneEnemy)), 2},
                                           games::EnemySpawn{
                                               arena::EnemyType(typeid(arena::TankEnemy)), 1}}});
        shortScript.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::SpitterEnemy)), 3}}});
        menu.levels_.push_back({QStringLiteral("测试台本"),
                                QStringLiteral("运行时提交"), 1, shortScript});
        menu.startLevel(int(menu.levels_.size()) - 1);
        game = menu.match_.game();
        success &= require(game->totalWaves() == 2 && game->currentWave() == 1
                               && game->waves_.queue_.size() == 3,
                           QStringLiteral("submitted script drives the run"));
        menu.returnToMenu();

        log(QStringLiteral("Snapshot roundtrip and GameView"));
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game != nullptr, QStringLiteral("snapshot test starts a run"));
        if (game != nullptr) {
            const int savedWallet = ShopService.score().total();
            const int savedKills = ShopService.score().kills();
            ShopService.score().total_ = 2840;
            ShopService.score().kills_ = 31;
            game->state_ = Games::State::Upgrade;
            game->runTime_ = 42.5f;
            game->waves_.start(3);
            game->waves_.queue_ = {
                arena::EnemyType(typeid(arena::DroneEnemy)),
                arena::EnemyType(typeid(arena::SpitterEnemy))};
            game->waves_.waveTotal_ = 12;
            game->arena_.enemies().clear();
            game->arena_.portals().clear();
            game->arena_.playerBullets().clear();
            game->arena_.clearEnemy();
            game->arena_.particles().clear();
            game->arena_.floatingTexts().clear();
            game->arena_.shockwaves().clear();
            game->arena_.pickups().clear();
            game->arena_.mines().clear();

            const arena::Context snapshotCtx = game->arena_.context();
            game->arena_.playerState().pos = {475.0f, 430.0f};
            game->arena_.playerState().hp = 74.0f;
            game->arena_.playerState().hpDisplay = 84.0f;
            game->arena_.playerState().aim = -0.55f;
            game->arena_.playerState().multishot = 2;
            game->arena_.spawnEnemy(snapshotCtx, QStringLiteral("drone"), {820.0f, 285.0f}, 3);
            game->arena_.spawnEnemy(snapshotCtx, QStringLiteral("amplifier"),
                                    {860.0f, 340.0f}, 3);
            game->arena_.spawnEnemy(snapshotCtx, QStringLiteral("boss"),
                                    {870.0f, 365.0f}, 3);
            if (game->arena_.boss() != nullptr) {
                game->arena_.boss()->hp = game->arena_.boss()->maxHp * 0.63f;
                game->arena_.boss()->hpDisplay = game->arena_.boss()->maxHp * 0.74f;
            }
            game->arena_.enemies().front()->damageBuffTimer = 0.4f;
            game->arena_.enemies().front()->damageBuffBonus = 0.3f;
            game->arena_.portals().emplace_back(
                arena::EnemyType(typeid(arena::SpitterEnemy)), Math::Vec2(1160, 615), 3);
            game->arena_.spawnPickup(snapshotCtx, {360.0f, 315.0f}, 16);
            game->arena_.placeMine(Math::Vec2(520.0f, 430.0f));
            game->arena_.spawnBurst(snapshotCtx, {860.0f, 290.0f}, Base::Coral,
                                    14, 130.0f, 0.7f);
            game->arena_.addShockwave({860.0f, 290.0f}, 48.0f, Base::Coral, 0.6f, 2);
            game->arena_.floatingTexts().push_back(arena::FloatingText{
                {700.0f, 300.0f}, QStringLiteral("-17"), Base::Red,
                0.62f, 0.62f, 15, true});
            game->arena_.spawnStandard({650.0f, 360.0f}, {900.0f, -250.0f}, 12.5f,
                                       Base::Cyan, Bullet::Standpoint::Player, 3.2f);
            game->arena_.spawnStandard({850.0f, 300.0f}, {-420.0f, 150.0f}, 11.0f,
                                       Base::Magenta, Bullet::Standpoint::Enemy, 4.0f);
            game->upgrades_.open();

            const net::Snapshot original = game->snapshot();
            success &= require(original.state == net::RunState::Upgrade
                                   && original.runTime == game->runTime()
                                   && original.score == 2840 && original.kills == 31,
                               QStringLiteral("snapshot carries match state, time and score"));
            success &= require(original.players.size() == 1
                                   && original.players.front().playerId == 1
                                   && original.localPlayerId == 1
                                   && original.players.front().color == Base::Cyan,
                               QStringLiteral("snapshot carries player id and local player"));
            success &= require(original.enemies.size() == 3
                                   && original.enemies.front().dataKey
                                          == QStringLiteral("drone")
                                   && original.bullets.size() == 2
                                   && original.portals.size() == 1
                                   && original.pickups.size() == 1
                                   && original.mines.size() == 1
                                   && !original.particles.empty()
                                   && original.floatingTexts.size() == 1
                                   && original.shockwaves.size() == 1,
                               QStringLiteral("snapshot includes enemies, bullets and entities"));
            success &= require(original.waves.currentWave == 3
                                   && original.waves.totalWaves == 8
                                   && original.waves.remainingThreats > 0
                                   && original.upgrades.size() == 3
                                   && original.boss.active,
                               QStringLiteral("snapshot includes waves, upgrade offers and boss"));

            const QByteArray payload = net::serializeSnapshot(original);
            net::Snapshot decoded;
            QString snapshotError;
            const bool decodedOk =
                net::deserializeSnapshot(payload, &decoded, &snapshotError);
            success &= require(decodedOk,
                               QStringLiteral("snapshot deserializes (%1)").arg(snapshotError));
            success &= require(decodedOk && decoded == original,
                               QStringLiteral("snapshot roundtrip is field-exact"));
            net::Snapshot mutated = decoded;
            if (!mutated.enemies.empty()) {
                mutated.enemies.front().hp += 1.0f;
            }
            success &= require(!(mutated == decoded),
                               QStringLiteral("snapshot comparison notices field changes"));

            const QByteArray frame = net::encodeSnapshotMessage(original);
            net::Snapshot framed;
            snapshotError.clear();
            const bool framedOk =
                net::decodeSnapshotMessage(frame, &framed, &snapshotError);
            success &= require(framedOk && framed == original,
                               QStringLiteral("snapshot message roundtrips with framing"));
            net::MessageHeader frameHeader;
            QByteArray framePayload;
            success &= require(net::decodeFrame(frame, &frameHeader, &framePayload)
                                   && frameHeader.type == net::MessageType::Snapshot
                                   && frameHeader.length == quint32(framePayload.size())
                                   && quint32(frame.size())
                                          == net::kFrameHeaderSize + frameHeader.length,
                               QStringLiteral("snapshot is written as one exact frame"));
            const QByteArray compressed =
                net::encodeCompressedSnapshotMessage(original);
            net::Snapshot unzipped;
            snapshotError.clear();
            success &= require(!compressed.isEmpty()
                                   && net::decodeCompressedSnapshotMessage(
                                          compressed, &unzipped, &snapshotError)
                                   && unzipped == original,
                               QStringLiteral("compressed snapshot roundtrips"));
            const QByteArray wrongVersion = net::frameMessage(
                net::MessageType::Snapshot, net::serializeSnapshot(original),
                quint16(net::kProtocolVersion + 1));
            net::Snapshot rejected;
            snapshotError.clear();
            success &= require(!net::decodeSnapshotMessage(wrongVersion, &rejected,
                                                          &snapshotError)
                                   && !snapshotError.isEmpty(),
                               QStringLiteral("protocol version mismatch is rejected"));

            net::RoomListMessage roomList;
            net::RoomInfo room;
            room.name = QStringLiteral("测试房间");
            room.players = 2;
            room.maxPlayers = 4;
            room.playing = false;
            net::RoomPlayerInfo roomMemberA;
            roomMemberA.sessionId = 11;
            roomMemberA.playerId = 1;
            roomMemberA.host = true;
            net::RoomPlayerInfo roomMemberB;
            roomMemberB.sessionId = 12;
            roomMemberB.playerId = 2;
            room.members.push_back(roomMemberA);
            room.members.push_back(roomMemberB);
            roomList.rooms.push_back(room);
            net::RoomListMessage decodedRooms;
            success &= require(net::decodeRoomList(net::encodeRoomList(roomList),
                                                   &decodedRooms)
                                   && decodedRooms == roomList,
                               QStringLiteral("room list roundtrips through the protocol"));

            client::GameView view;
            const Random::EngineState rngBeforeView = RandomService.engineState();
            view.apply(decoded);
            success &= require(RandomService.engineState() == rngBeforeView,
                               QStringLiteral("view rebuild leaves the simulation RNG untouched"));
            success &= require(view.state() == original.state
                                   && view.runTime() == original.runTime
                                   && view.score() == original.score
                                   && view.kills() == original.kills,
                               QStringLiteral("view restores match state and score"));
            success &= require(view.playerState().hp == original.players.front().hp
                                   && view.playerState().pos == original.players.front().pos
                                   && view.localPlayerId() == original.localPlayerId
                                   && view.playerState().playerId == 1
                                   && view.world().aliveEnemyCount()
                                          == int(original.enemies.size()),
                               QStringLiteral("view restores player and enemies from snapshot"));
            success &= require(view.waves().currentWave == original.waves.currentWave
                                   && view.waves().remainingThreats
                                          == original.waves.remainingThreats
                                   && view.upgrades().size() == original.upgrades.size()
                                   && view.bossInfo().active == original.boss.active,
                               QStringLiteral("view restores HUD, offers and boss bar"));
            success &= require(view.world().portals().size() == original.portals.size()
                                   && view.world().pickups().size() == original.pickups.size()
                                   && view.world().mines().size() == original.mines.size()
                                   && view.world().particles().size()
                                          == original.particles.size()
                                   && view.world().floatingTexts().size()
                                          == original.floatingTexts.size()
                                   && view.world().shockwaves().size()
                                          == original.shockwaves.size(),
                               QStringLiteral("view restores portals, pickups, mines and effects"));

            games::Renderer viewRenderer;
            viewRenderer.initialize();
            viewRenderer.render(view);
            success &= require(viewRenderer.frame().size()
                                   == QSize(GameConfig::ScreenWidth,
                                            GameConfig::ScreenHeight),
                               QStringLiteral("snapshot-built GameView renders a frame"));

            game->render();
            success &= require(game->view().playerState().pos
                                   == game->arena_.playerState().pos
                                   && game->view().world().aliveEnemyCount()
                                          == game->arena_.aliveEnemyCount()
                                   && game->view().score() == 2840,
                               QStringLiteral("local render path goes Simulation -> Snapshot -> View"));
            ShopService.score().total_ = savedWallet;
            ShopService.score().kills_ = savedKills;
            menu.returnToMenu();
        }

        log(QStringLiteral("Multiplayer aircraft and input ownership"));
        {
            games::GameSimulation::Level mpLevel;
            mpLevel.name = QStringLiteral("多人模拟");
            mpLevel.startWave = 1;
            mpLevel.enemyLevel = 1;
            mpLevel.script.push_back(games::Wave{{games::EnemySpawn{
                arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});
            games::GameSimulation mp(mpLevel);
            mp.setPlayers({2, 3});
            mp.start();
            Arena &mpArena = mp.arena_;
            success &= require(mp.playerCount() == 2 && mpArena.playerCount() == 2
                                   && mp.hasPlayer(2) && mp.hasPlayer(3)
                                   && !mp.hasPlayer(1) && mp.leaderPlayerId() == 2,
                               QStringLiteral("two slots become two player aircraft"));
            success &= require(mpArena.playerStateAt(0).playerId == 2
                                   && mpArena.playerStateAt(1).playerId == 3
                                   && mpArena.playerStateAt(0).color
                                          != mpArena.playerStateAt(1).color
                                   && mpArena.playerStateAt(0).pos
                                          != mpArena.playerStateAt(1).pos,
                               QStringLiteral("player aircraft get distinct ids, colors and spawns"));

            const Math::Vec2 p2Before = mpArena.playerStateAt(0).pos;
            const Math::Vec2 p3Before = mpArena.playerStateAt(1).pos;
            arena::InputState moveRight;
            moveRight.moveAxis = {1.0f, 0.0f};
            moveRight.mouseActive = true;
            moveRight.mousePosition = {p2Before.x + 200.0f, p2Before.y};
            mp.setInput(2, moveRight);
            for (int i = 0; i < 40; ++i) {
                mp.update(1.0f / 60.0f);
            }
            success &= require(mpArena.playerStateAt(0).pos.x > p2Before.x + 5.0f
                                   && mpArena.playerStateAt(1).pos.distanceTo(p3Before)
                                          < 0.5f,
                               QStringLiteral("input moves only the sender's aircraft"));

            // 让第一架停稳，再用一个不存在的 playerId 发输入：必须完全忽略。
            arena::InputState stop;
            stop.mouseActive = true;
            stop.mousePosition = {mpArena.playerStateAt(0).pos.x + 1.0f,
                                  mpArena.playerStateAt(0).pos.y};
            mp.setInput(2, stop);
            for (int i = 0; i < 60; ++i) {
                mp.update(1.0f / 60.0f);
            }
            const Math::Vec2 p2Idle = mpArena.playerStateAt(0).pos;
            const Math::Vec2 p3Idle = mpArena.playerStateAt(1).pos;
            arena::InputState bogus = moveRight;
            bogus.mousePosition = {p2Idle.x + 300.0f, p2Idle.y};
            mp.setInput(99, bogus);   // 未知/观战 id：没有飞机
            for (int i = 0; i < 40; ++i) {
                mp.update(1.0f / 60.0f);
            }
            success &= require(mpArena.playerStateAt(0).pos.distanceTo(p2Idle) < 1.0f
                                   && mpArena.playerStateAt(1).pos.distanceTo(p3Idle) < 1.0f,
                               QStringLiteral("unknown/spectator player id is ignored"));

            const net::Snapshot twoPlayer = mp.snapshot();
            success &= require(twoPlayer.players.size() == 2
                                   && twoPlayer.players[0].playerId == 2
                                   && twoPlayer.players[1].playerId == 3
                                   && twoPlayer.localPlayerId == 2
                                   && twoPlayer.players[0].color
                                          != twoPlayer.players[1].color,
                               QStringLiteral("snapshot carries both aircraft and localPlayerId"));
            net::Snapshot decodedTwo;
            const QByteArray twoPayload = net::serializeSnapshot(twoPlayer);
            success &= require(net::deserializeSnapshot(twoPayload, &decodedTwo)
                                   && decodedTwo == twoPlayer,
                               QStringLiteral("multi-player snapshot roundtrips with ids/local player"));

            client::GameView localView;
            localView.apply(twoPlayer);
            success &= require(localView.playerCount() == 2
                                   && localView.localPlayerId() == 2
                                   && localView.playerState().playerId == 2
                                   && localView.playerIdAt(1) == 3,
                               QStringLiteral("view selects the local aircraft from the snapshot"));
            // 同一个世界换一个 localPlayerId：客户端 3 的 HUD 必须切到 3 号机。
            net::Snapshot otherViewSnapshot = twoPlayer;
            otherViewSnapshot.localPlayerId = 3;
            net::Snapshot decodedOther;
            success &= require(net::deserializeSnapshot(
                                   net::serializeSnapshot(otherViewSnapshot), &decodedOther),
                               QStringLiteral("per-client localPlayerId survives serialization"));
            localView.apply(decodedOther);
            success &= require(localView.localPlayerId() == 3
                                   && localView.playerState().playerId == 3
                                   && localView.world().playerState().playerId == 3,
                               QStringLiteral("each client's view/HUD selects its own aircraft"));

            // 会话离开：只移除那一架，剩余玩家继续，不崩溃。
            mpArena.playerStateAt(0).invuln = 999.0f;
            success &= require(mp.removePlayer(2) && mp.playerCount() == 1
                                   && mp.hasPlayer(3) && !mp.hasPlayer(2),
                               QStringLiteral("leaving player removes only its aircraft"));
            for (int i = 0; i < 60; ++i) {
                mp.update(1.0f / 60.0f);
            }
            const net::Snapshot afterLeave = mp.snapshot();
            success &= require(mp.state() != games::GameSimulation::State::Defeat
                                   && afterLeave.players.size() == 1
                                   && afterLeave.players.front().playerId == 3
                                   && afterLeave.localPlayerId == 3,
                               QStringLiteral("match continues after a player disconnects"));

            // 多玩家阵亡 → 重新部署；全灭 → 失败。
            games::GameSimulation respawnSim(mpLevel);
            respawnSim.setPlayers({1, 2});
            respawnSim.start();
            Arena &respawnArena = respawnSim.arena_;
            arena::Context respawnCtx = respawnArena.context();
            respawnArena.playerStateAt(1).invuln = 999.0f;
            respawnArena.playerStateAt(0).invuln = 0.0f;
            respawnArena.playerById(1).takeDamage(
                respawnCtx, arena::Damage{9999.0f, false, arena::DamageSource::Bullet});
            success &= require(!respawnArena.playerById(1).state.alive
                                   && respawnArena.playerById(1).state.respawnTimer > 0.0f
                                   && respawnSim.state() == games::GameSimulation::State::Playing,
                               QStringLiteral("a downed aircraft waits to respawn while allies live"));
            for (int i = 0; i < 220; ++i) {
                respawnArena.playerStateAt(1).invuln = 999.0f;
                respawnSim.update(1.0f / 60.0f);
            }
            success &= require(respawnArena.playerById(1).state.alive
                                   && respawnArena.playerById(1).state.respawnTimer <= 0.0f,
                               QStringLiteral("downed aircraft respawns after the timer"));
            respawnArena.playerStateAt(0).invuln = 0.0f;
            respawnArena.playerStateAt(1).invuln = 0.0f;
            respawnArena.playerStateAt(0).hp = 1.0f;
            respawnArena.playerStateAt(1).hp = 1.0f;
            respawnArena.playerById(1).takeDamage(
                respawnCtx, arena::Damage{50.0f, false, arena::DamageSource::Bullet});
            respawnArena.playerById(2).takeDamage(
                respawnCtx, arena::Damage{50.0f, false, arena::DamageSource::Bullet});
            respawnSim.update(1.0f / 60.0f);
            success &= require(respawnSim.state() == games::GameSimulation::State::Defeat
                                   && respawnArena.playerDefeated() && respawnSim.playerCount() == 2,
                               QStringLiteral("all players down triggers defeat without removing them"));
        }

        log(QStringLiteral("TCP loopback"));
        // 真实 QTcpServer/QTcpSocket 回环：握手、StartMatch、输入、快照广播。
        auto pumpUntil = [](const std::function<bool()> &predicate, int timeoutMs) {
            QElapsedTimer timer;
            timer.start();
            while (!predicate() && timer.elapsed() < timeoutMs) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                QThread::msleep(1);
            }
            return predicate();
        };

        server::GameServer tcpServer;
        QString tcpError;
        const bool tcpListening = tcpServer.listen(0, true, &tcpError);
        success &= require(tcpListening && tcpServer.port() != 0,
                           QStringLiteral("TCP server listens on an automatic loopback port (%1)")
                               .arg(tcpError));
        if (tcpListening) {
            Games::Level tcpLevel;
            tcpLevel.name = QStringLiteral("TCP 回环测试");
            tcpLevel.startWave = 1;
            tcpLevel.enemyLevel = 1;
            tcpLevel.script.push_back(games::Wave{{games::EnemySpawn{
                arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});

            client::GameClient tcpClient;
            success &= require(tcpClient.connectToHost(QStringLiteral("127.0.0.1"),
                                                       tcpServer.port()),
                               QStringLiteral("TCP client starts connecting"));
            success &= require(pumpUntil([&] { return tcpClient.welcomed(); }, 3000),
                               QStringLiteral("TCP client completes Hello/Welcome handshake"));
            success &= require(pumpUntil([&] {
                                   return tcpClient.inLobby()
                                          && !tcpClient.rooms().empty();
                               }, 3000),
                               QStringLiteral("server presents a room before any match"));
            success &= require(tcpServer.claimAdmin(tcpClient.sessionId()),
                               QStringLiteral("host claims admin with its session id"));
            success &= require(tcpServer.adminSessionId() == tcpClient.sessionId(),
                               QStringLiteral("server records the admin session"));
            success &= require(tcpServer.startMatch(tcpLevel),
                               QStringLiteral("host starts the authoritative match locally"));
            success &= require(pumpUntil([&] { return tcpClient.snapshotCount() > 0; }, 3000),
                               QStringLiteral("TCP client receives snapshots"));
            success &= require(!tcpClient.inLobby(),
                               QStringLiteral("client leaves the room when the match starts"));
            success &= require(tcpServer.clientCount() == 1
                                   && tcpServer.snapshot().waves.totalWaves == 1,
                               QStringLiteral("server owns the authoritative match"));
            success &= require(tcpClient.lastSnapshot().waves.totalWaves == 1
                                   && tcpClient.view().playerState().pos
                                          == tcpClient.lastSnapshot().players.front().pos
                                   && tcpClient.lastSnapshot().localPlayerId
                                          == tcpClient.playerId()
                                   && tcpClient.view().waves().currentWave
                                          == tcpClient.lastSnapshot().waves.currentWave,
                               QStringLiteral("TCP snapshot rebuilds the client GameView"));

            const float beforeX = tcpClient.view().playerState().pos.x;
            net::InputCommand move;
            move.moveX = 1.0f;
            move.mouseActive = true;
            move.mouseX = beforeX + 220.0f;
            move.mouseY = tcpClient.view().playerState().pos.y;
            success &= require(tcpClient.sendInput(move),
                               QStringLiteral("input command is written to the TCP socket"));
            success &= require(pumpUntil([&] {
                                   return tcpClient.view().playerState().pos.x > beforeX + 5.0f;
                               }, 3000),
                               QStringLiteral("server applies the input command and broadcasts the result"));
            success &= require(tcpServer.snapshot().players.front().pos.x > beforeX + 5.0f,
                               QStringLiteral("authoritative simulation moved the player"));

            client::GameClient watcher;
            success &= require(watcher.connectToHost(QStringLiteral("127.0.0.1"),
                                                     tcpServer.port()),
                               QStringLiteral("a second client starts connecting"));
            success &= require(pumpUntil([&] {
                                   return watcher.welcomed() && watcher.snapshotCount() > 0;
                               }, 3000),
                               QStringLiteral("second client receives broadcast snapshots"));
            success &= require(tcpServer.clientCount() == 2,
                               QStringLiteral("server accepts two clients at once"));

            // 控制面：本地 stdio 帧可以开局/输入；TCP 客户端不能开局。
            server::GameServer stdioServer;
            QString stdioError;
            success &= require(stdioServer.listen(0, true, &stdioError),
                               QStringLiteral("stdio control test server listens (%1)")
                                   .arg(stdioError));
            net::StartMatchMessage stdioStart;
            stdioStart.levelName = QStringLiteral("stdio");
            stdioStart.startWave = 1;
            stdioStart.enemyLevel = 1;
            net::StartMatchWave stdioWave;
            stdioWave.spawns.push_back({QStringLiteral("drone"), 1});
            stdioStart.waves.push_back(stdioWave);
            success &= require(stdioServer.handleControlFrame(
                                   net::encodeStartMatch(stdioStart), &stdioError),
                               QStringLiteral("stdio StartMatch starts the authoritative match"));
            net::InputCommand stdioInput;
            stdioInput.moveX = 1.0f;
            success &= require(stdioServer.handleControlFrame(
                                   net::encodeInputMessage(stdioInput), &stdioError),
                               QStringLiteral("stdio Input applies to the simulation"));
            success &= require(pumpUntil([&] {
                                   return stdioServer.snapshot().players.front().pos.x > 1.0f;
                               }, 3000),
                               QStringLiteral("stdio input moves the player"));
            stdioServer.close();

            server::GameServer conflict;
            QString conflictError;
            success &= require(!conflict.listen(tcpServer.port(), true, &conflictError)
                                   && !conflictError.isEmpty(),
                               QStringLiteral("port in use is reported instead of crashing"));

            QTcpSocket rawClient;
            rawClient.connectToHost(QStringLiteral("127.0.0.1"), tcpServer.port());
            pumpUntil([&] {
                return rawClient.state() == QAbstractSocket::ConnectedState;
            }, 2000);
            const QByteArray wrongHello = net::frameMessage(
                net::MessageType::Hello, QByteArray(),
                quint16(net::kProtocolVersion + 1));
            rawClient.write(wrongHello);
            rawClient.flush();
            success &= require(pumpUntil([&] {
                                   return rawClient.state()
                                          == QAbstractSocket::UnconnectedState;
                               }, 3000),
                               QStringLiteral("server closes a client with a mismatched protocol"));
            success &= require(tcpServer.rejectedClients() >= 1,
                               QStringLiteral("protocol mismatch is rejected without crashing"));

            // 控制面安全：TCP 客户端不能 StartMatch，即使已经 Hello。
            QTcpSocket startProbe;
            startProbe.connectToHost(QStringLiteral("127.0.0.1"), tcpServer.port());
            pumpUntil([&] {
                return startProbe.state() == QAbstractSocket::ConnectedState;
            }, 2000);
            net::HelloMessage probeHello;
            probeHello.clientVersion = QStringLiteral("probe");
            startProbe.write(net::encodeHello(probeHello));
            startProbe.flush();
            pumpUntil([&] { return startProbe.bytesAvailable() > 0; }, 2000);
            net::StartMatchMessage intruderStart;
            intruderStart.levelName = QStringLiteral("intruder");
            intruderStart.startWave = 1;
            intruderStart.enemyLevel = 1;
            net::StartMatchWave intruderWave;
            intruderWave.spawns.push_back({QStringLiteral("drone"), 99});
            intruderStart.waves.push_back(intruderWave);
            startProbe.write(net::encodeStartMatch(intruderStart));
            startProbe.flush();
            success &= require(pumpUntil([&] {
                                   return startProbe.state()
                                          == QAbstractSocket::UnconnectedState;
                               }, 3000),
                               QStringLiteral("server rejects TCP StartMatch from a client"));
            success &= require(tcpServer.running()
                                   && tcpServer.snapshot().waves.totalWaves == 1,
                               QStringLiteral("TCP StartMatch cannot replace the host match"));

            // 已删除的 JoinRoom（id 9）与任意未知类型：Hello 之后必须被拒绝。
            const int rejectedBefore = tcpServer.rejectedClients();
            QTcpSocket unknownProbe;
            unknownProbe.connectToHost(QStringLiteral("127.0.0.1"), tcpServer.port());
            pumpUntil([&] {
                return unknownProbe.state() == QAbstractSocket::ConnectedState;
            }, 2000);
            net::HelloMessage unknownHello;
            unknownHello.clientVersion = QStringLiteral("unknown-probe");
            unknownProbe.write(net::encodeHello(unknownHello));
            unknownProbe.flush();
            pumpUntil([&] { return unknownProbe.bytesAvailable() > 0; }, 2000);
            unknownProbe.write(net::frameMessage(
                static_cast<net::MessageType>(9), QByteArray()));
            unknownProbe.flush();
            success &= require(pumpUntil([&] {
                                   return unknownProbe.state()
                                          == QAbstractSocket::UnconnectedState;
                               }, 3000),
                               QStringLiteral("server rejects the removed JoinRoom id"));
            success &= require(tcpServer.rejectedClients() == rejectedBefore + 1,
                               QStringLiteral("unknown message types are counted as rejects"));

            tcpClient.close();
            watcher.close();
            success &= require(pumpUntil([&] { return tcpServer.clientCount() == 0; }, 3000),
                               QStringLiteral("server drops sessions when clients disconnect"));

            const quint16 closedPort = tcpServer.port();
            tcpServer.close();
            pumpUntil([&] { return !tcpServer.isListening(); }, 1000);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            client::GameClient deadClient;
            deadClient.setAutoReconnect(false);
            success &= require(deadClient.connectToHost(QStringLiteral("127.0.0.1"),
                                                        closedPort),
                               QStringLiteral("client can attempt a closed-port connection"));
            success &= require(pumpUntil([&] { return deadClient.failed(); }, 8000),
                               QStringLiteral("connection failure is reported without crashing"));
            success &= require(!deadClient.lastError().isEmpty(),
                               QStringLiteral("connection failure carries an error message"));
        }

        log(QStringLiteral("Four-client room fan-out"));
        {
            server::GameServer quadServer;
            QString quadError;
            const bool quadListening = quadServer.listen(0, true, &quadError);
            success &= require(quadListening,
                               QStringLiteral("four-client server listens (%1)")
                                   .arg(quadError));
            if (quadListening) {
                const Games::Level quadLevel =
                    networkTestLevel(QStringLiteral("四人房间"));
                std::array<std::unique_ptr<client::GameClient>, 4> quads;
                bool quadConnectStarted = true;
                for (int i = 0; i < 4; ++i) {
                    quads[size_t(i)] = std::make_unique<client::GameClient>();
                    quadConnectStarted &=
                        quads[size_t(i)]->connectToHost(QStringLiteral("127.0.0.1"),
                                                        quadServer.port());
                }
                success &= require(quadConnectStarted,
                                   QStringLiteral("four clients start connecting before the match"));

                auto allInRoom = [&] {
                    for (const auto &quad : quads) {
                        if (!quad->welcomed() || !quad->inLobby()) {
                            return false;
                        }
                    }
                    return quadServer.roomPlayers() == 4;
                };
                success &= require(pumpNetworkUntil(allInRoom, 6000),
                                   QStringLiteral("all four clients complete Hello/Welcome"));
                success &= require(quadServer.roomPlayers() == 4
                                       && quadServer.roomMaxPlayers() == 4
                                       && quadServer.roomHostSessionId() == 0,
                                   QStringLiteral("room count/capacity/host come from the server"));
                std::array<int, 4> quadPlayerIds{};
                std::array<quint32, 4> quadSessionIds{};
                for (int i = 0; i < 4; ++i) {
                    quadPlayerIds[size_t(i)] = quads[size_t(i)]->playerId();
                    quadSessionIds[size_t(i)] = quads[size_t(i)]->sessionId();
                }
                std::array<int, 4> sortedPlayerIds = quadPlayerIds;
                std::sort(sortedPlayerIds.begin(), sortedPlayerIds.end());
                bool distinctSessions = true;
                for (int i = 0; i < 4; ++i) {
                    for (int j = i + 1; j < 4; ++j) {
                        distinctSessions &= quadSessionIds[size_t(i)]
                                            != quadSessionIds[size_t(j)];
                    }
                }
                success &= require(sortedPlayerIds
                                       == std::array<int, 4>{1, 2, 3, 4}
                                       && distinctSessions,
                                   QStringLiteral("four sessions get stable, distinct player ids"));

                // 未 claim admin 前，服务端拒绝开局；claim 之后才能开始。
                QString noAdminError;
                success &= require(!quadServer.startMatch(quadLevel, &noAdminError)
                                       && !noAdminError.isEmpty()
                                       && !quadServer.running(),
                                   QStringLiteral("match start is refused before admin is claimed"));
                success &= require(quadServer.claimAdmin(quadSessionIds[0]),
                                   QStringLiteral("host claims admin with the first session"));
                success &= require(quadServer.adminSessionId() == quadSessionIds[0]
                                       && quadServer.roomHostSessionId() == quadSessionIds[0],
                                   QStringLiteral("admin identity and room host agree"));
                success &= require(quadServer.startMatch(quadLevel),
                                   QStringLiteral("claimed admin starts the four-client match"));
                auto allHaveSnapshots = [&] {
                    for (const auto &quad : quads) {
                        if (quad->snapshotCount() == 0) {
                            return false;
                        }
                    }
                    return true;
                };
                success &= require(pumpNetworkUntil(allHaveSnapshots, 4000),
                                   QStringLiteral("all four clients receive snapshots"));
                bool fanoutOk = true;
                for (const auto &quad : quads) {
                    const net::Snapshot &snap = quad->lastSnapshot();
                    fanoutOk &= snap.players.size() == 4
                                && snap.localPlayerId == quad->playerId()
                                && quad->view().playerState().playerId
                                       == quad->playerId()
                                && !quad->inLobby();
                }
                success &= require(fanoutOk,
                                   QStringLiteral("each client sees four aircraft and its own local id"));
                const net::Snapshot &quadFirst = quads[0]->lastSnapshot();
                bool uniqueAircraft = quadFirst.players.size() == 4;
                for (size_t i = 0; i < quadFirst.players.size(); ++i) {
                    for (size_t j = i + 1; j < quadFirst.players.size(); ++j) {
                        uniqueAircraft &= quadFirst.players[i].playerId
                                              != quadFirst.players[j].playerId
                                          && quadFirst.players[i].color
                                              != quadFirst.players[j].color
                                          && quadFirst.players[i].pos
                                              != quadFirst.players[j].pos;
                    }
                }
                success &= require(uniqueAircraft,
                                   QStringLiteral("four aircraft keep distinct ids, colors and spawns"));

                auto playerPos = [](const net::Snapshot &snapshot, int playerId) {
                    for (const net::PlayerSnapshot &player : snapshot.players) {
                        if (player.playerId == playerId) {
                            return player.pos;
                        }
                    }
                    return Math::Vec2{};
                };
                const Math::Vec2 moverBefore =
                    playerPos(quads[1]->lastSnapshot(), quads[1]->playerId());
                const Math::Vec2 bystanderBefore =
                    playerPos(quads[2]->lastSnapshot(), quads[2]->playerId());
                net::InputCommand quadMove;
                quadMove.moveX = 1.0f;
                quadMove.mouseActive = true;
                quadMove.mouseX = moverBefore.x + 220.0f;
                quadMove.mouseY = moverBefore.y;
                success &= require(quads[1]->sendInput(quadMove),
                                   QStringLiteral("one of four clients sends input"));
                success &= require(pumpNetworkUntil([&] {
                                       return playerPos(quads[1]->lastSnapshot(),
                                                        quads[1]->playerId()).x
                                              > moverBefore.x + 4.0f;
                                   }, 3000),
                                   QStringLiteral("four-client input moves only the sender's aircraft"));
                success &= require(playerPos(quads[2]->lastSnapshot(),
                                             quads[2]->playerId())
                                       .distanceTo(bystanderBefore) < 1.5f,
                                   QStringLiteral("other clients' aircraft stay put"));

                // 房间满员时第 5 个连接被拒绝，不能挤掉已有成员。
                client::GameClient overflow;
                const int rejectsBefore = quadServer.rejectedClients();
                overflow.connectToHost(QStringLiteral("127.0.0.1"), quadServer.port());
                success &= require(pumpNetworkUntil([&] { return overflow.failed(); },
                                                    3000),
                                   QStringLiteral("a fifth client is rejected while the room is full"));
                success &= require(quadServer.rejectedClients() == rejectsBefore + 1
                                       && quadServer.roomPlayers() == 4,
                                   QStringLiteral("full-room rejection keeps the roster intact"));

                // 有槽位释放后，对局中再加入的会话只能观战。
                quads[3]->close();
                success &= require(pumpNetworkUntil([&] {
                                       return quadServer.roomPlayers() == 3;
                                   }, 3000),
                                   QStringLiteral("a mid-match disconnect frees its room slot"));
                client::GameClient lateSpectator;
                success &= require(lateSpectator.connectToHost(QStringLiteral("127.0.0.1"),
                                                               quadServer.port()),
                                   QStringLiteral("a late client connects after the match started"));
                success &= require(pumpNetworkUntil([&] {
                                       return lateSpectator.welcomed()
                                              && lateSpectator.snapshotCount() > 0;
                                   }, 4000),
                                   QStringLiteral("late client receives running snapshots"));
                success &= require(lateSpectator.spectating()
                                       && lateSpectator.localPlayerId() == 0
                                       && lateSpectator.lastSnapshot().players.size() == 3
                                       && quadServer.roomPlayers() == 4,
                                   QStringLiteral("late joiner is a spectator without an aircraft"));
                quads[0]->close();
                quads[1]->close();
                quads[2]->close();
                overflow.close();
                lateSpectator.close();
                success &= require(pumpNetworkUntil([&] {
                                       return quadServer.clientCount() == 0;
                                   }, 3000),
                                   QStringLiteral("four-client server tears down cleanly"));
                quadServer.close();
            }
        }

        log(QStringLiteral("Join/start/dissolve races"));
        {
            // 1) 第一个客户端还在连接时就开始对局：服务端用 fallback roster 开局，
            //    客户端完成握手后必须接管那架飞机，而不是变成观战者。
            server::GameServer earlyServer;
            QString earlyError;
            const bool earlyListening = earlyServer.listen(0, true, &earlyError);
            success &= require(earlyListening,
                               QStringLiteral("race server listens (%1)").arg(earlyError));
            LatencyProxy earlyProxy;
            QString earlyProxyError;
            const Games::Level raceLevel = networkTestLevel(QStringLiteral("竞态"));
            if (earlyListening
                && earlyProxy.listen(earlyServer.port(), 120, &earlyProxyError)) {
                client::GameClient earlyClient;
                success &= require(earlyClient.connectToHost(QStringLiteral("127.0.0.1"),
                                                             earlyProxy.port()),
                                   QStringLiteral("client starts connecting through a delay proxy"));
                success &= require(earlyServer.startMatch(raceLevel),
                                   QStringLiteral("host starts while the first client is still joining"));
                success &= require(earlyServer.running(),
                                   QStringLiteral("match runs before any Welcome was delivered"));
                success &= require(pumpNetworkUntil([&] {
                                       return earlyClient.welcomed()
                                              && earlyClient.snapshotCount() > 0;
                                   }, 6000),
                                   QStringLiteral("mid-join client completes the handshake into a running match"));
                success &= require(earlyClient.playerId() > 0
                                       && earlyClient.localPlayerId()
                                              == earlyClient.playerId()
                                       && !earlyClient.spectating(),
                                   QStringLiteral("mid-join client takes over the fallback slot"));
                success &= require(earlyClient.lastSnapshot().players.size() == 1,
                                   QStringLiteral("running match keeps exactly one fallback aircraft"));
                earlyClient.close();
                earlyProxy.close();
                earlyServer.close();
            } else {
                success &= require(false,
                                   QStringLiteral("race latency proxy listens (%1)")
                                       .arg(earlyProxyError));
            }

            // 2) 一个客户端已经在房间、第二个正握手时开始对局：第二个会话本局只能观战。
            server::GameServer midServer;
            QString midError;
            const bool midListening = midServer.listen(0, true, &midError);
            success &= require(midListening,
                               QStringLiteral("second race server listens (%1)").arg(midError));
            LatencyProxy midProxy;
            QString midProxyError;
            if (midListening && midProxy.listen(midServer.port(), 120, &midProxyError)) {
                client::GameClient seated;
                success &= require(seated.connectToHost(QStringLiteral("127.0.0.1"),
                                                        midServer.port()),
                                   QStringLiteral("seated client connects directly"));
                success &= require(pumpNetworkUntil([&] {
                                       return seated.welcomed() && seated.inLobby();
                                   }, 4000),
                                   QStringLiteral("seated client enters the room"));
                success &= require(midServer.claimAdmin(seated.sessionId()),
                                   QStringLiteral("seated client claims admin"));
                client::GameClient midJoin;
                success &= require(midJoin.connectToHost(QStringLiteral("127.0.0.1"),
                                                         midProxy.port()),
                                   QStringLiteral("second client starts a delayed handshake"));
                success &= require(pumpNetworkUntil([&] {
                                       return midServer.clientCount() == 2
                                              && midProxy.accepted() == 1
                                              && !midJoin.welcomed();
                                   }, 4000),
                                   QStringLiteral("second client is in the TCP session but not yet welcomed"));
                success &= require(midServer.startMatch(raceLevel),
                                   QStringLiteral("admin starts while the second client is mid-handshake"));
                success &= require(pumpNetworkUntil([&] {
                                       return seated.snapshotCount() > 0
                                              && midJoin.welcomed()
                                              && midJoin.snapshotCount() > 0;
                                   }, 5000),
                                   QStringLiteral("both clients reach snapshots after the race"));
                success &= require(midJoin.spectating()
                                       && midJoin.localPlayerId() == 0
                                       && midJoin.playerId() != seated.playerId()
                                       && midJoin.lastSnapshot().players.size() == 1,
                                   QStringLiteral("mid-handshake joiner becomes a spectator, not a crash"));

                // 3) 对局进行中解散：已入座和刚完成握手的客户端都必须进入失败态，不能挂起。
                midServer.dissolveRoom();
                success &= require(pumpNetworkUntil([&] {
                                       return seated.failed() && midJoin.failed();
                                   }, 5000),
                                   QStringLiteral("dissolve fails both connected clients instead of hanging"));
                success &= require(seated.lastError().contains(QStringLiteral("解散"))
                                       || seated.lastError().contains(QStringLiteral("closed")),
                                   QStringLiteral("seated client reports the dissolved room"));
                success &= require(pumpNetworkUntil([&] {
                                       return midServer.clientCount() == 0;
                                   }, 3000),
                                   QStringLiteral("dissolve drops every session"));
                success &= require(!midServer.isListening(),
                                   QStringLiteral("dissolve closes the listener"));
                success &= require(pumpNetworkUntil([&] {
                                       return !seated.welcomed() && !midJoin.welcomed();
                                   }, 3000),
                                   QStringLiteral("failed clients are no longer welcomed"));
                midProxy.close();
            } else {
                success &= require(false,
                                   QStringLiteral("second race proxy listens (%1)")
                                       .arg(midProxyError));
            }

            // 4) 房间已入座、第二个客户端还在握手时直接解散：两个客户端都不崩、不挂起。
            server::GameServer joinDissolveServer;
            QString joinDissolveError;
            const bool joinDissolveListening =
                joinDissolveServer.listen(0, true, &joinDissolveError);
            success &= require(joinDissolveListening,
                               QStringLiteral("join/dissolve race server listens (%1)")
                                   .arg(joinDissolveError));
            LatencyProxy joinDissolveProxy;
            QString joinDissolveProxyError;
            if (joinDissolveListening
                && joinDissolveProxy.listen(joinDissolveServer.port(), 120,
                                            &joinDissolveProxyError)) {
                client::GameClient roomClient;
                roomClient.connectToHost(QStringLiteral("127.0.0.1"),
                                         joinDissolveServer.port());
                success &= require(pumpNetworkUntil([&] {
                                       return roomClient.welcomed() && roomClient.inLobby();
                                   }, 4000),
                                   QStringLiteral("room client enters before the dissolve"));
                client::GameClient joiningClient;
                joiningClient.connectToHost(QStringLiteral("127.0.0.1"),
                                            joinDissolveProxy.port());
                success &= require(pumpNetworkUntil([&] {
                                       return joinDissolveServer.clientCount() == 2
                                              && joinDissolveProxy.accepted() == 1
                                              && !joiningClient.welcomed();
                                   }, 4000),
                                   QStringLiteral("dissolve happens with a second client mid-handshake"));
                joinDissolveServer.dissolveRoom();
                success &= require(pumpNetworkUntil([&] {
                                       return roomClient.failed() && joiningClient.failed();
                                   }, 5000),
                                   QStringLiteral("dissolve during join fails both clients without a crash"));
                success &= require(!joiningClient.lastError().isEmpty()
                                       && !roomClient.lastError().isEmpty(),
                                   QStringLiteral("both race clients carry a failure reason"));
                joinDissolveProxy.close();
            } else {
                success &= require(false,
                                   QStringLiteral("join/dissolve proxy listens (%1)")
                                       .arg(joinDissolveProxyError));
            }
            joinDissolveServer.close();
        }

        log(QStringLiteral("Admin and session lifecycle"));
        {
            server::GameServer adminServer;
            QString adminError;
            const bool adminListening = adminServer.listen(0, true, &adminError);
            success &= require(adminListening,
                               QStringLiteral("admin lifecycle server listens (%1)")
                                   .arg(adminError));
            if (adminListening) {
                const Games::Level adminLevel =
                    networkTestLevel(QStringLiteral("管理员"));
                client::GameClient hostClient;
                client::GameClient guestClient;
                success &= require(hostClient.connectToHost(QStringLiteral("127.0.0.1"),
                                                            adminServer.port())
                                       && guestClient.connectToHost(
                                           QStringLiteral("127.0.0.1"),
                                           adminServer.port()),
                                   QStringLiteral("host and guest sessions connect"));
                success &= require(pumpNetworkUntil([&] {
                                       return hostClient.welcomed()
                                              && guestClient.welcomed()
                                              && adminServer.roomPlayers() == 2;
                                   }, 4000),
                                   QStringLiteral("both sessions are welcomed into the room"));
                success &= require(hostClient.playerId() != guestClient.playerId(),
                                   QStringLiteral("host and guest get distinct player slots"));

                // 未 claim 前任何来源都不能开局。
                QString unclaimedError;
                success &= require(!adminServer.startMatch(adminLevel, &unclaimedError)
                                       && !unclaimedError.isEmpty()
                                       && !adminServer.running(),
                                   QStringLiteral("start is rejected without a claimed admin"));
                QString unclaimedReturnError;
                success &= require(!adminServer.returnToRoom(&unclaimedReturnError)
                                       && !unclaimedReturnError.isEmpty(),
                                   QStringLiteral("return-to-room is rejected without a claimed admin"));

                // TCP 客户端（非宿主控制面）发 StartMatch：直接拒绝且不改变服务端状态。
                const int rejectsBefore = adminServer.rejectedClients();
                QTcpSocket intruder;
                intruder.connectToHost(QStringLiteral("127.0.0.1"), adminServer.port());
                success &= require(pumpNetworkUntil([&] {
                                       return intruder.state()
                                              == QAbstractSocket::ConnectedState;
                                   }, 2000),
                                   QStringLiteral("raw TCP intruder connects"));
                net::HelloMessage intruderHello;
                intruderHello.clientVersion = QStringLiteral("non-admin");
                intruder.write(net::encodeHello(intruderHello));
                net::StartMatchMessage intruderStart;
                intruderStart.levelName = QStringLiteral("intruder");
                intruderStart.startWave = 1;
                intruderStart.enemyLevel = 1;
                net::StartMatchWave intruderWave;
                intruderWave.spawns.push_back({QStringLiteral("drone"), 99});
                intruderStart.waves.push_back(intruderWave);
                intruder.write(net::encodeStartMatch(intruderStart));
                intruder.flush();
                success &= require(pumpNetworkUntil([&] {
                                       return intruder.state()
                                              == QAbstractSocket::UnconnectedState;
                                   }, 3000),
                                   QStringLiteral("non-admin TCP StartMatch is rejected"));
                success &= require(pumpNetworkUntil([&] {
                                       return adminServer.roomPlayers() == 2;
                                   }, 3000),
                                   QStringLiteral("rejected intruder leaves the room"));
                success &= require(adminServer.rejectedClients() == rejectsBefore + 1
                                       && !adminServer.running(),
                                   QStringLiteral("rejected StartMatch starts nothing"));

                QString bogusClaimError;
                success &= require(!adminServer.claimAdmin(999999, &bogusClaimError)
                                       && !bogusClaimError.isEmpty(),
                                   QStringLiteral("claiming an unknown session fails"));
                success &= require(adminServer.claimAdmin(hostClient.sessionId()),
                                   QStringLiteral("host claims admin with its own session"));
                const quint32 adminId = hostClient.sessionId();
                success &= require(adminServer.adminSessionId() == adminId
                                       && adminServer.roomHostSessionId() == adminId,
                                   QStringLiteral("claimed admin owns the room host slot"));
                success &= require(adminServer.claimAdmin(hostClient.sessionId()),
                                   QStringLiteral("re-claiming the same session is idempotent"));
                success &= require(adminServer.adminSessionId() == adminId,
                                   QStringLiteral("admin identity survives re-claim"));
                success &= require(adminServer.startMatch(adminLevel),
                                   QStringLiteral("claimed admin starts the match"));
                success &= require(pumpNetworkUntil([&] {
                                       return hostClient.snapshotCount() > 0
                                              && guestClient.snapshotCount() > 0;
                                   }, 4000),
                                   QStringLiteral("both sessions receive the admin's match"));
                success &= require(adminServer.adminSessionId() == adminId,
                                   QStringLiteral("admin identity survives start"));

                success &= require(adminServer.returnToRoom(),
                                   QStringLiteral("claimed admin returns the room"));
                success &= require(!adminServer.running()
                                       && adminServer.adminSessionId() == adminId
                                       && adminServer.roomHostSessionId() == adminId,
                                   QStringLiteral("admin identity is stable across return-to-room"));
                success &= require(adminServer.returnToRoom(),
                                   QStringLiteral("returning to the room again is idempotent"));
                success &= require(adminServer.adminSessionId() == adminId,
                                   QStringLiteral("idempotent return keeps the admin identity"));
                success &= require(adminServer.startMatch(adminLevel),
                                   QStringLiteral("the same admin starts the next match"));
                success &= require(adminServer.adminSessionId() == adminId,
                                   QStringLiteral("admin identity survives retry/restart"));

                // 管理员掉线后：陈旧 admin 不会转给 guest，start/return 都被拒绝。
                hostClient.close();
                success &= require(pumpNetworkUntil([&] {
                                       return adminServer.roomPlayers() == 1;
                                   }, 3000),
                                   QStringLiteral("admin disconnect leaves the guest in the room"));
                QString noAdminStartError;
                success &= require(!adminServer.startMatch(adminLevel, &noAdminStartError)
                                       && !noAdminStartError.isEmpty()
                                       && adminServer.adminSessionId() == adminId,
                                   QStringLiteral("start is refused after the admin disconnects"));
                QString noAdminReturnError;
                success &= require(!adminServer.returnToRoom(&noAdminReturnError)
                                       && !noAdminReturnError.isEmpty(),
                                   QStringLiteral("return-to-room is refused after the admin disconnects"));
                success &= require(adminServer.adminSessionId() == adminId,
                                   QStringLiteral("stale admin id is not reassigned to the guest"));
                success &= require(!adminServer.claimAdmin(adminId),
                                   QStringLiteral("the disconnected admin session cannot be reclaimed"));
                guestClient.close();
                success &= require(pumpNetworkUntil([&] {
                                       return adminServer.clientCount() == 0;
                                   }, 3000),
                                   QStringLiteral("admin lifecycle server clears its sessions"));
                adminServer.close();
            }
        }

        log(QStringLiteral("Delayed network proxy"));
        {
            server::GameServer latencyServer;
            QString latencyError;
            const bool latencyListening = latencyServer.listen(0, true, &latencyError);
            success &= require(latencyListening,
                               QStringLiteral("latency test server listens (%1)")
                                   .arg(latencyError));
            LatencyProxy proxy;
            QString proxyError;
            const bool proxyListening =
                latencyListening && proxy.listen(latencyServer.port(), 90, &proxyError);
            success &= require(proxyListening,
                               QStringLiteral("test latency proxy listens (%1)")
                                   .arg(proxyError));
            if (proxyListening) {
                const Games::Level latencyLevel =
                    networkTestLevel(QStringLiteral("延迟网络"));
                client::GameClient delayed;
                QElapsedTimer handshakeClock;
                handshakeClock.start();
                success &= require(delayed.connectToHost(QStringLiteral("127.0.0.1"),
                                                         proxy.port()),
                                   QStringLiteral("client connects through the latency proxy"));
                success &= require(pumpNetworkUntil([&] { return delayed.welcomed(); },
                                                    5000),
                                   QStringLiteral("Hello/Welcome traverses the latency proxy"));
                const qint64 handshakeMs = handshakeClock.elapsed();
                success &= require(handshakeMs >= 80,
                                   QStringLiteral("proxy really delays the handshake (%1 ms)")
                                       .arg(handshakeMs));
                success &= require(proxy.accepted() == 1
                                       && proxy.bytesToServer() > 0
                                       && proxy.bytesToClient() > 0,
                                   QStringLiteral("proxy forwards bytes in both directions"));
                success &= require(pumpNetworkUntil([&] {
                                       return delayed.inLobby()
                                              && !delayed.rooms().empty()
                                              && delayed.rooms().front().players == 1;
                                   }, 4000),
                                   QStringLiteral("room state traverses the latency proxy"));
                success &= require(latencyServer.claimAdmin(delayed.sessionId()),
                                   QStringLiteral("admin claim works through the proxy connection"));
                success &= require(latencyServer.startMatch(latencyLevel),
                                   QStringLiteral("host start works with a delayed client"));
                success &= require(pumpNetworkUntil([&] {
                                       return delayed.snapshotCount() > 0;
                                   }, 5000),
                                   QStringLiteral("snapshots traverse the latency proxy"));
                proxy.setDelayMs(150);
                success &= require(proxy.delayMs() == 150,
                                   QStringLiteral("proxy delay is configurable (80-150 ms)"));
                const float delayedBefore = delayed.view().playerState().pos.x;
                net::InputCommand delayedMove;
                delayedMove.moveX = 1.0f;
                delayedMove.mouseActive = true;
                delayedMove.mouseX = delayedBefore + 220.0f;
                delayedMove.mouseY = delayed.view().playerState().pos.y;
                success &= require(delayed.sendInput(delayedMove),
                                   QStringLiteral("input is written through the delay proxy"));
                success &= require(pumpNetworkUntil([&] {
                                       return delayed.view().playerState().pos.x
                                              > delayedBefore + 4.0f;
                                   }, 5000),
                                   QStringLiteral("input and snapshot round-trip at 150 ms"));
                success &= require(latencyServer.snapshot().players.front().pos.x
                                       > delayedBefore + 4.0f,
                                   QStringLiteral("authoritative state moved behind the proxy"));
                delayed.close();
                success &= require(pumpNetworkUntil([&] {
                                       return latencyServer.clientCount() == 0;
                                   }, 3000),
                                   QStringLiteral("delayed client disconnects cleanly"));
                proxy.close();
            }
            latencyServer.close();
        }

        log(QStringLiteral("Multiplayer sessions over TCP"));
        {
            server::GameServer multiServer;
            QString multiError;
            const bool multiListening = multiServer.listen(0, true, &multiError);
            success &= require(multiListening,
                               QStringLiteral("multiplayer test server listens (%1)")
                                   .arg(multiError));
            if (multiListening) {
                Games::Level multiLevel;
                multiLevel.name = QStringLiteral("多人 TCP");
                multiLevel.startWave = 1;
                multiLevel.enemyLevel = 1;
                multiLevel.script.push_back(games::Wave{{games::EnemySpawn{
                    arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});

                client::GameClient alpha;
                client::GameClient beta;
                success &= require(alpha.connectToHost(QStringLiteral("127.0.0.1"),
                                                       multiServer.port())
                                       && beta.connectToHost(QStringLiteral("127.0.0.1"),
                                                             multiServer.port()),
                                   QStringLiteral("two clients connect before the match"));
                success &= require(pumpUntil([&] {
                                       return alpha.welcomed() && beta.welcomed()
                                              && alpha.inLobby() && beta.inLobby();
                                   }, 3000),
                                   QStringLiteral("both clients complete Hello/Welcome"));
                success &= require(alpha.playerId() > 0 && beta.playerId() > 0
                                       && alpha.playerId() != beta.playerId()
                                       && multiServer.roomPlayers() == 2,
                                   QStringLiteral("server assigns each session a stable slot"));
                success &= require(multiServer.claimAdmin(alpha.sessionId()),
                                   QStringLiteral("first session claims admin"));
                success &= require(multiServer.startMatch(multiLevel),
                                   QStringLiteral("host starts a match with two sessions"));
                success &= require(pumpUntil([&] {
                                       return alpha.snapshotCount() > 0
                                              && beta.snapshotCount() > 0;
                                   }, 3000),
                                   QStringLiteral("both sessions receive snapshots"));
                const net::Snapshot &alphaSnap = alpha.lastSnapshot();
                const net::Snapshot &betaSnap = beta.lastSnapshot();
                success &= require(alphaSnap.players.size() == 2
                                       && betaSnap.players.size() == 2
                                       && alphaSnap.localPlayerId == alpha.playerId()
                                       && betaSnap.localPlayerId == beta.playerId()
                                       && alpha.view().playerState().playerId
                                              == alpha.playerId()
                                       && beta.view().playerState().playerId == beta.playerId(),
                                   QStringLiteral("each client gets both aircraft and its own local id"));
                success &= require(alphaSnap.players[0].playerId
                                          != alphaSnap.players[1].playerId
                                       && alphaSnap.players[0].pos
                                              != alphaSnap.players[1].pos
                                       && alphaSnap.players[0].color
                                              != alphaSnap.players[1].color,
                                   QStringLiteral("two sessions get distinct aircraft"));

                auto playerPos = [](const net::Snapshot &snapshot, int playerId) {
                    for (const net::PlayerSnapshot &player : snapshot.players) {
                        if (player.playerId == playerId) {
                            return player.pos;
                        }
                    }
                    return Math::Vec2{};
                };
                const Math::Vec2 alphaBefore = playerPos(alphaSnap, alpha.playerId());
                const Math::Vec2 betaBefore = playerPos(betaSnap, beta.playerId());
                net::InputCommand moveRight;
                moveRight.moveX = 1.0f;
                moveRight.mouseActive = true;
                moveRight.mouseX = alphaBefore.x + 220.0f;
                moveRight.mouseY = alphaBefore.y;
                success &= require(alpha.sendInput(moveRight),
                                   QStringLiteral("first player's input reaches the server"));
                success &= require(pumpUntil([&] {
                                       return playerPos(alpha.lastSnapshot(),
                                                        alpha.playerId()).x
                                              > alphaBefore.x + 4.0f;
                                   }, 3000),
                                   QStringLiteral("only the sender's authoritative aircraft moves"));
                const Math::Vec2 betaAfterMove = playerPos(beta.lastSnapshot(), beta.playerId());
                success &= require(betaAfterMove.distanceTo(betaBefore) < 1.5f,
                                   QStringLiteral("the other session's aircraft stays put"));

                // 停住自己的输入，再让开局后加入的第三会话发输入：观战者不能控制任何飞机。
                net::InputCommand stopMoving;
                stopMoving.mouseActive = true;
                stopMoving.mouseX = 0.0f;
                stopMoving.mouseY = 0.0f;
                alpha.sendInput(stopMoving);
                QElapsedTimer settleWait;
                settleWait.start();
                while (settleWait.elapsed() < 420) {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                    QThread::msleep(1);
                }

                client::GameClient spectator;
                success &= require(spectator.connectToHost(QStringLiteral("127.0.0.1"),
                                                           multiServer.port()),
                                   QStringLiteral("a late client connects while the match runs"));
                success &= require(pumpUntil([&] {
                                       return spectator.welcomed()
                                              && spectator.snapshotCount() > 0;
                                   }, 3000),
                                   QStringLiteral("late client receives snapshot updates"));
                success &= require(spectator.localPlayerId() == 0
                                       && spectator.spectating()
                                       && spectator.lastSnapshot().players.size() == 2,
                                   QStringLiteral("a late session is a spectator without an aircraft"));
                const Math::Vec2 alphaIdle = playerPos(alpha.lastSnapshot(), alpha.playerId());
                net::InputCommand spectatorMove;
                spectatorMove.moveX = -1.0f;
                spectatorMove.mouseActive = true;
                spectatorMove.mouseX = alphaIdle.x - 300.0f;
                spectatorMove.mouseY = alphaIdle.y;
                spectator.sendInput(spectatorMove);
                QElapsedTimer spectatorWait;
                spectatorWait.start();
                while (spectatorWait.elapsed() < 260) {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                    QThread::msleep(1);
                }
                const Math::Vec2 alphaAfterSpectator =
                    playerPos(alpha.lastSnapshot(), alpha.playerId());
                success &= require(alphaAfterSpectator.distanceTo(alphaIdle) < 3.0f,
                                   QStringLiteral("spectator input is ignored by the server"));

                // 玩家中途离开：服务器移除那一架飞机，另一个玩家继续接收快照。
                beta.close();
                success &= require(pumpUntil([&] {
                                       return multiServer.clientCount() == 2;
                                   }, 3000),
                                   QStringLiteral("server drops a mid-match session on disconnect"));
                success &= require(pumpUntil([&] {
                                       return alpha.lastSnapshot().players.size() == 1
                                              && alpha.lastSnapshot().localPlayerId
                                                     == alpha.playerId()
                                              && alpha.lastSnapshot().players.front().playerId
                                                     == alpha.playerId();
                                   }, 3000),
                                   QStringLiteral("disconnect removes only that aircraft; the match continues"));
                for (int i = 0; i < 30; ++i) {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                    QThread::msleep(1);
                }
                success &= require(alpha.snapshotCount() > 0 && !alpha.failed()
                                       && multiServer.roomPlayers() == 2,
                                   QStringLiteral("remaining player and spectator stay connected"));

                alpha.close();
                spectator.close();
                success &= require(pumpUntil([&] {
                                       return multiServer.clientCount() == 0;
                                   }, 3000),
                                   QStringLiteral("multiplayer server tears down cleanly"));
                multiServer.close();
            }
        }

        log(QStringLiteral("Reconnect path"));
        {
            server::GameServer reconnectServer;
            QString reconnectError;
            const bool reconnectListening = reconnectServer.listen(0, true, &reconnectError);
            success &= require(reconnectListening,
                               QStringLiteral("reconnect test server listens (%1)")
                                   .arg(reconnectError));
            if (reconnectListening) {
                client::GameClient reconnectClient;
                reconnectClient.setAutoReconnect(true);
                success &= require(reconnectClient.connectToHost(
                                       QStringLiteral("127.0.0.1"),
                                       reconnectServer.port()),
                                   QStringLiteral("reconnect test client starts connecting"));
                success &= require(pumpUntil([&] { return reconnectClient.welcomed(); }, 3000),
                                   QStringLiteral("reconnect test client completes handshake"));
                reconnectServer.close();
                success &= require(pumpUntil([&] {
                                       return reconnectClient.reconnectAttempts() >= 1;
                                   }, 3000),
                                   QStringLiteral("disconnect schedules a bounded reconnect"));
                reconnectClient.close();
            }
        }

        log(QStringLiteral("Embedded single-player network path"));
        Menu networkMenu(false, false);
        networkMenu.startLevel(0);
        client::GameClient *netClient = networkMenu.match_.gameClient();
        success &= require(netClient != nullptr && networkMenu.match_.localServer() != nullptr,
                           QStringLiteral("default menu path starts an embedded loopback server"));
        success &= require(pumpUntil([&] {
                               return netClient != nullptr && netClient->snapshotCount() > 0;
                           }, 4000),
                           QStringLiteral("embedded server and client exchange snapshots"));
        if (netClient != nullptr) {
            success &= require(netClient->view().waves().totalWaves == 8,
                               QStringLiteral("embedded match runs the submitted campaign script"));
            networkMenu.update(1.0f / 60.0f);
            networkMenu.render();
            success &= require(networkMenu.match_.frame().size()
                                   == QSize(GameConfig::ScreenWidth,
                                            GameConfig::ScreenHeight),
                               QStringLiteral("embedded network match renders a frame"));
            const float embeddedBefore = netClient->view().playerState().pos.x;
            networkMenu.keyPress(Qt::Key_D);
            QElapsedTimer embeddedTimer;
            embeddedTimer.start();
            while (embeddedTimer.elapsed() < 700) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                networkMenu.update(1.0f / 60.0f);
                QThread::msleep(1);
            }
            networkMenu.keyRelease(Qt::Key_D);
            success &= require(netClient->view().playerState().pos.x
                                   > embeddedBefore + 3.0f,
                               QStringLiteral("embedded single-player still controls its own aircraft"));
        }
        networkMenu.returnToMenu();

        log(QStringLiteral("Room lobby lifecycle"));
        auto pumpMenu = [&](Menu &roomMenu, const std::function<bool()> &predicate,
                            int timeoutMs) {
            QElapsedTimer timer;
            timer.start();
            while (!predicate() && timer.elapsed() < timeoutMs) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                roomMenu.update(1.0f / 60.0f);
                QThread::msleep(1);
            }
            return predicate();
        };

        QTcpServer roomProbe;
        success &= require(roomProbe.listen(QHostAddress::LocalHost, 0),
                           QStringLiteral("room test reserves a host port"));
        const quint16 roomPort = roomProbe.serverPort();
        roomProbe.close();

        Menu hostMenu(false, false);
        hostMenu.keyPress(Qt::Key_2);
        hostMenu.keyPress(Qt::Key_F2);
        hostMenu.editor_.setPort(roomPort);
        hostMenu.keyPress(Qt::Key_Return);
        success &= require(hostMenu.screen() == Menu::Screen::Room
                               && hostMenu.match_.inRoom()
                               && hostMenu.match_.roomHost(),
                           QStringLiteral("host ENTER enters the room instead of the match"));
        success &= require(hostMenu.match_.localServer() != nullptr
                               && hostMenu.match_.localServer()->isListening()
                               && !hostMenu.match_.localServer()->running()
                               && hostMenu.match_.gameClient() != nullptr
                               && hostMenu.match_.gameClient()->snapshotCount() == 0,
                           QStringLiteral("host room owns a listening server without a match"));
        success &= require(pumpMenu(hostMenu, [&] {
                               return hostMenu.match_.roomConnected()
                                      && hostMenu.match_.roomPlayers() == 1;
                           }, 4000),
                           QStringLiteral("host appears in its own room with one player"));
        success &= require(hostMenu.match_.roomYouAreHost()
                               && !hostMenu.match_.roomName().isEmpty()
                               && hostMenu.match_.localServer()->roomHostSessionId()
                                      == hostMenu.match_.gameClient()->sessionId()
                               && hostMenu.roomView().canStart,
                           QStringLiteral("room view marks the host, name and player count"));
        menu::Room roomScreen;
        menu::Room::View hostRoomView = hostMenu.roomView();
        success &= require(roomScreen.keyPress(Qt::Key_Return, false, hostRoomView)
                                   == menu::Room::Action::Start
                               && roomScreen.keyPress(Qt::Key_X, false, hostRoomView)
                                      == menu::Room::Action::Dissolve
                               && roomScreen.keyPress(Qt::Key_Escape, false, hostRoomView)
                                      == menu::Room::Action::Back,
                           QStringLiteral("room host controls are ENTER start / X dissolve / ESC back"));

        Menu joinMenu(false, false);
        joinMenu.keyPress(Qt::Key_2);
        joinMenu.keyPress(Qt::Key_F3);
        joinMenu.editor_.setHost(QStringLiteral("127.0.0.1"));
        joinMenu.editor_.setPort(roomPort);
        joinMenu.keyPress(Qt::Key_Return);
        success &= require(joinMenu.screen() == Menu::Screen::Room
                               && joinMenu.match_.inRoom()
                               && !joinMenu.match_.roomHost(),
                           QStringLiteral("joining a room opens the same lobby screen"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.match_.roomConnected()
                                      && joinMenu.match_.roomPlayers() == 2;
                           }, 4000),
                           QStringLiteral("joined client sees the authoritative player count"));
        success &= require(pumpMenu(hostMenu, [&] {
                               return hostMenu.match_.roomPlayers() == 2;
                           }, 3000),
                           QStringLiteral("host room count updates when a client joins"));
        success &= require(hostMenu.match_.roomName() == joinMenu.match_.roomName()
                               && joinMenu.match_.roomMaxPlayers() == 4
                               && !joinMenu.match_.roomYouAreHost(),
                           QStringLiteral("both sides share the same room name and host marker"));
        {
            // 房间槽位必须和服务端会话一致：主人永远是 P1/HOST，加入者是 P2。
            const menu::Room::View hostSlots = hostMenu.roomView();
            const menu::Room::View joinSlots = joinMenu.roomView();
            const auto hostSlot = std::find_if(
                hostSlots.playerSlots.begin(), hostSlots.playerSlots.end(),
                [](const menu::Room::PlayerSlot &slot) { return slot.you; });
            const auto joinSlot = std::find_if(
                joinSlots.playerSlots.begin(), joinSlots.playerSlots.end(),
                [](const menu::Room::PlayerSlot &slot) { return slot.you; });
            success &= require(hostSlot != hostSlots.playerSlots.end()
                                   && hostSlot->host
                                   && joinSlot != joinSlots.playerSlots.end()
                                   && !joinSlot->host
                                   && hostSlots.playerSlots.size() == 2
                                   && joinSlots.playerSlots.size() == 2,
                               QStringLiteral("room slot list marks host and joiner correctly"));
        }
        joinMenu.keyPress(Qt::Key_Return);
        success &= require(joinMenu.match_.inRoom()
                               && !hostMenu.match_.localServer()->running(),
                           QStringLiteral("only the host can start the match from the room"));

        hostMenu.keyPress(Qt::Key_Return);
        success &= require(hostMenu.match_.localServer() != nullptr
                               && hostMenu.match_.localServer()->running(),
                           QStringLiteral("host ENTER starts the authoritative match"));
        success &= require(pumpMenu(hostMenu, [&] {
                               return hostMenu.screen() == Menu::Screen::Match
                                      && hostMenu.match_.gameClient() != nullptr
                                      && hostMenu.match_.gameClient()->snapshotCount() > 0;
                           }, 4000),
                           QStringLiteral("host switches from room to the running match"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.screen() == Menu::Screen::Match
                                      && joinMenu.match_.gameClient() != nullptr
                                      && joinMenu.match_.gameClient()->snapshotCount() > 0;
                           }, 4000),
                           QStringLiteral("joined client follows the host start into the match"));
        success &= require(hostMenu.match_.totalWaves() >= 1
                               && joinMenu.match_.totalWaves()
                                      == hostMenu.match_.totalWaves(),
                           QStringLiteral("host start uses the submitted script for every client"));

        // 房主/加入者都停在结果页时，房主“重新部署”必须原地回到房间：
        // 不重建服务器、不换会话、不丢管理员身份。
        const quint32 joinSessionBeforeRestart =
            joinMenu.match_.gameClient() != nullptr
                ? joinMenu.match_.gameClient()->sessionId()
                : 0;
        server::GameServer *hostServerBeforeRetry = hostMenu.match_.localServer();
        const quint32 hostSessionBeforeRetry =
            hostMenu.match_.gameClient() != nullptr
                ? hostMenu.match_.gameClient()->sessionId()
                : 0;
        const quint32 adminBeforeRetry =
            hostServerBeforeRetry != nullptr ? hostServerBeforeRetry->adminSessionId() : 0;
        hostMenu.screen_ = Menu::Screen::Result;
        joinMenu.screen_ = Menu::Screen::Result;
        hostMenu.retry();
        success &= require(hostMenu.screen() == Menu::Screen::Room
                               && hostMenu.match_.localServer() == hostServerBeforeRetry
                               && hostServerBeforeRetry != nullptr
                               && !hostServerBeforeRetry->running()
                               && hostMenu.match_.gameClient() != nullptr
                               && hostMenu.match_.gameClient()->sessionId()
                                      == hostSessionBeforeRetry
                               && hostServerBeforeRetry->adminSessionId()
                                      == adminBeforeRetry,
                           QStringLiteral("host retry from the result screen reuses server/admin/session"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.screen() == Menu::Screen::Room
                                      && joinMenu.match_.gameClient() != nullptr
                                      && joinMenu.match_.gameClient()->sessionId()
                                             == joinSessionBeforeRestart;
                           }, 4000),
                           QStringLiteral("result-screen clients follow the host back to the room"));
        // 重复 retry 不应重复建房或让会话漂移。
        hostMenu.retry();
        success &= require(hostMenu.screen() == Menu::Screen::Room
                               && hostMenu.match_.localServer() == hostServerBeforeRetry
                               && hostMenu.match_.gameClient() != nullptr
                               && hostMenu.match_.gameClient()->sessionId()
                                      == hostSessionBeforeRetry
                               && hostServerBeforeRetry->adminSessionId()
                                      == adminBeforeRetry,
                           QStringLiteral("repeated host retry stays in the same room and session"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.screen() == Menu::Screen::Room
                                      && joinMenu.match_.gameClient() != nullptr
                                      && joinMenu.match_.gameClient()->sessionId()
                                             == joinSessionBeforeRestart;
                           }, 2000),
                           QStringLiteral("repeated host retry keeps the joined session stable"));

        hostMenu.keyPress(Qt::Key_Return);
        success &= require(hostMenu.match_.localServer()->running(),
                           QStringLiteral("host starts the next match from the room"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.screen() == Menu::Screen::Match
                                      && joinMenu.match_.gameClient() != nullptr
                                      && joinMenu.match_.gameClient()->sessionId()
                                             == joinSessionBeforeRestart
                                      && joinMenu.match_.gameClient()->snapshotCount() > 0;
                           }, 4000),
                           QStringLiteral("joined client enters the restarted match"));

        // 客户端仍停在结算页时房主 ESC 离开：房主解散，加入者进入失败页。
        hostMenu.screen_ = Menu::Screen::Result;
        joinMenu.screen_ = Menu::Screen::Result;
        hostMenu.keyPress(Qt::Key_Escape);
        success &= require(hostMenu.screen() == Menu::Screen::Main
                               && hostMenu.match_.localServer() == nullptr,
                           QStringLiteral("host leaving the room closes the embedded server"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.screen() == Menu::Screen::Failure;
                           }, 4000),
                           QStringLiteral("host leaving the room sends clients to failure"));
        success &= require(joinMenu.failureView().title.contains(QStringLiteral("房间已解散")),
                           QStringLiteral("host leaving is reported as room dissolved"));
        success &= require(joinMenu.match_.failed()
                               && joinMenu.failureView().retryable,
                           QStringLiteral("client failure state is terminal but retryable"));

        joinMenu.keyPress(Qt::Key_Return);
        success &= require(joinMenu.screen() == Menu::Screen::Room
                               && joinMenu.match_.active(),
                           QStringLiteral("failure retry recreates the connection attempt"));
        success &= require(pumpMenu(joinMenu, [&] {
                               return joinMenu.screen() == Menu::Screen::Failure;
                           }, 8000),
                           QStringLiteral("retry against an absent server reaches failure"));
        success &= require(joinMenu.failureView().title.contains(
                               QStringLiteral("无法连接服务器")),
                           QStringLiteral("failure screen distinguishes a missing server"));
        joinMenu.keyPress(Qt::Key_Escape);
        success &= require(joinMenu.screen() == Menu::Screen::Main
                               && !joinMenu.match_.active(),
                           QStringLiteral("failure screen ESC returns to the main menu"));

        // 服务器接受 TCP 但从不 Welcome：必须有连接超时，不能一直停在空房间；
        // 连续重试每次只开一条新连接，不会堆积房间或挂起。
        QTcpServer silentServer;
        success &= require(silentServer.listen(QHostAddress::LocalHost, 0),
                           QStringLiteral("silent server reserves a port"));
        int silentAccepts = 0;
        QObject::connect(&silentServer, &QTcpServer::newConnection, [&] {
            while (QTcpSocket *socket = silentServer.nextPendingConnection()) {
                ++silentAccepts;
                socket->setParent(&silentServer);   // 接受但永远不写 Welcome
            }
        });
        Menu silentJoin(false, false);
        silentJoin.keyPress(Qt::Key_2);
        silentJoin.keyPress(Qt::Key_F3);
        silentJoin.editor_.setHost(QStringLiteral("127.0.0.1"));
        silentJoin.editor_.setPort(silentServer.serverPort());
        silentJoin.keyPress(Qt::Key_Return);
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (attempt > 0) {
                silentJoin.keyPress(Qt::Key_Return);
            }
            success &= require(pumpMenu(silentJoin, [&] {
                                   return silentJoin.screen() == Menu::Screen::Failure;
                               }, 7000),
                               QStringLiteral("silent server attempt %1 times out instead of hanging")
                                   .arg(attempt + 1));
            success &= require(silentAccepts == attempt + 1,
                               QStringLiteral("silent retry %1 opens exactly one connection")
                                   .arg(attempt + 1));
            success &= require(silentJoin.match_.localServer() == nullptr
                                   && silentJoin.match_.gameClient() != nullptr,
                               QStringLiteral("silent retry %1 never fabricates a room server")
                                   .arg(attempt + 1));
        }
        success &= require(silentJoin.failureView().title.contains(
                               QStringLiteral("不存在")),
                           QStringLiteral("timeout is presented as a missing room"));
        success &= require(silentAccepts == 3,
                           QStringLiteral("repeated timeout retries leave three sockets, not duplicates"));
        silentJoin.returnToMenu();
        silentServer.close();

        hostMenu.returnToMenu();

        log(success ? QStringLiteral("ALL C++ SMOKE TESTS PASSED")
                    : QStringLiteral("SMOKE TESTS FAILED"));
        logFile.close();
        return success ? 0 : 1;
    }

    // 性能基准（--bench）：固定场景下测 render / update 的每帧耗时。
    // 用于对比优化前后（配合 NEON_SINGLE_THREAD=1 做单线程对照）。
    static int bench()
    {
        Menu menu(false, true);
        menu.startLevel(0);
        Games *game = menu.match_.game();
        if (game == nullptr) {
            return 1;
        }

        Arena &arena = game->arena_;
        auto ctx = arena.context();
        game->state_ = Games::State::Playing;
        game->runTime_ = 112.0f;
        game->waves_.start(4);
        game->waves_.queue_ = {arena::EnemyType(typeid(arena::DroneEnemy)),
                               arena::EnemyType(typeid(arena::SpitterEnemy))};
        game->waves_.waveTotal_ = 17;

        arena.enemies().clear();
        arena.portals().clear();
        arena.pickups().clear();
        arena.playerBullets().clear();
        arena.clearEnemy();
        arena.particles().clear();
        arena.floatingTexts().clear();
        arena.shockwaves().clear();

        Arena::PlayerState &player = arena.playerState();
        player.pos = {475.0f, 430.0f};
        player.hp = 74.0f;
        player.hpDisplay = 84.0f;
        player.aim = -0.55f;
        player.invuln = 99.0f;
        BaseService.mouseMove({1010.0f, 265.0f});

        const std::array<const char *, 12> kinds = {
            "drone", "drone", "drone", "drone", "spitter", "spitter",
            "tank", "splitter", "amplifier", "mender", "linker", "summoner"};
        const std::array<Math::Vec2, 12> spots = {{
            {820, 285}, {960, 520}, {700, 165}, {1080, 360},
            {950, 210}, {1120, 470}, {760, 445}, {600, 210},
            {860, 340}, {930, 470}, {790, 520}, {690, 300}}};
        for (int i = 0; i < 12; ++i) {
            arena.spawnEnemy(ctx, QString::fromLatin1(kinds[size_t(i)]), spots[size_t(i)], 4);
        }
        arena.portals().emplace_back(arena::EnemyType(typeid(arena::SpitterEnemy)),
                                     Math::Vec2(1160, 615), 4);
        arena.spawnPickup(ctx, {360, 315}, 16);
        arena.player().shoot(ctx);
        arena.spawnBurst(ctx, {860, 290}, Base::Coral, 14, 130.0f, 0.7f);
        arena.spawnBurst(ctx, {715, 400}, Base::Cyan, 8, 100.0f, 0.5f);
        arena.addShockwave({860, 290}, 48.0f, Base::Coral, 0.6f, 2);
        for (int i = 0; i < 6; ++i) {
            arena.submit(std::make_unique<arena::StandardBullet>(
                Math::Vec2(650.0f + float(i) * 60.0f, 360.0f),
                Math::Vec2(900.0f, -250.0f + float(i) * 40.0f), 12.5f, Base::Cyan,
                Bullet::Standpoint::Player, 3.2f));
        }
        game->renderer_.showBanner(QStringLiteral("第 4 波"),
                                   QStringLiteral("敌方信号正在接近"), 2.0f, Base::Cyan);
        game->renderer_.bannerTimer_ = 1.6f;

        for (int i = 0; i < 30; ++i) {
            game->render();
        }

        QElapsedTimer timer;
        constexpr int frames = 300;
        timer.start();
        for (int i = 0; i < frames; ++i) {
            game->render();
        }
        const double renderMs = double(timer.nsecsElapsed()) / 1e6 / frames;

        BaseService.mousePressLeft({1010.0f, 265.0f});
        constexpr int steps = 600;
        timer.restart();
        for (int i = 0; i < steps; ++i) {
            arena.playerState().invuln = 999.0f;
            arena.update(1.0f / 60.0f);
        }
        const double updateMs = double(timer.nsecsElapsed()) / 1e6 / steps;
        BaseService.mouseReleaseLeft();

        std::printf("BENCH frames=%d render=%.3f ms/frame update=%.3f ms/frame workers=%d pooled=%d\n",
                    frames, renderMs, updateMs, BaseService.parallelWorkers(),
                    arena.pooledBulletCount());
        std::fflush(stdout);
        return 0;
    }

    static int capture(const QString &directory)
    {
        QDir dir(directory);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            return 1;
        }
        Menu menu(false, true);
        Games *game = nullptr;
        // 截图模拟窗口的合成顺序：画面 + 全局指针（BaseService 的 base::Pointer 组件）。
        auto movePointer = [&](const Math::Vec2 &position) {
            menu.mouseMove(position);
            BaseService.pointerMoveTo(position);
        };
        auto saveMenu = [&](Menu &screenMenu, const QString &name) {
            screenMenu.render();
            const QString path = dir.filePath(name);
            QImage frame = screenMenu.frame().copy();
            {
                QPainter overlay(&frame);
                BaseService.pointerDraw(overlay);
            }
            if (!frame.save(path)) {
                std::fprintf(stderr, "Failed to save %s\n", path.toUtf8().constData());
            } else {
                std::printf("%s\n", path.toUtf8().constData());
            }
        };
        auto save = [&](const QString &name) { saveMenu(menu, name); };

        for (int i = 0; i < 120; ++i) {
            menu.update(1.0f / 60.0f);
        }
        movePointer({GameConfig::ScreenWidth / 2.0f, 454.0f});

        // 星区地图
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        save(QStringLiteral("map.png"));
        menu.keyPress(Qt::Key_Escape);
        menu.keyRelease(Qt::Key_Escape);

        // 商店
        ShopService.score().total_ = 2400;
        ShopService.inventory().addMine(2);
        const QRectF shopEntry = menu.main_.shopButtonRect();
        movePointer({float(shopEntry.center().x()), float(shopEntry.center().y())});
        menu.mousePressLeft({float(shopEntry.center().x()),
                             float(shopEntry.center().y())});
        save(QStringLiteral("shop.png"));
        menu.keyPress(Qt::Key_Escape);

        // 战备
        LoadoutService.select(1);
        const QRectF armoryEntry = menu.main_.armoryButtonRect();
        movePointer({float(armoryEntry.center().x()),
                     float(armoryEntry.center().y())});
        menu.mousePressLeft({float(armoryEntry.center().x()),
                             float(armoryEntry.center().y())});
        menu.mouseReleaseLeft();
        save(QStringLiteral("armory.png"));
        menu.keyPress(Qt::Key_Escape);
        LoadoutService.select(0);

        // 应用图标（程序化绘制）
        Base::appIcon(256).save(dir.filePath(QStringLiteral("icon.png")));
        const QRectF settingsEntry = menu.main_.settingsButtonRect();
        menu.mousePressLeft({float(settingsEntry.center().x()),
                             float(settingsEntry.center().y())});
        menu.mouseReleaseLeft();
        menu.keyPress(Qt::Key_Escape);

        const QRectF customCard = menu.main_.levelRect(Menu::CustomLevelIndex, int(menu.levels_.size()));
        movePointer({float(customCard.center().x()), float(customCard.center().y())});
        menu.update(1.0f / 60.0f);
        save(QStringLiteral("custom_level.png"));

        menu.keyPress(Qt::Key_Return);
        for (int i = 0; i < 2; ++i) {
            menu.keyPress(Qt::Key_N);
        }
        menu.keyPress(Qt::Key_Down);
        menu.keyPress(Qt::Key_Right);
        for (int i = 0; i < 3; ++i) {
            menu.keyPress(Qt::Key_Plus);
        }
        const QRectF spitterChip = menu.editor_.paletteChipRect(1);
        const QRectF dropRow = menu.editor_.rowRect(1);
        menu.mousePressLeft({float(spitterChip.center().x()),
                             float(spitterChip.center().y())});
        menu.mouseMove({float(dropRow.center().x()), float(dropRow.center().y())});
        menu.mouseReleaseLeft();
        save(QStringLiteral("editor.png"));
        menu.keyPress(Qt::Key_F2);
        menu.editor_.setPort(8899);
        movePointer({640, 324});
        save(QStringLiteral("server_setup.png"));
        menu.keyPress(Qt::Key_F3);
        menu.editor_.setHost(QStringLiteral("192.168.1.9"));
        movePointer({640, 344});
        save(QStringLiteral("server_join.png"));
        menu.keyPress(Qt::Key_Escape);

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->runTime_ = 112.0f;
        game->waves_.start(4);
        game->waves_.queue_ = {arena::EnemyType(typeid(arena::DroneEnemy)),
                               arena::EnemyType(typeid(arena::SpitterEnemy))};
        game->waves_.waveTotal_ = 17;
        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.pickups().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        game->arena_.particles().clear();
        game->arena_.floatingTexts().clear();
        game->arena_.shockwaves().clear();
        ShopService.score().total_ = 2840;
        ShopService.score().kills_ = 31;
        Arena::PlayerState &player = game->arena_.player().state;
        player.pos = {475, 430};
        player.hp = 74;
        player.hpDisplay = 84;
        player.aim = -0.55f;
        player.invuln = 99.0f;
        movePointer({1010, 265});

        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {820, 285}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {960, 520}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {700, 165}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {1080, 360}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("spitter"), {950, 210}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("spitter"), {1120, 470}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("tank"), {760, 445}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("splitter"), {600, 210}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("amplifier"), {860, 340}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("mender"), {930, 470}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("linker"), {790, 520}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("rusher"), {1050, 250}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("necromancer"), {1150, 560}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("summoner"), {690, 300}, 4);
        int enemyIndex = 0;
        for (const auto &enemy : game->arena_.enemies()) {
            enemy->age = 2.0f + float(enemyIndex) * 0.3f;
            if (enemyIndex < 3) {
                enemy->hp = enemy->maxHp * 0.55f;
            }
            ++enemyIndex;
        }
        game->arena_.portals().emplace_back(
            arena::EnemyType(typeid(arena::SpitterEnemy)), Math::Vec2(1160, 615), 4);
        game->arena_.spawnPickup(game->arena_.context(), {360, 315}, 16);
        game->arena_.player().shoot(game->arena_.context());
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(650, 360), Math::Vec2(900, -250), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(590, 325), Math::Vec2(950, -180), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(720, 400), Math::Vec2(880, -60), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(850, 300), Math::Vec2(-420, 150), 11.0f, Base::Magenta, Bullet::Standpoint::Enemy, 4.0f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(1010, 510), Math::Vec2(-380, -90), 11.0f, Base::Magenta, Bullet::Standpoint::Enemy, 4.0f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(1090, 420), Math::Vec2(-440, -30), 13.0f, QColor(255, 112, 190), Bullet::Standpoint::Enemy, 5.0f));
        game->arena_.spawnBurst(game->arena_.context(), {860, 290}, Base::Coral, 14, 130.0f, 0.7f);
        game->arena_.spawnBurst(game->arena_.context(), {715, 400}, Base::Cyan, 8, 100.0f, 0.5f);
        game->arena_.addShockwave({860, 290}, 48.0f, Base::Coral, 0.6f, 2);
        game->renderer_.showBanner(QStringLiteral("第 4 波"),
                                  QStringLiteral("敌方信号正在接近"), 1.8f, Base::Cyan);
        game->renderer_.bannerTimer_ = 1.3f;
        game->arena_.placeMine(Math::Vec2(520, 430));
        save(QStringLiteral("gameplay.png"));

        // 多玩家画面：同一份快照里有两架飞机，本机 HUD 只显示自己的生命/冲刺，
        // 远程飞机用不同颜色和识别环绘制。
        {
            games::GameSimulation::Level mpLevel;
            mpLevel.name = QStringLiteral("双机演示");
            mpLevel.startWave = 1;
            mpLevel.enemyLevel = 1;
            mpLevel.script.push_back(games::Wave{{games::EnemySpawn{
                arena::EnemyType(typeid(arena::DroneEnemy)), 3}}});
            games::GameSimulation mp(mpLevel);
            mp.setPlayers({1, 2});
            mp.start();
            Arena &mpArena = mp.arena_;
            const arena::Context mpCtx = mpArena.context();
            mpArena.playerStateAt(0).pos = {520, 470};
            mpArena.playerStateAt(0).hp = 74;
            mpArena.playerStateAt(0).hpDisplay = 84;
            mpArena.playerStateAt(0).aim = -0.42f;
            mpArena.playerStateAt(0).invuln = 0.0f;
            mpArena.playerStateAt(1).pos = {780, 520};
            mpArena.playerStateAt(1).hp = 91;
            mpArena.playerStateAt(1).hpDisplay = 100;
            mpArena.playerStateAt(1).aim = 2.2f;
            for (const auto &[kind, pos] : std::array<std::pair<const char *, Math::Vec2>, 6>{{
                     {"drone", {940, 270}}, {"drone", {1080, 560}},
                     {"spitter", {880, 380}}, {"tank", {700, 620}},
                     {"amplifier", {1030, 330}}, {"mender", {620, 250}}}}) {
                mpArena.spawnEnemy(mpCtx, QString::fromLatin1(kind), pos, 1);
            }
            for (const auto &enemy : mpArena.enemies()) {
                enemy->age = 1.4f;
            }
            mpArena.spawnStandard({560, 430}, {880, -160}, 12.5f, Base::Cyan,
                                  Bullet::Standpoint::Player, 3.0f);
            mpArena.spawnStandard({820, 470}, {-620, -200}, 12.5f, Base::Amber,
                                  Bullet::Standpoint::Player, 3.0f);
            mpArena.spawnStandard({980, 360}, {-540, 120}, 11.0f, Base::Magenta,
                                  Bullet::Standpoint::Enemy, 4.0f);
            mpArena.spawnBurst(mpCtx, {900, 300}, Base::Coral, 12, 130.0f, 0.6f);
            net::Snapshot mpSnapshot = mp.snapshot();
            client::GameView mpView;
            mpView.apply(mpSnapshot);
            games::Renderer mpRenderer;
            mpRenderer.initialize();
            mpRenderer.render(mpView);
            const QString path = dir.filePath(QStringLiteral("multiplayer.png"));
            if (!mpRenderer.frame().save(path)) {
                std::fprintf(stderr, "Failed to save %s\n", path.toUtf8().constData());
            } else {
                std::printf("%s\n", path.toUtf8().constData());
            }
        }

        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.pickups().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        game->arena_.particles().clear();
        game->arena_.floatingTexts().clear();
        game->arena_.shockwaves().clear();
        game->arena_.player().state.pos = {585, 420};
        game->waves_.start(4);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 15;
        game->enterUpgrade();
        movePointer({640, 390});
        save(QStringLiteral("upgrade.png"));

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 16;
        game->arena_.beginWave();
        game->runTime_ = 304.0f;
        ShopService.score().total_ = 9870;
        ShopService.score().kills_ = 63;
        game->arena_.enemies().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        game->arena_.particles().clear();
        game->arena_.shockwaves().clear();
        Arena::PlayerState &bossPlayer = game->arena_.player().state;
        bossPlayer.pos = {430, 435};
        bossPlayer.hp = 58;
        bossPlayer.hpDisplay = 71;
        bossPlayer.aim = -0.25f;
        bossPlayer.invuln = 99.0f;
        movePointer({850, 380});
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("boss"), {870, 365}, 8);
        game->arena_.boss()->hp =
            game->arena_.boss()->maxHp * 0.63f;
        game->arena_.boss()->hpDisplay =
            game->arena_.boss()->maxHp * 0.74f;
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {680, 250}, 8);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {740, 540}, 8);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("tank"), {1020, 500}, 8);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("spitter"), {1100, 300}, 8);
        game->arena_.spawnPickup(game->arena_.context(), {330, 285}, 16);
        for (int i = 0; i < 12; ++i) {
            const float angle = i * Math::Tau / 12.0f;
            const Math::Vec2 direction(std::cos(angle), std::sin(angle));
            game->arena_.submit(std::make_unique<arena::StandardBullet>(game->arena_.boss()->pos + direction * 58.0f,
                direction * 285.0f, 13.0f, Base::Magenta, Bullet::Standpoint::Enemy, 5.0f));
        }
        bossPlayer.fireFlash = 0.04f;
        game->arena_.player().shoot(game->arena_.context());
        game->arena_.spawnBurst(game->arena_.context(), game->arena_.boss()->pos,
                                         Base::Magenta, 12, 150.0f, 0.8f);
        game->renderer_.showBanner(QStringLiteral("最终波次"),
                                  QStringLiteral("警告：检测到「虚空吞噬者」"),
                                  2.8f, Base::Magenta, QStringLiteral("FINAL WAVE"));
        game->renderer_.bannerTimer_ = 0.0f;
        save(QStringLiteral("boss.png"));

        if (game->arena_.boss() != nullptr) {
            game->arena_.boss()->takeDamage(
                game->arena_.context(), arena::Damage{999999.0f, false, arena::DamageSource::Bullet});
        }
        // 胜负由台本决定：Boss 被毁只是战果，最后一波清空才进入结算。
        game->triggerVictory();
        for (int i = 0; i < 120; ++i) {
            game->update(1.0f / 60.0f);
        }
        menu.update(0.0f);
        // 结算 UI 由 QML ResultOverlay 绘制；离屏 capture 不生成第二套 Widgets 结算图。

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->waves_.start(3);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 12;
        game->runTime_ = 86.0f;
        ShopService.score().total_ = 1240;
        ShopService.score().kills_ = 17;
        game->arena_.player().state.hp = 8;
        game->arena_.player().state.invuln = 0.0f;
        game->arena_.player().takeDamage(
            game->arena_.context(), arena::Damage{20.0f, false, arena::DamageSource::Bullet});
        for (int i = 0; i < 30; ++i) {
            game->update(1.0f / 60.0f);
        }
        menu.update(0.0f);
        // 失败结算同样只由 QML 绘制。

        // 房间/大厅与房间失败页：真实内嵌服务端 + 两个真实 TCP 客户端。
        QTcpServer captureProbe;
        quint16 captureRoomPort = 0;
        if (captureProbe.listen(QHostAddress::LocalHost, 0)) {
            captureRoomPort = captureProbe.serverPort();
            captureProbe.close();
        }
        if (captureRoomPort != 0) {
            Menu hostRoom(false, false);
            hostRoom.keyPress(Qt::Key_2);
            hostRoom.keyPress(Qt::Key_F2);
            hostRoom.editor_.setPort(captureRoomPort);
            hostRoom.keyPress(Qt::Key_Return);

            auto spinRoom = [&](Menu &screenMenu, const std::function<bool()> &done,
                                int timeoutMs) {
                QElapsedTimer timer;
                timer.start();
                while (!done() && timer.elapsed() < timeoutMs) {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                    screenMenu.update(1.0f / 60.0f);
                    QThread::msleep(1);
                }
            };
            // 先把主人的回环客户端接入房间（拿到 P1/房主），再创建加入者，
            // 这样截图和真实操作顺序一致，slot 不会因事件顺序互换。
            spinRoom(hostRoom, [&] {
                return hostRoom.match_.roomConnected()
                       && hostRoom.match_.roomPlayers() == 1;
            }, 4000);

            Menu joinRoom(false, false);
            joinRoom.keyPress(Qt::Key_2);
            joinRoom.keyPress(Qt::Key_F3);
            joinRoom.editor_.setHost(QStringLiteral("127.0.0.1"));
            joinRoom.editor_.setPort(captureRoomPort);
            joinRoom.keyPress(Qt::Key_Return);

            spinRoom(joinRoom, [&] {
                return joinRoom.match_.roomConnected()
                       && joinRoom.match_.roomPlayers() == 2;
            }, 4000);
            spinRoom(hostRoom, [&] {
                return hostRoom.match_.roomPlayers() == 2;
            }, 2000);

            hostRoom.mouseMove({410.0f, 576.0f});
            BaseService.pointerMoveTo({410.0f, 576.0f});
            saveMenu(hostRoom, QStringLiteral("room_host.png"));
            joinRoom.mouseMove({880.0f, 576.0f});
            BaseService.pointerMoveTo({880.0f, 576.0f});
            saveMenu(joinRoom, QStringLiteral("room_join.png"));

            hostRoom.keyPress(Qt::Key_X);
            spinRoom(joinRoom, [&] {
                return joinRoom.screen() == Menu::Screen::Failure;
            }, 4000);
            joinRoom.mouseMove({510.0f, 532.0f});
            BaseService.pointerMoveTo({510.0f, 532.0f});
            // 失败页 UI 只在 QML FailureOverlay；离屏 capture 不再生成第二套 Widgets 图。
        }
        return 0;
    }
};

#endif // DEBUG
