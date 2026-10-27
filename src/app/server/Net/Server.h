#pragma once

#include "Net/Messages.h"
#include "Net/Protocol.h"
#include "Net/Snapshot.h"
#include "Games/Simulation/GameSimulation.h"
#include "Room/Room.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTimer>

#include <memory>
#include <vector>

class QTcpSocket;

namespace games {
class GameSimulation;
}

namespace server {

// GameServer：权威服务端（阶段 3）。
//
// 拥有一个 headless games::GameSimulation（没有 Renderer / Menu / BaseService 输入），
// 通过 QTcpServer 接受客户端；固定 60Hz tick 推进模拟，并向所有已握手客户端广播
// Snapshot / Event / MatchEnd。客户端只发送 Hello / StartMatch / Input，服务端从不
// 接受客户端的世界状态。
//
// 单机模式由 menu::Match 在 127.0.0.1 上启动一个 port 0 的实例；联机模式由
// `NeonCore.exe --server --port 7777` 启动同一个类。两者走完全相同的协议。
class GameServer : public QObject {
public:
    explicit GameServer(QObject *parent = nullptr);
    ~GameServer() override;

    GameServer(const GameServer &) = delete;
    GameServer &operator=(const GameServer &) = delete;

    // loopbackOnly=true 只绑定 127.0.0.1（单机内嵌服务端/测试）；否则绑定所有网卡。
    // port=0 由系统自动分配；失败时返回 false 并把原因写进 error。
    bool listen(quint16 port = net::kDefaultPort, bool loopbackOnly = false,
                QString *error = nullptr);
    void close();
    bool isListening() const { return tcpServer_.isListening(); }
    quint16 port() const { return tcpServer_.serverPort(); }
    QString errorString() const { return lastError_; }

    int clientCount() const { return sessions_.size(); }
    int rejectedClients() const { return rejectedClients_; }
    int tickRate() const { return kTickRate; }
    bool running() const { return simulation_ != nullptr; }
    const net::Snapshot &snapshot() const { return snapshot_; }
    // 服务端分配给某个会话的稳定 playerId/slot（测试/宿主调试用）。
    int playerIdForSession(quint32 sessionId) const;
    int matchPlayerCount() const { return int(matchPlayers_.size()); }

    // ----- 房间生命周期（控制面：只允许本地宿主调用） -----
    // listen() 会先建立一个默认房间；宿主可以用 openRoom() 换成自己的房间名。
    // 房间的开/关与人数以服务端为准，客户端只收到 RoomList。
    bool openRoom(const QString &name, QString *error = nullptr);
    void dissolveRoom(const QString &reason = QStringLiteral("房间已解散"));
    bool roomOpen() const { return rooms_.isOpen(); }
    QString roomName() const;
    int roomPlayers() const;
    int roomMaxPlayers() const;
    quint32 roomHostSessionId() const;

    // 控制面：只允许本地宿主（内嵌 Match / 独立服务端 stdio）调用。
    // TCP 客户端只能收快照，不能 StartMatch、暂停、选强化或控制服务器。
    bool startMatch(const games::GameSimulation::Level &level, QString *error = nullptr);
    bool handleControlFrame(const QByteArray &frame, QString *error = nullptr);

private:
    static constexpr int kTickRate = 60;

    struct Session {
        QTcpSocket *socket = nullptr;
        net::FrameBuffer frames;
        bool hello = false;
        bool welcomed = false;
        // 稳定玩家 id/slot（1..4）；0 表示没有槽位（房间满时不会 welcomed）。
        int playerId = 0;
        // 是否拿到这一局的飞机；中途加入的会话是观战者。
        bool inMatch = false;
        quint32 id = 0;
        quint32 roomId = 0;
    };

    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    void onDisconnected(QTcpSocket *socket);
    void onTick();

    void handleFrame(QTcpSocket *socket, const QByteArray &frame);
    void handleHello(QTcpSocket *socket, const QByteArray &frame);
    void handleInput(QTcpSocket *socket, const QByteArray &frame);
    void handleJoinRoom(QTcpSocket *socket, const QByteArray &frame);
    void applyInput(int playerId, const net::InputCommand &command);
    int allocatePlayerId() const;
    std::vector<int> rosterForMatch() const;
    bool startMatchMessage(const net::StartMatchMessage &request, QString *error);
    void startMatchSocket(QTcpSocket *socket, const net::StartMatchMessage &request);
    void sendWelcome(QTcpSocket *socket, bool accepted, const QString &reason);
    void sendMatchEnd(QTcpSocket *socket, const QString &reason);
    void sendRoomClosed(const QString &reason);
    void reject(QTcpSocket *socket, const QString &reason);
    void broadcastSnapshot();
    void sendRoomList(QTcpSocket *socket);
    void broadcastRoomList();
    int welcomedCount() const;
    void broadcastEvent(const net::EventMessage &event);
    void broadcastToWelcomed(const QByteArray &frame);

    QTcpServer tcpServer_;
    QTimer tickTimer_;
    QElapsedTimer tickClock_;
    double accumulator_ = 0.0;
    QHash<QTcpSocket *, Session> sessions_;
    RoomManager rooms_;
    std::unique_ptr<games::GameSimulation> simulation_;
    net::Snapshot snapshot_;
    // 这一局实际拿到飞机的 playerId（开局时从已 welcomed 的会话快照生成）。
    std::vector<int> matchPlayers_;
    QTcpSocket *controllerSocket_ = nullptr;
    quint32 nextSessionId_ = 1;
    int rejectedClients_ = 0;
    bool matchEndSent_ = false;
    // openRoom() 由本地宿主调用：只有这种房间才把第一个会话标成房主。
    // 独立 NeonCoreServer 的 TCP 客户端都只是加入者，控制权在 stdio 宿主。
    bool localHostRoom_ = false;
    QString lastError_;
};

} // namespace server
