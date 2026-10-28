#pragma once

#include "Net/Input.h"
#include "Net/Protocol.h"

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QtGlobal>

#include <vector>

// 阶段 3 的控制消息（Hello / Welcome / StartMatch / Event / MatchEnd）。
//
// 与 Input / Snapshot 一样，payload 用 QDataStream 小端编码，外层统一是
// [u32 length][u16 type][u16 version][payload]。所有结构都只含数据：
// 台本里的敌人类型用注册表 dataKey 字符串，不在协议里出现 std::type_index。
namespace net {

struct HelloMessage {
    QString clientVersion;

    bool operator==(const HelloMessage &) const = default;
};

struct WelcomeMessage {
    quint32 sessionId = 0;
    // 服务端分配给这个会话的稳定玩家 id/slot（1..4）；0 表示没有槽位。
    int playerId = 0;
    quint16 tickRate = 60;
    bool accepted = false;
    QString reason;

    bool operator==(const WelcomeMessage &) const = default;
};

struct StartMatchSpawn {
    QString dataKey;
    int count = 1;

    bool operator==(const StartMatchSpawn &) const = default;
};

struct StartMatchWave {
    std::vector<StartMatchSpawn> spawns;

    bool operator==(const StartMatchWave &) const = default;
};

struct StartMatchMessage {
    QString levelName;
    int startWave = 1;
    int enemyLevel = 1;
    std::vector<StartMatchWave> waves;

    bool operator==(const StartMatchMessage &) const = default;
};

enum class EventKind : quint16 {
    WaveStarted = 1,
    UpgradeOpened = 2,
    UpgradeSelected = 3,
    BossDown = 4,
    WaveCleared = 5,
    Victory = 6,
    VictoryFinished = 7,
    Defeat = 8,
    Feedback = 9,
};

struct EventMessage {
    EventKind kind = EventKind::WaveStarted;
    int wave = 0;
    bool boss = false;
    bool finalWave = false;
    QColor color;
    int alpha = 0;
    float shake = 0.0f;

    bool operator==(const EventMessage &) const = default;
};

struct MatchEndMessage {
    bool victory = false;
    int score = 0;       // 服务端钱包
    int runScore = 0;    // 本局获得
    int kills = 0;
    int currentWave = 0;
    int totalWaves = 0;
    float runTime = 0.0f;
    bool retryable = true;
    // 空字符串表示正常结束；非空表示服务端拒绝了 StartMatch 等原因。
    QString reason;

    bool operator==(const MatchEndMessage &) const = default;
};

struct RoomPlayerInfo {
    quint32 sessionId = 0;
    int playerId = 0;
    bool host = false;
    bool inMatch = false;

    bool operator==(const RoomPlayerInfo &) const = default;
};

struct RoomInfo {
    QString name;
    int players = 0;
    int maxPlayers = 4;
    bool playing = false;
    // 服务端实际 welcomed 的会话列表（列表=计数，连接/断开会同步更新）。
    std::vector<RoomPlayerInfo> members;

    bool operator==(const RoomInfo &) const = default;
};

// 单房间模型：列表保留一个元素，便于以后扩展多房间而不改外层帧。
struct ClaimAdminMessage {
    quint32 sessionId = 0;
    bool operator==(const ClaimAdminMessage &) const = default;
};

struct RoomListMessage {
    std::vector<RoomInfo> rooms;
    // Host 从结果页“重新部署”时置 true：客户端从结算页回到房间。
    bool enterRoom = false;

    bool operator==(const RoomListMessage &) const = default;
};

// ----- QDataStream payload 序列化 -----
QByteArray serializeHello(const HelloMessage &message);
bool deserializeHello(const QByteArray &payload, HelloMessage *message,
                      QString *error = nullptr);

QByteArray serializeWelcome(const WelcomeMessage &message);
bool deserializeWelcome(const QByteArray &payload, WelcomeMessage *message,
                        QString *error = nullptr);

QByteArray serializeStartMatch(const StartMatchMessage &message);
bool deserializeStartMatch(const QByteArray &payload, StartMatchMessage *message,
                           QString *error = nullptr);

QByteArray serializeEvent(const EventMessage &message);
bool deserializeEvent(const QByteArray &payload, EventMessage *message,
                      QString *error = nullptr);

QByteArray serializeMatchEnd(const MatchEndMessage &message);
bool deserializeMatchEnd(const QByteArray &payload, MatchEndMessage *message,
                         QString *error = nullptr);

QByteArray serializeClaimAdmin(const ClaimAdminMessage &message);
bool deserializeClaimAdmin(const QByteArray &payload, ClaimAdminMessage *message,
                           QString *error = nullptr);

QByteArray serializeRoomList(const RoomListMessage &message);
bool deserializeRoomList(const QByteArray &payload, RoomListMessage *message,
                         QString *error = nullptr);

// ----- 带帧头的完整消息 -----
QByteArray encodeHello(const HelloMessage &message,
                       quint16 version = kProtocolVersion);
bool decodeHello(const QByteArray &frame, HelloMessage *message,
                 QString *error = nullptr);

QByteArray encodeWelcome(const WelcomeMessage &message,
                         quint16 version = kProtocolVersion);
bool decodeWelcome(const QByteArray &frame, WelcomeMessage *message,
                   QString *error = nullptr);

QByteArray encodeStartMatch(const StartMatchMessage &message,
                            quint16 version = kProtocolVersion);
bool decodeStartMatch(const QByteArray &frame, StartMatchMessage *message,
                      QString *error = nullptr);

QByteArray encodeEvent(const EventMessage &message,
                       quint16 version = kProtocolVersion);
bool decodeEvent(const QByteArray &frame, EventMessage *message,
                 QString *error = nullptr);

QByteArray encodeMatchEnd(const MatchEndMessage &message,
                          quint16 version = kProtocolVersion);
bool decodeMatchEnd(const QByteArray &frame, MatchEndMessage *message,
                    QString *error = nullptr);

QByteArray encodeClaimAdmin(const ClaimAdminMessage &message,
                          quint16 version = kProtocolVersion);
bool decodeClaimAdmin(const QByteArray &frame, ClaimAdminMessage *message,
                      QString *error = nullptr);

QByteArray encodeRoomList(const RoomListMessage &message,
                          quint16 version = kProtocolVersion);
bool decodeRoomList(const QByteArray &frame, RoomListMessage *message,
                    QString *error = nullptr);

} // namespace net
