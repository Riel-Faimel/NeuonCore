#include "Net/Server.h"

#include "Arena/Combatants/Enemy.h"
#include "Games/Simulation/GameSimulation.h"
#include "Shop/Shop.h"

#include <QHostAddress>
#include <QTcpSocket>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace server {

namespace {

// 服务端模拟：GameSimulation 的钩子只负责生成表现事件，服务端本身不含 Renderer。
class ServerSimulation final : public games::GameSimulation {
public:
    using games::GameSimulation::GameSimulation;

    std::function<void(const net::EventMessage &)> onEvent;

protected:
    void publish(net::EventMessage event)
    {
        if (onEvent) {
            onEvent(event);
        }
    }

    void onWaveStarted(int wave, bool boss, bool finalWave) override
    {
        net::EventMessage event;
        event.kind = net::EventKind::WaveStarted;
        event.wave = wave;
        event.boss = boss;
        event.finalWave = finalWave;
        publish(std::move(event));
    }

    void onUpgradeOpened() override
    {
        net::EventMessage event;
        event.kind = net::EventKind::UpgradeOpened;
        publish(std::move(event));
    }

    void onUpgradeSelected(const QColor &color) override
    {
        net::EventMessage event;
        event.kind = net::EventKind::UpgradeSelected;
        event.color = color;
        publish(std::move(event));
    }

    void onBossDown() override
    {
        net::EventMessage event;
        event.kind = net::EventKind::BossDown;
        publish(std::move(event));
    }

    void onWaveCleared(bool finalWave) override
    {
        net::EventMessage event;
        event.kind = net::EventKind::WaveCleared;
        event.finalWave = finalWave;
        publish(std::move(event));
    }

    void onVictory() override
    {
        net::EventMessage event;
        event.kind = net::EventKind::Victory;
        publish(std::move(event));
    }

    void onVictoryFinished() override
    {
        net::EventMessage event;
        event.kind = net::EventKind::VictoryFinished;
        publish(std::move(event));
    }

    void onDefeat() override
    {
        net::EventMessage event;
        event.kind = net::EventKind::Defeat;
        publish(std::move(event));
    }

    void onFeedback(const Arena::Feedback &feedback) override
    {
        net::EventMessage event;
        event.kind = net::EventKind::Feedback;
        event.color = feedback.color;
        event.alpha = feedback.alpha;
        event.shake = feedback.shake;
        publish(std::move(event));
    }
};

} // namespace

GameServer::GameServer(QObject *parent)
    : QObject(parent), tcpServer_(), tickTimer_()
{
    tickTimer_.setTimerType(Qt::PreciseTimer);
    tickTimer_.setInterval(16);
    connect(&tickTimer_, &QTimer::timeout, this, [this] { onTick(); });
    connect(&tcpServer_, &QTcpServer::newConnection, this, [this] {
        onNewConnection();
    });
}

GameServer::~GameServer()
{
    close();
}

bool GameServer::listen(quint16 port, bool loopbackOnly, QString *error)
{
    close();
    lastError_.clear();
    const QHostAddress address =
        loopbackOnly ? QHostAddress(QHostAddress::LocalHost)
                     : QHostAddress(QHostAddress::Any);
    if (!tcpServer_.listen(address, port)) {
        lastError_ = QStringLiteral("cannot listen on %1:%2: %3")
                         .arg(address.toString())
                         .arg(port)
                         .arg(tcpServer_.errorString());
        if (error != nullptr) {
            *error = lastError_;
        }
        return false;
    }
    return true;
}

void GameServer::close()
{
    tickTimer_.stop();
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        QTcpSocket *socket = it.key();
        if (socket != nullptr) {
            socket->disconnect(this);
            socket->abort();
            socket->deleteLater();
        }
    }
    sessions_.clear();
    controllerSocket_ = nullptr;
    simulation_.reset();
    snapshot_.clear();
    matchEndSent_ = false;
    accumulator_ = 0.0;
    tcpServer_.close();
}

void GameServer::onNewConnection()
{
    while (QTcpSocket *socket = tcpServer_.nextPendingConnection()) {
        Session session;
        session.socket = socket;
        session.id = nextSessionId_++;
        sessions_.insert(socket, session);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            onReadyRead(socket);
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            onDisconnected(socket);
        });
    }
}

void GameServer::onReadyRead(QTcpSocket *socket)
{
    auto it = sessions_.find(socket);
    if (it == sessions_.end()) {
        return;
    }
    it->frames.append(socket->readAll());
    for (;;) {
        QByteArray frame;
        QString error;
        const net::FrameBuffer::Result result = it->frames.next(&frame, &error);
        if (result == net::FrameBuffer::Result::NeedMore) {
            return;
        }
        if (result == net::FrameBuffer::Result::Error) {
            reject(socket, error);
            return;
        }
        handleFrame(socket, frame);
        if (!sessions_.contains(socket)
            || socket->state() == QAbstractSocket::UnconnectedState) {
            return;
        }
        it = sessions_.find(socket);
        if (it == sessions_.end()) {
            return;
        }
    }
}

