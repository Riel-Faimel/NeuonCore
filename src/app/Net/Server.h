#pragma once

#include "Net/Messages.h"
#include "Net/Protocol.h"
#include "Net/Snapshot.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTimer>

#include <memory>

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

private:
    static constexpr int kTickRate = 60;

    struct Session {
        QTcpSocket *socket = nullptr;
        net::FrameBuffer frames;
        bool hello = false;
        bool welcomed = false;
        quint32 id = 0;
    };

    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    void onDisconnected(QTcpSocket *socket);
    void onTick();

    void handleFrame(QTcpSocket *socket, const QByteArray &frame);
    void handleHello(QTcpSocket *socket, const QByteArray &frame);
    void handleStartMatch(QTcpSocket *socket, const QByteArray &frame);
    void handleInput(QTcpSocket *socket, const QByteArray &frame);

    void startMatch(QTcpSocket *socket, const net::StartMatchMessage &request);
    void sendWelcome(QTcpSocket *socket, bool accepted, const QString &reason);
    void sendMatchEnd(QTcpSocket *socket, const QString &reason);
    void reject(QTcpSocket *socket, const QString &reason);
    void broadcastSnapshot();
    void broadcastEvent(const net::EventMessage &event);
    void broadcastToWelcomed(const QByteArray &frame);

    QTcpServer tcpServer_;
    QTimer tickTimer_;
    QElapsedTimer tickClock_;
    double accumulator_ = 0.0;
    QHash<QTcpSocket *, Session> sessions_;
    std::unique_ptr<games::GameSimulation> simulation_;
    net::Snapshot snapshot_;
    QTcpSocket *controlSocket_ = nullptr;
    quint32 nextSessionId_ = 1;
    int rejectedClients_ = 0;
    bool matchEndSent_ = false;
    QString lastError_;
};

} // namespace server
