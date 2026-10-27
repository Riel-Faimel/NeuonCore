#include "Menu/Match/Match.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Net/Client.h"
#include "Net/Protocol.h"
#include "Net/Server.h"
#include "Shop/Shop.h"

#include <QPainter>
#include <QLinearGradient>
#include <Qt>

#include <utility>

namespace menu {

namespace {

Games::State toGameState(net::RunState state)
{
    switch (state) {
    case net::RunState::Playing:
        return Games::State::Playing;
    case net::RunState::Upgrade:
        return Games::State::Upgrade;
    case net::RunState::Paused:
        return Games::State::Paused;
    case net::RunState::Victory:
        return Games::State::Victory;
    case net::RunState::Defeat:
        return Games::State::Defeat;
    }
    return Games::State::Playing;
}

} // namespace

Match::Match()
    : fallback_(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                QImage::Format_ARGB32_Premultiplied)
{
    fallback_.fill(Qt::black);
}

Match::~Match() = default;

void Match::configure(Mode mode, const QString &host, quint16 port)
{
    mode_ = mode;
    host_ = host;
    port_ = port;
    error_.clear();
}

void Match::start(const Games::Level &level)
{
    stop();
    level_ = level;
    hasLevel_ = true;
    switch (mode_) {
    case Mode::Local:
        startLocal(level);
        break;
    case Mode::Host:
        startHostRoom(level);
        break;
    case Mode::Network:
        startNetwork(level);
        break;
    }
}

bool Match::restart()
{
    if (!hasLevel_) {
        return false;
    }
    const Games::Level level = level_;
    stop();
    start(level);
    return active();
}

void Match::startLocal(const Games::Level &level)
{
    games_ = std::make_unique<Games>(level);
}

void Match::startNetwork(const Games::Level &level)
{
    client_ = std::make_unique<client::GameClient>();
    if (host_.isEmpty()) {
        // 单机：同进程内嵌服务端，自动端口 + 127.0.0.1 回环，仍然走真实 TCP。
        // 单机路径保持“立即开局”；只有 Mode::Host 才进入房间等待房主开始。
        server_ = std::make_unique<server::GameServer>();
        QString error;
        if (!server_->listen(0, true, &error)) {
            error_ = error;
            client_->setFatalError(error);
            return;
        }
        client_->setAutoReconnect(false);
        if (!client_->connectToHost(QStringLiteral("127.0.0.1"), server_->port())) {
            error_ = client_->lastError();
            return;
        }
        // 控制面不走 TCP：内嵌宿主直接开始对局；网络客户端只能收快照。
        QString startError;
        if (!server_->startMatch(level, &startError)) {
            error_ = startError;
            client_->setFatalError(startError);
            return;
        }
    } else {
        // 加入远端主机：只连接并进入房间；开局只能由房主本地 startMatch 触发。
        // 失败由专用失败页提供手动重试，不做后台重连以免停留在空画面。
        client_->setAutoReconnect(false);
        const quint16 port = port_ == 0 ? net::kDefaultPort : port_;
        if (!client_->connectToHost(host_, port)) {
            error_ = client_->lastError();
            return;
        }
    }
}

void Match::startHostRoom(const Games::Level &level)
{
    client_ = std::make_unique<client::GameClient>();
    server_ = std::make_unique<server::GameServer>();
    QString error;
    // 局域网主机：绑定所有网卡上的显式端口，让其他玩家加入。
    if (!server_->listen(port_, false, &error)) {
        error_ = error;
        client_->setFatalError(error);
        return;
    }
    const QString roomName = QStringLiteral("%1 房间 · %2")
                                 .arg(level.name.isEmpty() ? QStringLiteral("自定义")
                                                           : level.name)
                                 .arg(server_->port());
    if (!server_->openRoom(roomName, &error)) {
        error_ = error;
        client_->setFatalError(error);
        return;
    }
    client_->setAutoReconnect(false);
    if (!client_->connectToHost(QStringLiteral("127.0.0.1"), server_->port())) {
        error_ = client_->lastError();
        return;
    }
    // 注意：这里不调用 startMatch()。房主在房间页按 ENTER 时才真正开始。
}

void Match::stop()
{
    client_.reset();
    server_.reset();
    games_.reset();
    error_.clear();
    level_ = Games::Level{};
    hasLevel_ = false;
}

bool Match::failed() const
{
    if (!error_.isEmpty()) {
        return true;
    }
    return client_ != nullptr && client_->failed();
}

bool Match::lobby() const
{
    return inRoom();
}

bool Match::inRoom() const
{
    if (games_ != nullptr || client_ == nullptr || failed()) {
        return false;
    }
    if (client_->snapshotCount() > 0 || client_->matchEnded()) {
        return false;
    }
    if (mode_ == Mode::Host) {
        return true;
    }
    // 加入远端：在收到第一份快照前都留在房间页；单机内嵌服务端立即开局，不经过这里。
    return mode_ == Mode::Network && !host_.isEmpty();
}

bool Match::roomConnected() const
{
    return client_ != nullptr && client_->welcomed();
}

bool Match::roomPlaying() const
{
    if (server_ != nullptr && server_->running()) {
        return true;
    }
    if (client_ != nullptr) {
        for (const net::RoomInfo &room : client_->rooms()) {
            if (room.playing) {
                return true;
            }
        }
    }
    return false;
}

bool Match::roomYouAreHost() const
{
    // 只有启动内嵌 GameServer 的 Match 是房主；加入远端时即使是独立服务端的
    // 第一个 TCP 会话也不是房主（控制权在启动 NeonCoreServer 的 stdio 宿主）。
    return mode_ == Mode::Host && server_ != nullptr;
}

QString Match::roomName() const
{
    if (server_ != nullptr && !server_->roomName().isEmpty()) {
        return server_->roomName();
    }
    if (client_ != nullptr) {
        for (const net::RoomInfo &room : client_->rooms()) {
            if (!room.name.isEmpty()) {
                return room.name;
            }
        }
    }
    return QStringLiteral("等待房间信息…");
}

QString Match::roomEndpoint() const
{
    if (server_ != nullptr) {
        return QStringLiteral("0.0.0.0:%1").arg(server_->port());
    }
    if (!host_.isEmpty()) {
        return QStringLiteral("%1:%2").arg(host_).arg(port_ == 0 ? net::kDefaultPort : port_);
    }
    return QString();
}

int Match::roomPlayers() const
{
    if (server_ != nullptr) {
        return server_->roomPlayers();
    }
    if (client_ != nullptr) {
        for (const net::RoomInfo &room : client_->rooms()) {
            return room.players;
        }
    }
    return 0;
}

int Match::roomMaxPlayers() const
{
    if (server_ != nullptr) {
        return server_->roomMaxPlayers();
    }
    if (client_ != nullptr) {
        for (const net::RoomInfo &room : client_->rooms()) {
            return room.maxPlayers;
        }
    }
    return 4;
}

bool Match::startRoomMatch()
{
    if (!roomHost() || server_ == nullptr) {
        return false;
    }
    if (server_->running()) {
        return true;   // 重复 ENTER 不重开对局
    }
    QString startError;
    if (!server_->startMatch(level_, &startError)) {
        error_ = startError;
        if (client_ != nullptr) {
            client_->setFatalError(startError);
        }
        return false;
    }
    return true;
}

void Match::dissolveRoom()
{
    if (server_ != nullptr) {
        server_->dissolveRoom(QStringLiteral("房间已解散"));
    }
    client_.reset();
    server_.reset();
    games_.reset();
    error_.clear();
}

QString Match::failureTitle() const
{
    if (mode_ == Mode::Host) {
        return QStringLiteral("无法启动服务器");
    }
    if (client_ != nullptr && client_->lastError().contains(QStringLiteral("解散"))) {
        return QStringLiteral("房间已解散");
    }
    // 进过房间的客户端掉线（包括主机在对局中解散）都按“房间已解散”呈现。
    if (client_ != nullptr && client_->hadRoom()) {
        return QStringLiteral("房间已解散");
    }
    if (client_ != nullptr && client_->snapshotCount() > 0) {
        return QStringLiteral("连接已中断");
    }
    if (client_ != nullptr && client_->hadSession()) {
        return QStringLiteral("房间已解散");
    }
    return QStringLiteral("无法连接服务器");
}

QString Match::failureDetail() const
{
    QString detail;
    if (!error_.isEmpty()) {
        detail = error_;
    } else if (client_ != nullptr && !client_->lastError().isEmpty()) {
        detail = client_->lastError();
    }
    if (detail.isEmpty()) {
        return QStringLiteral("服务器没有响应，或房间已经关闭。");
    }
    if (detail == failureTitle()) {
        return QStringLiteral("房主关闭了房间服务器，所有客户端连接已断开。");
    }
    return detail;
}

QString Match::failureEndpoint() const
{
    if (server_ != nullptr) {
        return QStringLiteral("127.0.0.1:%1").arg(server_->port());
    }
    if (!host_.isEmpty()) {
        return QStringLiteral("%1:%2").arg(host_).arg(port_ == 0 ? net::kDefaultPort : port_);
    }
    return QString();
}

void Match::update(float dt)
{
    if (games_) {
        games_->update(dt);
    } else if (client_) {
        client_->update(dt);
    }
}

void Match::render()
{
    if (games_) {
        games_->render();
    } else if (client_) {
        if (client_->inLobby()) {
            renderLobby();
        } else {
            client_->render();
        }
    }
}

void Match::renderLobby()
{
    fallback_.fill(QColor(2, 3, 9));
    QPainter painter(&fallback_);
    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.6, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SERVER LOBBY  //  NO ACTIVE MATCH"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 92}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("房间"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 148},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("主机尚未开始对局，等待服务端控制。"),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 188},
               QStringLiteral("center"), 210);

