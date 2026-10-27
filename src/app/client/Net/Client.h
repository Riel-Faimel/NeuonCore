#pragma once

#include "Client/GameView.h"
#include "Games/Renderer/Renderer.h"
#include "Games/Simulation/GameSimulation.h"
#include "Net/Input.h"
#include "Net/Messages.h"
#include "Net/Protocol.h"
#include "Net/Snapshot.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include <memory>

namespace client {

// GameClient：TCP 客户端（阶段 3）。
//
// 连接、握手（Hello/Welcome）、发送 Input、接收 Snapshot/Event/MatchEnd；
// 用 GameView 重建只读世界，再由现有的 games::Renderer 绘制。它不推进模拟、
// 不读取服务端对象，也不把本地世界状态回传给服务端。
//
// 单机模式下 GameClient 连接同进程 127.0.0.1 上的内嵌 GameServer；联机模式连接
// `--server` 启动的远端端口。两条路径使用完全相同的消息。
class GameClient : public QObject {
public:
    explicit GameClient(QObject *parent = nullptr);
    ~GameClient() override;

    GameClient(const GameClient &) = delete;
    GameClient &operator=(const GameClient &) = delete;

    bool connectToHost(const QString &host, quint16 port);
    void close();
    void setAutoReconnect(bool enabled);

    void update(float dt);
    void render();
    const QImage &frame() const { return renderer_.frame(); }

    // 输入转发：Match 负责把窗口事件同时交给 BaseService（移动/瞄准轮询）。
    void keyPress(int key, bool autoRepeat);
    void keyRelease(int key);
    void mousePressLeft(const Math::Vec2 &position);
    void mouseReleaseLeft();
    void requestMine(const Math::Vec2 &position);
    int upgradeIndexAt(const Math::Vec2 &position) const;

    // 直接发送一条输入命令（测试/调试）；正常路径由 update() 每帧采集 BaseService。
    bool sendInput(const net::InputCommand &command);
    bool joinRoom(quint32 roomId);

    bool welcomed() const { return welcomed_; }
    bool matchActive() const { return snapshotCount_ > 0; }
    bool inLobby() const { return inLobby_; }
    const std::vector<net::RoomInfo> &rooms() const { return rooms_; }
    bool failed() const { return failed_; }
    bool matchEnded() const { return matchEnded_; }
    bool autoReconnect() const { return autoReconnect_; }
    int snapshotCount() const { return snapshotCount_; }
    int reconnectAttempts() const { return reconnectAttempts_; }
    QString lastError() const { return lastError_; }
    QAbstractSocket::SocketState socketState() const { return socket_.state(); }
    quint16 serverTickRate() const { return tickRate_; }
    quint32 sessionId() const { return sessionId_; }

    net::RunState state() const { return view_.state(); }
    const GameView &view() const { return view_; }
    const net::Snapshot &lastSnapshot() const { return lastSnapshot_; }
    const net::MatchEndMessage &matchEnd() const { return matchEnd_; }
    int runScore() const { return matchEnd_.runScore; }

    // 启动内嵌服务端失败等由 menu::Match 注入；保持 GameClient 仍可渲染空画面。
    void setFatalError(const QString &reason);

private:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSocketError();
    void onReconnectTimeout();

    void handleFrame(const QByteArray &frame);
    void handleWelcome(const QByteArray &frame);
    void handleSnapshot(const QByteArray &frame);
    void handleEvent(const QByteArray &frame);
    void handleMatchEnd(const QByteArray &frame);
    void handleRoomList(const QByteArray &frame);

    void sendHello();
    arena::InputState localInput() const;
    void applyEvent(const net::EventMessage &event);
    void scheduleReconnect();
    void fail(const QString &reason);

    static constexpr int kMaxReconnectAttempts = 3;

    QTcpSocket socket_;
    QTimer reconnectTimer_;
    net::FrameBuffer frames_;
    GameView view_;
    games::Renderer renderer_;
    net::Snapshot lastSnapshot_;
    net::MatchEndMessage matchEnd_;
    std::vector<net::RoomInfo> rooms_;

    QString host_;
    quint16 port_ = 0;
    QString lastError_;
    quint32 sessionId_ = 0;
    quint16 tickRate_ = 60;
    int snapshotCount_ = 0;
    int reconnectAttempts_ = 0;

    bool matchRequested_ = false;
    bool welcomed_ = false;
    bool inLobby_ = false;
    bool failed_ = false;
    bool matchEnded_ = false;
    bool autoReconnect_ = false;
    bool closedByUser_ = false;

    bool pendingDash_ = false;
    bool pendingPause_ = false;
    bool pendingMine_ = false;
    int upgradeChoice_ = -1;
    Math::Vec2 minePosition_;
};

} // namespace client
