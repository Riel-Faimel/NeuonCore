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
    rooms_.reset();
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
    // 服务端一开始就有一个等待中的房间；宿主可以再用 openRoom() 命名。
    rooms_.open(QStringLiteral("NEON CORE 房间"));
    localHostRoom_ = false;
    return true;
}

bool GameServer::openRoom(const QString &name, QString *error)
{
    if (simulation_ != nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("cannot open a room while a match is running");
        }
        return false;
    }
    if (!tcpServer_.isListening()) {
        if (error != nullptr) {
            *error = QStringLiteral("cannot open a room before listen()");
        }
        return false;
    }
    rooms_.open(name.isEmpty() ? QStringLiteral("NEON CORE 房间") : name,
                controllerSocket_ != nullptr ? sessions_.value(controllerSocket_).id : 0);
    localHostRoom_ = true;
    broadcastRoomList();
    return true;
}

void GameServer::dissolveRoom(const QString &reason)
{
    if (!rooms_.isOpen() && sessions_.isEmpty() && !tcpServer_.isListening()) {
        return;
    }
    // 先通知：客户端收到 MatchEnd(reason) 后进入“房间已解散”失败页；
    // 即使消息在断开时丢失，客户端也会用断线兜底渲染同一失败页。
    sendRoomClosed(reason);
    tickTimer_.stop();
    simulation_.reset();
    snapshot_.clear();
    matchPlayers_.clear();
    matchEndSent_ = true;
    accumulator_ = 0.0;
    rooms_.reset();
    tcpServer_.close();
    controllerSocket_ = nullptr;
    localHostRoom_ = false;
    const QList<QTcpSocket *> sockets = sessions_.keys();
    for (QTcpSocket *socket : sockets) {
        if (socket != nullptr) {
            socket->disconnectFromHost();
        }
    }
}

QString GameServer::roomName() const
{
    return rooms_.isOpen() ? rooms_.defaultRoom().name : QString();
}

int GameServer::roomPlayers() const
{
    return welcomedCount();
}

int GameServer::roomMaxPlayers() const
{
    return rooms_.isOpen() ? rooms_.defaultRoom().maxPlayers : 0;
}

quint32 GameServer::roomHostSessionId() const
{
    if (!rooms_.isOpen()) {
        return 0;
    }
    const Room &room = rooms_.defaultRoom();
    if (room.hostSessionId != 0) {
        return room.hostSessionId;
    }
    if (!localHostRoom_ || controllerSocket_ == nullptr) {
        return 0;
    }
    return sessions_.value(controllerSocket_).id;
}

int GameServer::welcomedCount() const
{
    int count = 0;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->welcomed) {
            ++count;
        }
    }
    return count;
}

int GameServer::playerIdForSession(quint32 sessionId) const
{
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->id == sessionId) {
            return it->playerId;
        }
    }
    return 0;
}

int GameServer::allocatePlayerId() const
{
    bool used[net::kMaxPlayersPerRoom + 1] = {};
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (!it->welcomed) {
            continue;
        }
        if (it->playerId >= 1 && it->playerId <= net::kMaxPlayersPerRoom) {
            used[it->playerId] = true;
        }
    }
    for (int id = 1; id <= net::kMaxPlayersPerRoom; ++id) {
        if (!used[id]) {
            return id;
        }
    }
    return 0;
}

std::vector<int> GameServer::rosterForMatch() const
{
    std::vector<int> ids;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->welcomed && it->playerId >= 1
            && it->playerId <= net::kMaxPlayersPerRoom) {
            ids.push_back(it->playerId);
        }
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    if (ids.size() > size_t(net::kMaxPlayersPerRoom)) {
        ids.resize(size_t(net::kMaxPlayersPerRoom));
    }
    if (ids.empty()) {
        // 独立 NeonCoreServer 还没有 TCP 客户端时，stdio 宿主仍然能开一局单机模拟。
        ids.push_back(1);
    }
    return ids;
}

