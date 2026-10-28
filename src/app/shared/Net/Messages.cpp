#include "Net/Messages.h"

#include <QDataStream>
#include <QIODevice>

#include <utility>

namespace net {

namespace {

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

template <typename Writer>
QByteArray serialize(Writer writer)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    writer(stream);
    return stream.status() == QDataStream::Ok ? payload : QByteArray();
}

template <typename Reader>
bool deserialize(const QByteArray &payload, Reader reader, QString *error)
{
    QDataStream stream(payload);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    if (!reader(stream)) {
        setError(error, QStringLiteral("truncated or malformed message payload"));
        return false;
    }
    if (stream.status() != QDataStream::Ok || !stream.atEnd()) {
        setError(error, QStringLiteral("message payload malformed"));
        return false;
    }
    return true;
}

bool decodeMessageFrame(const QByteArray &frame, MessageType expected,
                        QByteArray *payload, QString *error)
{
    MessageHeader header;
    if (!decodeFrame(frame, &header, payload, error)) {
        return false;
    }
    if (header.type != expected) {
        setError(error, QStringLiteral("unexpected message type %1").arg(int(header.type)));
        return false;
    }
    if (header.version != kProtocolVersion) {
        setError(error, QStringLiteral("protocol version mismatch: %1 != %2")
                            .arg(header.version)
                            .arg(kProtocolVersion));
        return false;
    }
    return true;
}

// 各消息的 payload 编解码函数签名一致；把“payload -> 帧”和
// “帧 -> payload -> 消息”的重复样板集中在这里，具体消息只声明自己的字段。
template <typename Message>
QByteArray encodeMessage(MessageType type, const Message &message,
                         QByteArray (*serializer)(const Message &), quint16 version)
{
    const QByteArray payload = serializer(message);
    return payload.isEmpty() ? QByteArray() : frameMessage(type, payload, version);
}

template <typename Message>
bool decodeMessage(MessageType type, const QByteArray &frame, Message *message,
                   bool (*deserializer)(const QByteArray &, Message *, QString *),
                   QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, type, &payload, error)
           && deserializer(payload, message, error);
}

bool readCount(QDataStream &stream, quint32 *count, quint32 maximum, QString *error)
{
    quint32 value = 0;
    stream >> value;
    if (stream.status() != QDataStream::Ok) {
        setError(error, QStringLiteral("truncated item count"));
        return false;
    }
    if (value > maximum) {
        setError(error, QStringLiteral("item count out of range"));
        return false;
    }
    *count = value;
    return true;
}

} // namespace

QByteArray serializeHello(const HelloMessage &message)
{
    return serialize([&](QDataStream &stream) { stream << message.clientVersion; });
}

bool deserializeHello(const QByteArray &payload, HelloMessage *message, QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null hello destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           stream >> message->clientVersion;
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}

QByteArray serializeWelcome(const WelcomeMessage &message)
{
    return serialize([&](QDataStream &stream) {
        stream << message.sessionId << qint32(message.playerId) << message.tickRate
               << message.accepted << message.reason;
    });
}

bool deserializeWelcome(const QByteArray &payload, WelcomeMessage *message,
                        QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null welcome destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           qint32 playerId = 0;
                           stream >> message->sessionId >> playerId
                               >> message->tickRate >> message->accepted
                               >> message->reason;
                           message->playerId = int(playerId);
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}

QByteArray serializeStartMatch(const StartMatchMessage &message)
{
    return serialize([&](QDataStream &stream) {
        stream << message.levelName << message.startWave << message.enemyLevel;
        stream << quint32(message.waves.size());
        for (const StartMatchWave &wave : message.waves) {
            stream << quint32(wave.spawns.size());
            for (const StartMatchSpawn &spawn : wave.spawns) {
                stream << spawn.dataKey << spawn.count;
            }
        }
    });
}

bool deserializeStartMatch(const QByteArray &payload, StartMatchMessage *message,
                           QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null start match destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           stream >> message->levelName >> message->startWave
                               >> message->enemyLevel;
                           if (stream.status() != QDataStream::Ok) {
                               return false;
                           }
                           quint32 waveCount = 0;
                           if (!readCount(stream, &waveCount, 512, error)) {
                               return false;
                           }
                           message->waves.clear();
                           message->waves.resize(size_t(waveCount));
                           for (StartMatchWave &wave : message->waves) {
                               quint32 spawnCount = 0;
                               if (!readCount(stream, &spawnCount, 256, error)) {
                                   return false;
                               }
                               wave.spawns.resize(size_t(spawnCount));
                               for (StartMatchSpawn &spawn : wave.spawns) {
                                   stream >> spawn.dataKey >> spawn.count;
                                   if (stream.status() != QDataStream::Ok) {
                                       return false;
                                   }
                               }
                           }
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}

QByteArray serializeEvent(const EventMessage &message)
{
    return serialize([&](QDataStream &stream) {
        stream << quint16(message.kind) << message.wave << message.boss
               << message.finalWave << message.color << message.alpha << message.shake;
    });
}

bool deserializeEvent(const QByteArray &payload, EventMessage *message, QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null event destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           quint16 kind = 0;
                           stream >> kind >> message->wave >> message->boss
                               >> message->finalWave >> message->color >> message->alpha
                               >> message->shake;
                           message->kind = EventKind(kind);
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}