void GameServer::onDisconnected(QTcpSocket *socket)
{
    if (controllerSocket_ == socket) {
        controllerSocket_ = nullptr;
    }
    sessions_.remove(socket);
    socket->deleteLater();
}

void GameServer::handleFrame(QTcpSocket *socket, const QByteArray &frame)
{
    net::MessageHeader header;
    QByteArray payload;
    QString error;
    if (!net::decodeFrame(frame, &header, &payload, &error)) {
        reject(socket, error);
        return;
    }
    if (header.version != net::kProtocolVersion) {
        reject(socket, QStringLiteral("protocol version mismatch: client %1, server %2")
                          .arg(header.version)
                          .arg(net::kProtocolVersion));
        return;
    }

    auto it = sessions_.find(socket);
    if (it == sessions_.end()) {
        return;
    }
    if (!it->hello && header.type != net::MessageType::Hello) {
        reject(socket, QStringLiteral("hello required before other messages"));
        return;
    }

    switch (header.type) {
    case net::MessageType::Hello:
        handleHello(socket, frame);
        break;
    case net::MessageType::StartMatch:
        // 控制面禁用网络：只有本地宿主（内嵌 Match / stdio）能开局。
        reject(socket, QStringLiteral("StartMatch is local-only; use the server host"));
        break;
    case net::MessageType::Input:
        handleInput(socket, frame);
        break;
    case net::MessageType::Welcome:
    case net::MessageType::Snapshot:
    case net::MessageType::Event:
    case net::MessageType::MatchEnd:
        reject(socket, QStringLiteral("server-only message type received from client"));
        break;
    }
}

void GameServer::handleHello(QTcpSocket *socket, const QByteArray &frame)
{
    net::HelloMessage hello;
    QString error;
    if (!net::decodeHello(frame, &hello, &error)) {
        reject(socket, error);
        return;
    }
    auto it = sessions_.find(socket);
    if (it == sessions_.end()) {
        return;
    }
    it->hello = true;
    it->welcomed = true;
    if (controllerSocket_ == nullptr) {
        // 第一个连接到本地内嵌服务端的客户端是主控玩家；后续客户端只读。
        controllerSocket_ = socket;
    }
    sendWelcome(socket, true, QString());
}

bool GameServer::startMatch(const games::GameSimulation::Level &level, QString *error)
{
    if (level.script.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("start match carries no waves");
        }
        return false;
    }

    auto simulation = std::make_unique<ServerSimulation>(level);
    simulation->onEvent = [this](const net::EventMessage &event) {
        broadcastEvent(event);
    };
    simulation_ = std::move(simulation);
    matchEndSent_ = false;

    // 服务端是比分/波次的权威：每局开始时重置本局统计，击杀由 Arena 写回 ShopService。
    ShopService.score().beginRun();

    tickClock_.start();
    accumulator_ = 0.0;
    simulation_->start();
    simulation_->writeSnapshot(snapshot_);
    broadcastSnapshot();
    tickTimer_.start();
    return true;
}

bool GameServer::startMatchMessage(const net::StartMatchMessage &request, QString *error)
{
    games::GameSimulation::Level level;
    level.name = request.levelName.isEmpty() ? QStringLiteral("网络对局")
                                             : request.levelName;
    level.startWave = std::max(1, request.startWave);
    level.enemyLevel = std::max(1, request.enemyLevel);
    level.script.reserve(request.waves.size());
    for (const net::StartMatchWave &waveData : request.waves) {
        games::Wave wave;
        wave.enemies.reserve(waveData.spawns.size());
        for (const net::StartMatchSpawn &spawnData : waveData.spawns) {
            if (spawnData.count <= 0) {
                continue;
            }
            const std::optional<arena::EnemyType> type =
                arena::Enemy::typeForKey(spawnData.dataKey);
            if (!type) {
                continue;
            }
            wave.enemies.push_back({*type, spawnData.count});
        }
        if (!wave.enemies.empty()) {
            level.script.push_back(std::move(wave));
        }
    }
    if (level.script.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("start match has no valid enemy spawns");
        }
        return false;
    }
    return startMatch(level, error);
}

bool GameServer::handleControlFrame(const QByteArray &frame, QString *error)
{
    net::MessageHeader header;
    QByteArray payload;
    QString decodeError;
    if (!net::decodeFrame(frame, &header, &payload, &decodeError)) {
        if (error != nullptr) {
            *error = decodeError;
        }
        return false;
    }
    if (header.version != net::kProtocolVersion) {
        if (error != nullptr) {
            *error = QStringLiteral("protocol version mismatch");
        }
        return false;
    }
    switch (header.type) {
    case net::MessageType::StartMatch: {
        net::StartMatchMessage request;
        if (!net::decodeStartMatch(frame, &request, &decodeError)) {
            if (error != nullptr) {
                *error = decodeError;
            }
            return false;
        }
        if (simulation_ != nullptr
            && simulation_->state() != games::GameSimulation::State::Victory
            && simulation_->state() != games::GameSimulation::State::Defeat) {
            if (error != nullptr) {
                *error = QStringLiteral("match already running");
            }
            return false;
        }
        return startMatchMessage(request, error);
    }
    case net::MessageType::Input: {
        net::InputCommand command;
        if (!net::decodeInputMessage(frame, &command, &decodeError)) {
            if (error != nullptr) {
                *error = decodeError;
            }
            return false;
        }
        applyInput(command);
        return true;
    }
    default:
        if (error != nullptr) {
            *error = QStringLiteral("unsupported stdio control message");
        }
        return false;
    }
}

