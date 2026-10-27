#include "Net/Messages.h"

#include <QDataStream>
#include <QIODevice>

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

QByteArray serializeRoomList(const RoomListMessage &message)
{
    return serialize([&](QDataStream &stream) {
        stream << quint32(message.rooms.size());
        for (const RoomInfo &room : message.rooms) {
            stream << room.id << room.name << qint32(room.players)
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
            message->rooms.clear();
            message->rooms.reserve(count);
            for (quint32 i = 0; i < count; ++i) {
                RoomInfo room;
                qint32 players = 0;
                qint32 maxPlayers = 0;
                stream >> room.id >> room.name >> players >> maxPlayers
                    >> room.playing;
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

QByteArray serializeJoinRoom(const JoinRoomMessage &message)
{
    return serialize([&](QDataStream &stream) { stream << message.roomId; });
}

bool deserializeJoinRoom(const QByteArray &payload, JoinRoomMessage *message,
                         QString *error)
{
    if (message == nullptr) {
        setError(error, QStringLiteral("null join room destination"));
        return false;
    }
    return deserialize(payload,
                       [&](QDataStream &stream) {
                           stream >> message->roomId;
                           return stream.status() == QDataStream::Ok;
                       },
                       error);
}

QByteArray encodeHello(const HelloMessage &message, quint16 version)
{
    const QByteArray payload = serializeHello(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::Hello, payload, version);
}

bool decodeHello(const QByteArray &frame, HelloMessage *message, QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::Hello, &payload, error)
           && deserializeHello(payload, message, error);
}

QByteArray encodeWelcome(const WelcomeMessage &message, quint16 version)
{
    const QByteArray payload = serializeWelcome(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::Welcome, payload, version);
}

bool decodeWelcome(const QByteArray &frame, WelcomeMessage *message, QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::Welcome, &payload, error)
           && deserializeWelcome(payload, message, error);
}

QByteArray encodeStartMatch(const StartMatchMessage &message, quint16 version)
{
    const QByteArray payload = serializeStartMatch(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::StartMatch, payload, version);
}

bool decodeStartMatch(const QByteArray &frame, StartMatchMessage *message,
                      QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::StartMatch, &payload, error)
           && deserializeStartMatch(payload, message, error);
}

QByteArray encodeEvent(const EventMessage &message, quint16 version)
{
    const QByteArray payload = serializeEvent(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::Event, payload, version);
}

bool decodeEvent(const QByteArray &frame, EventMessage *message, QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::Event, &payload, error)
           && deserializeEvent(payload, message, error);
}

QByteArray encodeMatchEnd(const MatchEndMessage &message, quint16 version)
{
    const QByteArray payload = serializeMatchEnd(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::MatchEnd, payload, version);
}

bool decodeMatchEnd(const QByteArray &frame, MatchEndMessage *message, QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::MatchEnd, &payload, error)
           && deserializeMatchEnd(payload, message, error);
}

QByteArray encodeRoomList(const RoomListMessage &message, quint16 version)
{
    const QByteArray payload = serializeRoomList(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::RoomList, payload, version);
}

bool decodeRoomList(const QByteArray &frame, RoomListMessage *message, QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::RoomList, &payload, error)
           && deserializeRoomList(payload, message, error);
}

QByteArray encodeJoinRoom(const JoinRoomMessage &message, quint16 version)
{
    const QByteArray payload = serializeJoinRoom(message);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::JoinRoom, payload, version);
}

bool decodeJoinRoom(const QByteArray &frame, JoinRoomMessage *message, QString *error)
{
    QByteArray payload;
    return decodeMessageFrame(frame, MessageType::JoinRoom, &payload, error)
           && deserializeJoinRoom(payload, message, error);
}

} // namespace net