QByteArray serializeMatchEnd(const MatchEndMessage &message)
{
    return serialize([&](QDataStream &stream) {
        stream << message.victory << message.score << message.runScore
               << message.kills << message.currentWave << message.totalWaves
               << message.runTime << message.retryable << message.reason;
    });
}

bool deserializeMatchEnd(const QByteArray &payload, MatchEndMessage *message,
                         QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null match end destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           stream >> message->victory >> message->score
                               >> message->runScore >> message->kills
                               >> message->currentWave >> message->totalWaves
                               >> message->runTime >> message->retryable
                               >> message->reason;
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}

QByteArray serializeClaimAdmin(const ClaimAdminMessage &message)
{
    return serialize([&](QDataStream &stream) { stream << message.sessionId; });
}

bool deserializeClaimAdmin(const QByteArray &payload, ClaimAdminMessage *message,
                           QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null claim-admin destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           stream >> message->sessionId;
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}
QByteArray serializeRoomList(const RoomListMessage &message)
{
    return serialize([&](QDataStream &stream) {
        stream << quint32(message.rooms.size()) << message.enterRoom;
        for (const RoomInfo &room : message.rooms) {
            stream << room.name << qint32(room.players)
                   << qint32(room.maxPlayers) << room.playing;
            stream << quint32(room.members.size());
            for (const RoomPlayerInfo &member : room.members) {
                stream << member.sessionId << qint32(member.playerId)
                       << member.host << member.inMatch;
            }
        }
    });
}

bool deserializeRoomList(const QByteArray &payload, RoomListMessage *message,
                         QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null room list destination"));
        return false;
    }
    return deserialize(
        payload,
        [&](QDataStream &stream) {
            quint32 count = 0;
            if (!readCount(stream, &count, kMaxSnapshotItems, error)) {
                return false;
            }
            stream >> message->enterRoom;
            message->rooms.clear();
            message->rooms.reserve(count);
            for (quint32 i = 0; i < count; ++i) {
                RoomInfo room;
                qint32 players = 0;
                qint32 maxPlayers = 0;
                stream >> room.name >> players >> maxPlayers >> room.playing;
                if (stream.status() != QDataStream::Ok) {
                    return false;
                }
                room.players = int(players);
                room.maxPlayers = int(maxPlayers);
                quint32 memberCount = 0;
                if (!readCount(stream, &memberCount, kMaxSnapshotItems, error)) {
                    return false;
                }
                room.members.resize(size_t(memberCount));
                for (RoomPlayerInfo &member : room.members) {
                    qint32 playerId = 0;
                    stream >> member.sessionId >> playerId >> member.host
                        >> member.inMatch;
                    member.playerId = int(playerId);
                    if (stream.status() != QDataStream::Ok) {
                        return false;
                    }
                }
                message->rooms.push_back(std::move(room));
            }
            return true;
        },
        error);
}

QByteArray encodeHello(const HelloMessage &message, quint16 version)
{
    return encodeMessage(MessageType::Hello, message, serializeHello, version);
}

bool decodeHello(const QByteArray &frame, HelloMessage *message, QString *error)
{
    return decodeMessage(MessageType::Hello, frame, message, deserializeHello, error);
}

QByteArray encodeWelcome(const WelcomeMessage &message, quint16 version)
{
    return encodeMessage(MessageType::Welcome, message, serializeWelcome, version);
}

bool decodeWelcome(const QByteArray &frame, WelcomeMessage *message, QString *error)
{
    return decodeMessage(MessageType::Welcome, frame, message, deserializeWelcome, error);
}

QByteArray encodeStartMatch(const StartMatchMessage &message, quint16 version)
{
    return encodeMessage(MessageType::StartMatch, message, serializeStartMatch, version);
}

bool decodeStartMatch(const QByteArray &frame, StartMatchMessage *message,
                      QString *error)
{
    return decodeMessage(MessageType::StartMatch, frame, message,
                         deserializeStartMatch, error);
}

QByteArray encodeEvent(const EventMessage &message, quint16 version)
{
    return encodeMessage(MessageType::Event, message, serializeEvent, version);
}

bool decodeEvent(const QByteArray &frame, EventMessage *message, QString *error)
{
    return decodeMessage(MessageType::Event, frame, message, deserializeEvent, error);
}

QByteArray encodeMatchEnd(const MatchEndMessage &message, quint16 version)
{
    return encodeMessage(MessageType::MatchEnd, message, serializeMatchEnd, version);
}

bool decodeMatchEnd(const QByteArray &frame, MatchEndMessage *message, QString *error)
{
    return decodeMessage(MessageType::MatchEnd, frame, message, deserializeMatchEnd,
                         error);
}

QByteArray encodeClaimAdmin(const ClaimAdminMessage &message, quint16 version)
{
    const QByteArray payload = serializeClaimAdmin(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::ClaimAdmin, payload, version);
}

bool decodeClaimAdmin(const QByteArray &frame, ClaimAdminMessage *message,
                      QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::ClaimAdmin, &payload, error)
           && deserializeClaimAdmin(payload, message, error);
}
QByteArray encodeRoomList(const RoomListMessage &message, quint16 version)
{
    return encodeMessage(MessageType::RoomList, message, serializeRoomList, version);
}

bool decodeRoomList(const QByteArray &frame, RoomListMessage *message, QString *error)
{
    return decodeMessage(MessageType::RoomList, frame, message, deserializeRoomList,
                         error);
}

} // namespace net