void GameServer::close()
{
    tickTimer_.stop();
    localHostRoom_ = false;
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
    rooms_.reset();
    simulation_.reset();
    snapshot_.clear();
    matchPlayers_.clear();
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
    auto it = sessions_.find(socket);
    if (it == sessions_.end()) {
        socket->deleteLater();
        return;
    }
    const int playerId = it->playerId;
    const quint32 sessionId = it->id;
    const bool wasInMatch = it->inMatch;
    if (controllerSocket_ == socket) {
        controllerSocket_ = nullptr;
    }
    sessions_.remove(socket);
    socket->deleteLater();

    // 房主离开房间：把房主交给下一个 welcomed 会话，房间继续。
    if (rooms_.isOpen() && rooms_.defaultRoom().hostSessionId == sessionId) {
        quint32 nextHost = 0;
        for (auto other = sessions_.begin(); other != sessions_.end(); ++other) {
            if (other->welcomed) {
                nextHost = other->id;
                break;
            }
        }
        rooms_.setHostSession(nextHost);
    }

    // 对局中的玩家离开：移除对应飞机（不是整局结束），其他玩家继续。
    if (wasInMatch && playerId > 0) {
        std::erase(matchPlayers_, playerId);
        if (simulation_ != nullptr) {
            simulation_->removePlayer(playerId);
        }
    }
    broadcastRoomList();
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
    case net::MessageType::JoinRoom:
        handleJoinRoom(socket, frame);
        break;
    case net::MessageType::Welcome:
    case net::MessageType::Snapshot:
    case net::MessageType::Event:
    case net::MessageType::MatchEnd:
    case net::MessageType::RoomList:
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
    if (it->welcomed) {
        // 重复 Hello 不重新分配槽位，保持 playerId 稳定。
        sendWelcome(socket, true, QString());
        return;
    }
    if (welcomedCount() >= net::kMaxPlayersPerRoom) {
        reject(socket, QStringLiteral("房间已满"));
        return;
    }
    it->hello = true;
    it->welcomed = true;
    it->playerId = allocatePlayerId();
    it->inMatch = false;

    // 单机内嵌路径会在 TCP 握手完成前就 startMatch()（roster 回退成 {1}）。
    // 如果这一局里存在这个 slot 的飞机、而且还没有别的会话控制它，就让刚
    // welcomed 的会话接管它——否则单机会变成“有飞机但不能操作”的观战者。
    if (simulation_ != nullptr && simulation_->hasPlayer(it->playerId)
        && std::find(matchPlayers_.begin(), matchPlayers_.end(), it->playerId)
               != matchPlayers_.end()) {
        bool taken = false;
        for (auto other = sessions_.begin(); other != sessions_.end(); ++other) {
            if (other.key() != socket && other->welcomed && other->inMatch
                && other->playerId == it->playerId) {
                taken = true;
                break;
            }
        }
        if (!taken) {
            it->inMatch = true;
        }
    }

    if (controllerSocket_ == nullptr) {
        // 第一个连接到本地内嵌服务端的客户端是主控玩家；后续客户端各有自己的飞机。
        controllerSocket_ = socket;
    }
    if (localHostRoom_ && rooms_.isOpen()
        && rooms_.defaultRoom().hostSessionId == 0) {
        // 内嵌宿主自己的回环客户端是房主；独立服务端的客户端仍是加入者。
        rooms_.setHostSession(it->id);
    }
    sendWelcome(socket, true, QString());
    broadcastRoomList();
}

void GameServer::handleJoinRoom(QTcpSocket *socket, const QByteArray &frame)
{
    net::JoinRoomMessage request;
    QString error;
    if (!net::decodeJoinRoom(frame, &request, &error)) {
        reject(socket, error);
        return;
    }
    auto it = sessions_.find(socket);
    if (it == sessions_.end()) {
        return;
    }
    bool found = false;
    for (const Room &room : rooms_.rooms()) {
        if (room.id == request.roomId) {
            found = true;
            break;
        }
    }
    if (!found) {
        reject(socket, QStringLiteral("room not found"));
        return;
    }
    it->roomId = request.roomId;
    sendRoomList(socket);
    broadcastRoomList();
}


bool GameServer::startMatch(const games::GameSimulation::Level &level, QString *error)
{
    if (level.script.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("start match carries no waves");
        }
        return false;
    }

    if (!rooms_.isOpen()) {
        // 兼容直接调用 startMatch() 的宿主 / 测试：没有显式房间时补一个。
        rooms_.open(QStringLiteral("NEON CORE 房间"),
                    controllerSocket_ != nullptr ? sessions_.value(controllerSocket_).id : 0);
    }

    // 这一局拿到飞机的会话：开局时已 welcomed 的会话，按 playerId 升序。
    // 开局后才加入的会话 playerId 仍然稳定，但这一局只是观战者。
    const std::vector<int> roster = rosterForMatch();
    matchPlayers_ = roster;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        it->inMatch = it->welcomed && it->playerId > 0
                      && std::find(roster.begin(), roster.end(), it->playerId)
                             != roster.end();
    }

    auto simulation = std::make_unique<ServerSimulation>(level);
    simulation->setPlayers(roster);
    simulation->onEvent = [this](const net::EventMessage &event) {
        broadcastEvent(event);
    };
    simulation_ = std::move(simulation);
    matchEndSent_ = false;
    rooms_.setPlaying(true);
    broadcastRoomList();

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
        const int target = command.playerId > 0
                               ? command.playerId
                               : (simulation_ != nullptr
                                      ? simulation_->leaderPlayerId()
                                      : 0);
        applyInput(target, command);
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
    if (simulation_ == nullptr) {
        return;
    }
    auto it = sessions_.find(socket);
    if (it == sessions_.end() || !it->welcomed || it->playerId <= 0) {
        return;   // 未知/无槽位会话
    }
    if (!it->inMatch || !simulation_->hasPlayer(it->playerId)) {
        return;   // 观战者/中途加入：没有自己的飞机，输入忽略
    }
    net::InputCommand command;
    QString error;
    if (!net::decodeInputMessage(frame, &command, &error)) {
        // 单条坏输入不踢掉整个会话；忽略并保留连接。
        return;
    }
    // TCP 客户端不能选择飞机：永远只控制服务端分配给这个会话的 playerId。
    applyInput(it->playerId, command);
}

