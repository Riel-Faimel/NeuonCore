#include "Net/Client.h"

#include "Base/Base.h"

#include <QAbstractSocket>

#include <utility>

namespace client {

namespace {

constexpr int kReconnectDelayMs = 700;
constexpr const char *kClientVersion = "NeonCore/3";

} // namespace

GameClient::GameClient(QObject *parent) : QObject(parent)
{
    renderer_.initialize();
    BaseService.setMouseActive(true);

    reconnectTimer_.setSingleShot(true);
    connectTimer_.setSingleShot(true);
    connect(&connectTimer_, &QTimer::timeout, this, [this] {
        if (!welcomed_ && !failed_ && !closedByUser_) {
            fail(QStringLiteral("连接服务器超时，房间不存在"));
        }
    });
    connect(&socket_, &QTcpSocket::connected, this, [this] { onConnected(); });
    connect(&socket_, &QTcpSocket::readyRead, this, [this] { onReadyRead(); });
    connect(&socket_, &QTcpSocket::disconnected, this, [this] { onDisconnected(); });
    connect(&socket_, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) { onSocketError(); });
    connect(&reconnectTimer_, &QTimer::timeout, this, [this] {
        onReconnectTimeout();
    });
}

GameClient::~GameClient()
{
    close();
}

bool GameClient::connectToHost(const QString &host, quint16 port)
{
    if (host.isEmpty() || port == 0) {
        fail(QStringLiteral("invalid server endpoint"));
        return false;
    }
    reconnectTimer_.stop();
    connectTimer_.start(5000);
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.abort();
    }
    frames_.clear();
    host_ = host;
    port_ = port;
    lastError_.clear();
    sessionId_ = 0;
    playerId_ = 0;
    snapshotCount_ = 0;
    reconnectAttempts_ = 0;
    welcomed_ = false;
    failed_ = false;
    matchEnded_ = false;
    closedByUser_ = false;
    rooms_.clear();
    inLobby_ = false;
    hadSession_ = false;
    hadRoom_ = false;
    pendingDash_ = false;
    pendingPause_ = false;
    pendingMine_ = false;
    upgradeChoice_ = -1;
    socket_.connectToHost(host_, port_);
    return true;
}

void GameClient::close()
{
    closedByUser_ = true;
    reconnectTimer_.stop();
    connectTimer_.stop();
    welcomed_ = false;
    frames_.clear();
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.abort();
    }
}

void GameClient::setAutoReconnect(bool enabled)
{
    autoReconnect_ = enabled;
    if (!enabled) {
        reconnectTimer_.stop();
    }
}

