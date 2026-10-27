#include "Net/Input.h"

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

} // namespace

QByteArray serializeInputCommand(const InputCommand &command)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << command.moveX << command.moveY << command.mouseActive
           << command.mouseX << command.mouseY << command.shooting
           << command.dashRequested << command.pauseToggle
           << command.mineRequested << command.mineX << command.mineY
           << command.upgradeChoice;
    return stream.status() == QDataStream::Ok ? payload : QByteArray();
}

bool deserializeInputCommand(const QByteArray &payload, InputCommand *command,
                             QString *error)
{
    if (command == nullptr) {
        setError(error, QStringLiteral("null input destination"));
        return false;
    }
    QDataStream stream(payload);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> command->moveX >> command->moveY >> command->mouseActive
        >> command->mouseX >> command->mouseY >> command->shooting
        >> command->dashRequested >> command->pauseToggle
        >> command->mineRequested >> command->mineX >> command->mineY
        >> command->upgradeChoice;
    if (stream.status() != QDataStream::Ok || !stream.atEnd()) {
        setError(error, QStringLiteral("input payload malformed"));
        return false;
    }
    return true;
}

QByteArray encodeInputMessage(const InputCommand &command, quint16 version)
{
    const QByteArray payload = serializeInputCommand(command);
    return payload.isEmpty() ? QByteArray()
                             : frameMessage(MessageType::Input, payload, version);
}

bool decodeInputMessage(const QByteArray &frame, InputCommand *command,
                        QString *error)
{
    MessageHeader header;
    QByteArray payload;
    if (!decodeFrame(frame, &header, &payload, error)) {
        return false;
    }
    if (header.type != MessageType::Input) {
        setError(error, QStringLiteral("message is not an input command"));
        return false;
    }
    if (header.version != kProtocolVersion) {
        setError(error, QStringLiteral("protocol version mismatch: %1 != %2")
                            .arg(header.version)
                            .arg(kProtocolVersion));
        return false;
    }
    return deserializeInputCommand(payload, command, error);
}

} // namespace net