void GameServer::applyInput(int playerId, const net::InputCommand &command)
{
    if (simulation_ == nullptr || playerId <= 0
        || !simulation_->hasPlayer(playerId)) {
        return;
    }
    arena::InputState input;
    input.moveAxis = {std::clamp(command.moveX, -1.0f, 1.0f),
                      std::clamp(command.moveY, -1.0f, 1.0f)};
    input.mouseActive = command.mouseActive;
    input.mousePosition = {command.mouseX, command.mouseY};
    input.shooting = command.shooting;
    input.dashRequested = command.dashRequested;
    simulation_->setInput(playerId, input);

    if (command.mineRequested) {
        simulation_->placeMine({command.mineX, command.mineY});
    }
    // 暂停/强化选择是整局共享动作：只接受对局 leader（最小 playerId），
    // 避免观战或第二个玩家打断所有人的对局。
    const bool leader = playerId == simulation_->leaderPlayerId();
    if (command.pauseToggle && leader) {
        simulation_->setPaused(simulation_->state()
                               != games::GameSimulation::State::Paused);
    }
    if (command.upgradeChoice >= 0 && leader) {
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

void GameServer::sendRoomList(QTcpSocket *socket)
{
    auto it = sessions_.find(socket);
    if (it == sessions_.end() || !it->welcomed) {
        return;
    }
    if (!rooms_.isOpen()) {
        // 房间已解散：服务端权威地发送空列表，客户端不会进入假对局。
        net::RoomListMessage empty;
        const QByteArray frame = net::encodeRoomList(empty);
        if (!frame.isEmpty()) {
            socket->write(frame);
        }
        return;
    }
    rooms_.setPlayers(welcomedCount());
    net::RoomListMessage message;
    message.rooms.reserve(rooms_.rooms().size());
    const quint32 hostSessionId = roomHostSessionId();
    for (const Room &room : rooms_.rooms()) {
        net::RoomInfo info;
        info.id = room.id;
        info.name = room.name;
        info.players = room.players;
        info.maxPlayers = room.maxPlayers;
        info.playing = room.playing;
        for (auto member = sessions_.begin(); member != sessions_.end(); ++member) {
            if (!member->welcomed) {
                continue;
            }
            net::RoomPlayerInfo player;
            player.sessionId = member->id;
            player.playerId = member->playerId;
            player.host = member->id == hostSessionId;
            player.inMatch = member->inMatch;
            info.members.push_back(player);
        }
        info.players = int(info.members.size());
        message.rooms.push_back(std::move(info));
    }
    const QByteArray frame = net::encodeRoomList(message);
    if (!frame.isEmpty()) {
        socket->write(frame);
    }
}

void GameServer::broadcastRoomList()
{
    if (!rooms_.isOpen()) {
        return;
    }
    rooms_.setPlayers(welcomedCount());
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->welcomed && it.key() != nullptr) {
            sendRoomList(it.key());
        }
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
    welcome.playerId = it->playerId;
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
    rooms_.setPlaying(false);
    // 对局结束后会话回到房间/观战状态；下一局重新分配飞机。
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        it->inMatch = false;
    }
    matchPlayers_.clear();
    broadcastRoomList();
}

void GameServer::sendRoomClosed(const QString &reason)
{
    net::MatchEndMessage message;
    message.reason = reason;
    message.retryable = true;
    const QByteArray frame = net::encodeMatchEnd(message);
    if (frame.isEmpty()) {
        return;
    }
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        QTcpSocket *socket = it.key();
        if (socket == nullptr || !it->welcomed) {
            continue;
        }
        socket->write(frame);
        socket->flush();
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
    if (snapshot_.players.empty() && simulation_ == nullptr) {
        return;
    }
    // 快照内容每帧只提取一次；localPlayerId 是每个客户端自己的视角，
    // 编码前按会话覆盖，编完恢复，避免为每个会话复制整份世界。
    const int baseLocalPlayerId = snapshot_.localPlayerId;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        QTcpSocket *socket = it.key();
        if (socket == nullptr || !it->welcomed) {
            continue;
        }
        snapshot_.localPlayerId = (it->inMatch && it->playerId > 0) ? it->playerId : 0;
        const QByteArray frame = net::encodeSnapshotMessage(snapshot_);
        if (!frame.isEmpty()) {
            socket->write(frame);
        }
    }
    snapshot_.localPlayerId = baseLocalPlayerId;
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
