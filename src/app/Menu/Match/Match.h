#pragma once

#include "Games/Games.h"

#include <QImage>
#include <QString>
#include <QtGlobal>

#include <memory>

class SelfTest;

namespace client {
class GameClient;
}

namespace server {
class GameServer;
}

namespace menu {

// Match 组件：持有“当前这一局”，负责开局、推进、渲染和局内输入转发。
// 只活一局：Menu 开局时 start()，回主菜单时 stop()。
//
// 默认走网络路径：GameClient 连接一个服务端（单机时是 127.0.0.1 上自动端口的
// 内嵌 GameServer），服务端权威模拟，客户端只渲染快照。自动化测试/离线运行可
// 切到 Local 模式，继续直接持有 Games（与阶段 2 完全一致）。
class Match {
public:
    enum class Mode {
        Local,
        Network,
    };

    Match();
    ~Match();

    Match(const Match &) = delete;
    Match &operator=(const Match &) = delete;

    // host 为空表示单机内嵌服务端；否则连接 host:port 上的远端服务端。
    void configure(Mode mode, const QString &host = QString(), quint16 port = 0);

    void start(const Games::Level &level);
    void stop();
    bool active() const { return games_ != nullptr || client_ != nullptr; }
    bool failed() const;
    QString error() const { return error_; }

    void update(float dt);
    void render();
    const QImage &frame() const;
    Games::State state() const;
    bool finished() const;
    int runScore() const;
    int kills() const;
    int wave() const;
    int totalWaves() const;
    float runTime() const;

    void keyPress(int key, bool autoRepeat);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mousePressRight(const Math::Vec2 &position);
    void mouseReleaseLeft();

    // 测试 / 调试用；正常流程不需要直接拿 Games / GameClient。
    Games *game() { return games_.get(); }
    client::GameClient *gameClient() { return client_.get(); }
    server::GameServer *localServer() { return server_.get(); }

private:
    friend class ::SelfTest;

    void startLocal(const Games::Level &level);
    void startNetwork(const Games::Level &level);

    Mode mode_ = Mode::Network;
    QString host_;
    quint16 port_ = 0;
    QString error_;

    std::unique_ptr<Games> games_;
    std::unique_ptr<client::GameClient> client_;
    std::unique_ptr<server::GameServer> server_;
    QImage fallback_;
};

} // namespace menu
