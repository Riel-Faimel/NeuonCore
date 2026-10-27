#include "Menu/Match/Match.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Net/Client.h"
#include "Net/Protocol.h"
#include "Net/Server.h"
#include "Shop/Shop.h"

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
    if (mode_ == Mode::Local) {
        startLocal(level);
    } else {
        startNetwork(level);
    }
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
    } else {
        client_->setAutoReconnect(true);
        const quint16 port = port_ == 0 ? net::kDefaultPort : port_;
        if (!client_->connectToHost(host_, port)) {
            error_ = client_->lastError();
            return;
        }
    }
    client_->startMatch(level);
}

void Match::stop()
{
    client_.reset();
    server_.reset();
    games_.reset();
    error_.clear();
}

bool Match::failed() const
{
    if (!error_.isEmpty()) {
        return true;
    }
    return client_ != nullptr && client_->failed();
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
        client_->render();
    }
}

const QImage &Match::frame() const
{
    if (games_) {
        return games_->frame();
    }
    if (client_) {
        return client_->frame();
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