void GameServer::handleInput(QTcpSocket *socket, const QByteArray &frame)
{
    if (simulation_ == nullptr || socket != controllerSocket_) {
        return;
    }
    net::InputCommand command;
    QString error;
    if (!net::decodeInputMessage(frame, &command, &error)) {
        // 单条坏输入不踢掉整个会话；忽略并保留连接。
        return;
    }
    applyInput(command);
}

void GameServer::applyInput(const net::InputCommand &command)
{
    if (simulation_ == nullptr) {
        return;
    }
    arena::InputState input;
    input.moveAxis = {std::clamp(command.moveX, -1.0f, 1.0f),
                      std::clamp(command.moveY, -1.0f, 1.0f)};
    input.mouseActive = command.mouseActive;
    input.mousePosition = {command.mouseX, command.mouseY};
    input.shooting = command.shooting;
    input.dashRequested = command.dashRequested;
    simulation_->setInput(input);

    if (command.mineRequested) {
        simulation_->placeMine({command.mineX, command.mineY});
    }
    if (command.pauseToggle) {
        simulation_->setPaused(simulation_->state()
                               != games::GameSimulation::State::Paused);
    }
    if (command.upgradeChoice >= 0) {
        simulation_->chooseUpgrade(command.upgradeChoice);
    }
}

void GameServer::onTick()
{
    if (simulation_ == nullptr) {
        return;
    }
    const qint64 elapsed = tickClock_.nsecsElapsed();
    tickClock_.restart();
    accumulator_ += std::clamp(double(elapsed) / 1e9, 0.0, 0.25);

    constexpr double tickSeconds = 1.0 / double(kTickRate);
    int steps = 0;
    while (accumulator_ >= tickSeconds && steps < 5) {
        simulation_->update(float(tickSeconds));
        accumulator_ -= tickSeconds;
        ++steps;
    }
    if (accumulator_ > tickSeconds * 2.0) {
        accumulator_ = 0.0;   // 严重掉帧时不要追帧到卡死
    }
    if (steps == 0) {
        return;
    }

    simulation_->writeSnapshot(snapshot_);
    broadcastSnapshot();

    if (!matchEndSent_
        && (simulation_->state() == games::GameSimulation::State::Victory
            || simulation_->state() == games::GameSimulation::State::Defeat)) {
        matchEndSent_ = true;
        sendMatchEnd(nullptr, QString());
        tickTimer_.stop();
    }
}

void GameServer::sendWelcome(QTcpSocket *socket, bool accepted, const QString &reason)
{
    auto it = sessions_.find(socket);
    if (it == sessions_.end()) {
        return;
    }
    net::WelcomeMessage welcome;
    welcome.sessionId = it->id;
    welcome.tickRate = kTickRate;
    welcome.accepted = accepted;
    welcome.reason = reason;
    const QByteArray frame = net::encodeWelcome(welcome);
    if (!frame.isEmpty()) {
        socket->write(frame);
    }
}

void GameServer::sendMatchEnd(QTcpSocket *socket, const QString &reason)
{
    net::MatchEndMessage message;
    message.reason = reason;
    message.retryable = true;
    if (simulation_ != nullptr) {
        simulation_->writeSnapshot(snapshot_);
        message.victory = simulation_->state() == games::GameSimulation::State::Victory;
        message.score = snapshot_.score;
        message.kills = snapshot_.kills;
        message.currentWave = snapshot_.waves.currentWave;
        message.totalWaves = snapshot_.waves.totalWaves;
        message.runTime = snapshot_.runTime;
    }
    message.runScore = ShopService.score().run();

    const QByteArray frame = net::encodeMatchEnd(message);
    if (frame.isEmpty()) {
        return;
    }
    if (socket != nullptr) {
        socket->write(frame);
        socket->flush();
    } else {
        broadcastToWelcomed(frame);
    }
}

void GameServer::reject(QTcpSocket *socket, const QString &reason)
{
    ++rejectedClients_;
    lastError_ = reason;
    sendWelcome(socket, false, reason);
    socket->flush();
    socket->disconnectFromHost();
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        onDisconnected(socket);
    }
}

void GameServer::broadcastSnapshot()
{
    const QByteArray frame = net::encodeSnapshotMessage(snapshot_);
    if (!frame.isEmpty()) {
        broadcastToWelcomed(frame);
    }
}

void GameServer::broadcastEvent(const net::EventMessage &event)
{
    const QByteArray frame = net::encodeEvent(event);
    if (!frame.isEmpty()) {
        broadcastToWelcomed(frame);
    }
}

void GameServer::broadcastToWelcomed(const QByteArray &frame)
{
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (!it->welcomed || it.key() == nullptr) {
            continue;
        }
        it.key()->write(frame);
    }
}

} // namespace server