    float top = 240.0f;
    if (client_ != nullptr) {
        for (const net::RoomInfo &room : client_->rooms()) {
            const QRectF card(300.0f, top, 680.0f, 76.0f);
            Base::panel(painter, card, room.playing ? Base::Green : Base::Cyan,
                        220, 10, QColor(6, 16, 27));
            Base::text(painter, Base::mediumFont(), room.name, Base::White,
                       {float(card.left()) + 20.0f, float(card.center().y()) - 10.0f},
                       QStringLiteral("midleft"), 230);
            Base::text(painter, Base::tinyFont(),
                       QStringLiteral("%1 / %2 人 · %3")
                           .arg(room.players)
                           .arg(room.maxPlayers)
                           .arg(room.playing ? QStringLiteral("对局中")
                                             : QStringLiteral("等待中")),
                       Base::Muted,
                       {float(card.left()) + 20.0f, float(card.center().y()) + 16.0f},
                       QStringLiteral("midleft"), 200);
            top += 90.0f;
        }
    }
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("ESC 返回主菜单 · 对局开始后自动进入战斗"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("center"), 170);
}

const QImage &Match::frame() const
{
    if (games_) {
        return games_->frame();
    }
    if (client_) {
        return client_->inLobby() ? fallback_ : client_->frame();
    }
    return fallback_;
}