void GameClient::update(float dt)
{
    if (view_.state() != net::RunState::Paused) {
        renderer_.update(dt);
    }
    if (!welcomed_ || socket_.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const arena::InputState input = localInput();
    net::InputCommand command;
    command.moveX = input.moveAxis.x;
    command.moveY = input.moveAxis.y;
    command.mouseActive = input.mouseActive;
    command.mouseX = input.mousePosition.x;
    command.mouseY = input.mousePosition.y;
    command.shooting = input.shooting;
    command.dashRequested = pendingDash_;
    command.pauseToggle = pendingPause_;
    command.mineRequested = pendingMine_;
    command.mineX = minePosition_.x;
    command.mineY = minePosition_.y;
    command.upgradeChoice = upgradeChoice_;
    pendingDash_ = false;
    pendingPause_ = false;
    pendingMine_ = false;
    upgradeChoice_ = -1;
    sendInput(command);
}

void GameClient::render()
{
    renderer_.render(view_);
}

void GameClient::keyPress(int key, bool autoRepeat)
{
    if (autoRepeat) {
        return;
    }
    if (view_.state() == net::RunState::Upgrade) {
        if (key >= Qt::Key_1 && key <= Qt::Key_3) {
            upgradeChoice_ = key - Qt::Key_1;
        }
        return;
    }
    if (key == Qt::Key_Escape || key == Qt::Key_P) {
        pendingPause_ = true;
    } else if (key == Qt::Key_Space) {
        pendingDash_ = true;
    }
}

void GameClient::mousePressLeft(const Math::Vec2 &position)
{
    if (view_.state() != net::RunState::Upgrade) {
        return;
    }
    const int index = renderer_.upgradeIndexAt(position);
    if (index >= 0) {
        upgradeChoice_ = index;
    }
}

void GameClient::requestMine(const Math::Vec2 &position)
{
    pendingMine_ = true;
    minePosition_ = position;
}

bool GameClient::sendInput(const net::InputCommand &command)
{
    if (socket_.state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    const QByteArray frame = net::encodeInputMessage(command);
    if (frame.isEmpty()) {
        return false;
    }
    return socket_.write(frame) != -1;
}

void GameClient::setFatalError(const QString &reason)
{
    fail(reason);
}

void GameClient::onConnected()
{
    lastError_.clear();
    sendHello();
}

void GameClient::onReadyRead()
{
    frames_.append(socket_.readAll());
    for (;;) {
        QByteArray frame;
        QString error;
        const net::FrameBuffer::Result result = frames_.next(&frame, &error);
        if (result == net::FrameBuffer::Result::NeedMore) {
            return;
        }
        if (result == net::FrameBuffer::Result::Error) {
            fail(error);
            socket_.abort();
            return;
        }
        handleFrame(frame);
        if (failed_ || closedByUser_
            || socket_.state() == QAbstractSocket::UnconnectedState) {
            return;
        }
    }
}

void GameClient::onDisconnected()
{
    const bool wasWaitingInRoom = inLobby_ || (hadRoom_ && snapshotCount_ == 0);
    welcomed_ = false;
    frames_.clear();
    if (closedByUser_ || failed_ || matchEnded_) {
        return;
    }
    // 房间阶段掉线不需要重连：主机已经解散/关闭，直接进入失败页。
    // 对局阶段的掉线仍允许客户端按配置重连。
    if (wasWaitingInRoom) {
        inLobby_ = false;
        fail(QStringLiteral("房间已解散"));
        return;
    }
    if (autoReconnect_) {
        scheduleReconnect();
    } else {
        fail(QStringLiteral("connection closed"));
    }
}

void GameClient::onSocketError()
{
    if (closedByUser_ || failed_) {
        return;
    }
    lastError_ = socket_.errorString();
    if (autoReconnect_ && !matchEnded_) {
        scheduleReconnect();
    } else {
        fail(lastError_);
    }
}

void GameClient::onReconnectTimeout()
{
    if (closedByUser_ || failed_ || matchEnded_) {
        return;
    }
    frames_.clear();
    welcomed_ = false;
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.abort();
    }
    connectTimer_.start(5000);
    socket_.connectToHost(host_, port_);
}

void GameClient::handleFrame(const QByteArray &frame)
{
    net::MessageHeader header;
    QByteArray payload;
    QString error;
    if (!net::decodeFrame(frame, &header, &payload, &error)) {
        fail(error);
        socket_.abort();
        return;
    }
    if (header.version != net::kProtocolVersion) {
        fail(QStringLiteral("protocol version mismatch: server %1, client %2")
                 .arg(header.version)
                 .arg(net::kProtocolVersion));
        socket_.abort();
        return;
    }

    switch (header.type) {
    case net::MessageType::Welcome:
        handleWelcome(payload);
        break;
    case net::MessageType::Snapshot:
        handleSnapshot(payload);
        break;
    case net::MessageType::Event:
        handleEvent(payload);
        break;
    case net::MessageType::MatchEnd:
        handleMatchEnd(payload);
        break;
    case net::MessageType::RoomList:
        handleRoomList(payload);
        break;
    case net::MessageType::Hello:
    case net::MessageType::StartMatch:
    case net::MessageType::Input:
        fail(QStringLiteral("client-only message type received from server"));
        socket_.abort();
        break;
    default:
        fail(QStringLiteral("unknown server message type %1").arg(int(header.type)));
        socket_.abort();
        break;
    }
}

void GameClient::handleWelcome(const QByteArray &payload)
{
    net::WelcomeMessage welcome;
    QString error;
    if (!net::deserializeWelcome(payload, &welcome, &error)) {
        fail(error);
        socket_.abort();
        return;
    }
    if (!welcome.accepted) {
        fail(welcome.reason.isEmpty() ? QStringLiteral("server rejected connection")
                                      : welcome.reason);
        socket_.disconnectFromHost();
        return;
    }
    sessionId_ = welcome.sessionId;
    playerId_ = welcome.playerId;
    welcomed_ = true;
    connectTimer_.stop();
    hadSession_ = true;
    reconnectAttempts_ = 0;
    lastError_.clear();
}

void GameClient::handleSnapshot(const QByteArray &payload)
{
    net::Snapshot snapshot;
    QString error;
    if (!net::deserializeSnapshot(payload, &snapshot, &error)) {
        fail(error);
        socket_.abort();
        return;
    }
    lastSnapshot_ = std::move(snapshot);
    view_.apply(lastSnapshot_);
    ++snapshotCount_;
    matchEnded_ = false;
    matchEnd_.reason.clear();
    inLobby_ = false;
}

void GameClient::handleRoomList(const QByteArray &payload)
{
    net::RoomListMessage message;
    QString error;
    if (!net::deserializeRoomList(payload, &message, &error)) {
        return;
    }
    rooms_ = std::move(message.rooms);
    bool lobby = false;
    for (const net::RoomInfo &room : rooms_) {
        if (!room.playing) {
            lobby = true;
            break;
        }
    }
    inLobby_ = lobby && snapshotCount_ == 0;
    if (inLobby_) {
        hadRoom_ = true;
    }
}

void GameClient::handleEvent(const QByteArray &payload)
{
    net::EventMessage event;
    QString error;
    if (!net::deserializeEvent(payload, &event, &error)) {
        return;   // 表现事件坏了不终止对局
    }
    applyEvent(event);
}

void GameClient::handleMatchEnd(const QByteArray &payload)
{
    net::MatchEndMessage message;
    QString error;
    if (!net::deserializeMatchEnd(payload, &message, &error)) {
        fail(error);
        return;
    }
    matchEnd_ = message;
    matchEnded_ = true;
    if (!message.reason.isEmpty()) {
        fail(message.reason);
    }
}

void GameClient::sendHello()
{
    net::HelloMessage hello;
    hello.clientVersion = QString::fromLatin1(kClientVersion);
    const QByteArray frame = net::encodeHello(hello);
    if (frame.isEmpty() || socket_.write(frame) == -1) {
        fail(QStringLiteral("cannot send hello"));
    }
}

arena::InputState GameClient::localInput() const
{
    arena::InputState input;
    input.moveAxis = BaseService.movementAxis();
    input.mouseActive = BaseService.mouseActive();
    input.mousePosition = BaseService.mousePosition();
    input.shooting = BaseService.shootingHeld();
    return input;
}

void GameClient::applyEvent(const net::EventMessage &event)
{
    switch (event.kind) {
    case net::EventKind::WaveStarted: {
        const QString title = event.finalWave ? QStringLiteral("最终波次")
                                              : QStringLiteral("第 %1 波").arg(event.wave);
        if (event.boss) {
            renderer_.showBanner(title, QStringLiteral("警告：检测到「虚空吞噬者」"),
                                 2.8f, Base::Magenta,
                                 event.finalWave ? QStringLiteral("FINAL WAVE")
                                                 : QStringLiteral("BOSS SIGNAL"));
            BaseService.playSound(QStringLiteral("boss"));
        } else {
            renderer_.showBanner(title, QStringLiteral("敌方信号正在接近"), 2.15f,
                                 Base::Cyan);
        }
        break;
    }
    case net::EventKind::UpgradeOpened:
        renderer_.flash(Base::Cyan, 28);
        break;
    case net::EventKind::UpgradeSelected:
        renderer_.flash(event.color, 35);
        break;
    case net::EventKind::BossDown:
        renderer_.shake(12.0f);
        renderer_.flash(Base::White, 90);
        renderer_.showBanner(QStringLiteral("目标已摧毁"),
                             QStringLiteral("虚空吞噬者停止响应"), 1.8f,
                             Base::Cyan, QStringLiteral("BOSS DOWN"));
        break;
    case net::EventKind::WaveCleared:
        if (!event.finalWave) {
            renderer_.showBanner(QStringLiteral("波次清理完毕"),
                                 QStringLiteral("正在同步强化协议"), 1.4f,
                                 Base::Green, QStringLiteral("SECTOR CLEAR"));
        }
        break;
    case net::EventKind::Victory:
        renderer_.shake(18.0f);
        renderer_.flash(Base::White, 130);
        renderer_.showBanner(QStringLiteral("区域已肃清"),
                             QStringLiteral("台本中的敌方信号全部消失"), 1.6f,
                             Base::Cyan, QStringLiteral("SECTOR CLEAR"));
        BaseService.playSound(QStringLiteral("victory"));
        break;
    case net::EventKind::VictoryFinished:
        renderer_.burstConfetti();
        break;
    case net::EventKind::Defeat:
        renderer_.shake(20.0f);
        renderer_.flash(Base::Red, 120);
        BaseService.playSound(QStringLiteral("defeat"));
        break;
    case net::EventKind::Feedback:
        if (event.alpha > 0) {
            renderer_.flash(event.color, event.alpha);
        }
        if (event.shake > 0.0f) {
            renderer_.shake(event.shake);
        }
        break;
    }
}

void GameClient::scheduleReconnect()
{
    if (!autoReconnect_ || closedByUser_ || failed_ || matchEnded_) {
        return;
    }
    if (reconnectTimer_.isActive()) {
        return;
    }
    if (reconnectAttempts_ >= kMaxReconnectAttempts) {
        fail(lastError_.isEmpty() ? QStringLiteral("reconnect failed") : lastError_);
        return;
    }
    ++reconnectAttempts_;
    reconnectTimer_.start(kReconnectDelayMs + (reconnectAttempts_ - 1) * 250);
}

void GameClient::fail(const QString &reason)
{
    failed_ = true;
    if (!reason.isEmpty()) {
        lastError_ = reason;
    }
    reconnectTimer_.stop();
    connectTimer_.stop();
}

} // namespace client
