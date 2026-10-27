#include "Net/Protocol.h"

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

QByteArray frameMessage(MessageType type, const QByteArray &payload, quint16 version)
{
    if (payload.size() < 0 || quint32(payload.size()) > kMaxPayloadBytes) {
        return {};
    }
    QByteArray frame;
    frame.reserve(int(kFrameHeaderSize) + payload.size());
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(payload.size()) << quint16(type) << version;
    if (!payload.isEmpty()) {
        stream.writeRawData(payload.constData(), payload.size());
    }
    return stream.status() == QDataStream::Ok ? frame : QByteArray();
}

FrameBuffer::Result FrameBuffer::next(QByteArray *frame, QString *error)
{
    if (frame == nullptr) {
        setError(error, QStringLiteral("null frame destination"));
        return Result::Error;
    }
    if (buffer_.size() < int(kFrameHeaderSize)) {
        return Result::NeedMore;
    }

    QDataStream stream(buffer_);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint32 length = 0;
    stream >> length;
    if (stream.status() != QDataStream::Ok) {
        setError(error, QStringLiteral("unreadable frame length"));
        return Result::Error;
    }
    if (length > kMaxPayloadBytes) {
        setError(error, QStringLiteral("payload too large"));
        return Result::Error;
    }
    const qsizetype total = qsizetype(kFrameHeaderSize) + qsizetype(length);
    if (buffer_.size() < total) {
        return Result::NeedMore;
    }

    *frame = buffer_.left(total);
    buffer_.remove(0, total);
    return Result::Ready;
}

bool decodeFrame(const QByteArray &bytes, MessageHeader *header, QByteArray *payload,
                 QString *error)
{
    if (header == nullptr || payload == nullptr) {
        setError(error, QStringLiteral("null frame destination"));
        return false;
    }
    if (bytes.size() < int(kFrameHeaderSize)) {
        setError(error, QStringLiteral("frame shorter than header"));
        return false;
    }

    QDataStream stream(bytes);
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint32 length = 0;
    quint16 type = 0;
    quint16 version = 0;
    stream >> length >> type >> version;
    if (stream.status() != QDataStream::Ok) {
        setError(error, QStringLiteral("unreadable frame header"));
        return false;
    }
    if (length > kMaxPayloadBytes) {
        setError(error, QStringLiteral("payload too large"));
        return false;
    }
    if (quint32(bytes.size()) != kFrameHeaderSize + length) {
        setError(error, QStringLiteral("frame length mismatch"));
        return false;
    }

    header->length = length;
    header->type = MessageType(type);
    header->version = version;
    *payload = bytes.mid(int(kFrameHeaderSize), int(length));
    return true;
}

} // namespace net