Games::State Match::state() const
{
    if (games_) {
        return games_->state();
    }
    if (!client_) {
        return Games::State::Defeat;
    }
    if (failed()) {
        return Games::State::Defeat;
    }
    return toGameState(client_->state());
}

bool Match::finished() const
{
    if (games_) {
        return games_->state() == Games::State::Victory
               || games_->state() == Games::State::Defeat;
    }
    if (!client_) {
        return true;
    }
    if (failed() || client_->matchEnded()) {
        return true;
    }
    return client_->state() == net::RunState::Victory
           || client_->state() == net::RunState::Defeat;
}

int Match::runScore() const
{
    if (games_) {
        return games_->runScore();
    }
    return client_ ? client_->runScore() : 0;
}

int Match::kills() const
{
    if (games_) {
        return games_->kills();
    }
    return client_ ? client_->view().kills() : 0;
}

int Match::wave() const
{
    if (games_) {
        return games_->currentWave();
    }
    return client_ ? client_->view().waves().currentWave : 0;
}

int Match::totalWaves() const
{
    if (games_) {
        return games_->totalWaves();
    }
    return client_ ? client_->view().waves().totalWaves : 0;
}

float Match::runTime() const
{
    if (games_) {
        return games_->runTime();
    }
    return client_ ? client_->view().runTime() : 0.0f;
}

void Match::keyPress(int key, bool autoRepeat)
{
    if (games_) {
        games_->keyPress(key, autoRepeat);
        return;
    }
    if (client_) {
        BaseService.keyPress(key);
        client_->keyPress(key, autoRepeat);
    }
}

void Match::keyRelease(int key)
{
    if (games_) {
        games_->keyRelease(key);
        return;
    }
    if (client_) {
        BaseService.keyRelease(key);
        client_->keyRelease(key);
    }
}

void Match::mouseMove(const Math::Vec2 &position)
{
    if (games_) {
        games_->mouseMove(position);
        return;
    }
    BaseService.mouseMove(position);
}

void Match::mousePressLeft(const Math::Vec2 &position)
{
    if (games_) {
        games_->mousePressLeft(position);
        return;
    }
    if (client_) {
        BaseService.mousePressLeft(position);
        client_->mousePressLeft(position);
    }
}

void Match::mousePressRight(const Math::Vec2 &position)
{
    if (games_) {
        games_->mousePressRight(position);
        return;
    }
    if (!client_ || client_->state() != net::RunState::Playing) {
        return;
    }
    if (position.x < GameConfig::FieldLeft || position.x > GameConfig::FieldRight
        || position.y < GameConfig::FieldTop || position.y > GameConfig::FieldBottom) {
        return;
    }
    // 阶段 3 的库存仍是客户端本地（服务端权威钱包/库存是阶段 4）；
    // 服务端只接收“在何处布雷”的意图并负责生成地雷。
    if (!ShopService.inventory().takeMine()) {
        BaseService.playSound(QStringLiteral("ui"));
        return;
    }
    ShopService.inventory().save();
    client_->requestMine(position);
}

void Match::mouseReleaseLeft()
{
    if (games_) {
        games_->mouseReleaseLeft();
        return;
    }
    BaseService.mouseReleaseLeft();
    if (client_) {
        client_->mouseReleaseLeft();
    }
}

} // namespace menu
