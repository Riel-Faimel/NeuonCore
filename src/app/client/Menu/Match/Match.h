#pragma once

#include "Debug.h"

#include "Games/Games.h"

#include <QImage>
#include <QString>
#include <QtGlobal>

#include <memory>

DebugInfo(class SelfTest;)

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
        // 局域网主机：启动内嵌 GameServer 并进入房间，等房主按 ENTER 才 startMatch。
        // 与 Network 的“单机自动端口 + 立即开局”和“加入远端”区分开。
        Host,
    };

    Match();
    ~Match();

    Match(const Match &) = delete;
    Match &operator=(const Match &) = delete;

    // host 为空表示单机内嵌服务端；否则连接 host:port 上的远端服务端。
    void configure(Mode mode, const QString &host = QString(), quint16 port = 0);

    void start(const Games::Level &level);
    void stop();
    // 用上一次 start() 的关卡和配置重新开始（失败页的重试）。
    bool restart();
    // 房间/内嵌服务端已存在时，直接在原地开下一局；不重建服务器和连接。
    bool restartInPlace();
    // 房间宿主在结果页返回房间：服务端停止本局并广播进入房间，不重开对局。
    bool returnToRoom();
    bool active() const { return games_ != nullptr || client_ != nullptr; }
    bool lobby() const;
    bool failed() const;
    QString error() const { return error_; }

    // ----- 房间 / 大厅（服务端权威） -----
    bool inRoom() const;
    bool roomHost() const { return mode_ == Mode::Host; }
    bool roomConnected() const;
    bool roomPlaying() const;
    bool roomYouAreHost() const;
    QString roomName() const;
    QString roomEndpoint() const;
    int roomPlayers() const;
    int roomMaxPlayers() const;

    // 房主动作：直接调用内嵌 GameServer::startMatch()，绝不走 TCP StartMatch。
    bool startRoomMatch();
    // 房主解散：通知客户端并关闭内嵌服务端；之后 Match 不再 active。
    void dissolveRoom();
    // 加入者离开；只断开本地客户端，不影响主机。
    void leaveRoom() { stop(); }

    // 失败页信息（区分无法连接 / 房间已解散）。
    QString failureTitle() const;
    QString failureDetail() const;
    QString failureEndpoint() const;

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
    const client::GameClient *gameClient() const { return client_.get(); }
    server::GameServer *localServer() { return server_.get(); }

private:
    DebugInfo(friend class ::SelfTest;)

    void startLocal(const Games::Level &level);
    void startNetwork(const Games::Level &level);
    void startHostRoom(const Games::Level &level);
    void claimAdminIfNeeded();
    void renderLobby();

    Mode mode_ = Mode::Network;
    QString host_;
    quint16 port_ = 0;
    QString error_;
    Games::Level level_;
    bool hasLevel_ = false;
    bool adminClaimed_ = false;

    std::unique_ptr<Games> games_;
    std::unique_ptr<client::GameClient> client_;
    std::unique_ptr<server::GameServer> server_;
    QImage fallback_;
};

} // namespace menu
