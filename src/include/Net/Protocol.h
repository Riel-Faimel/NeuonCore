#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

class QDataStream;

// 网络协议层（阶段 2：只定义消息格式，不建立连接）。
//
// 所有消息统一成：
//   [u32 length][u16 type][u16 version][payload]
// length 是 payload 的字节数（不含 8 字节头），字节序固定小端。
// 阶段 3 的 QTcpSocket 只负责收发这些完整帧；快照本身的读写见 Net/Snapshot.h。
namespace net {

inline constexpr quint16 kProtocolVersion = 1;
inline constexpr quint16 kSnapshotDataVersion = 1;
inline constexpr quint16 kDefaultPort = 7777;
inline constexpr quint32 kFrameHeaderSize = 8;
inline constexpr quint32 kMaxPayloadBytes = 4u * 1024u * 1024u;
// 防止损坏的长度字段/计数导致可信端一次性提交巨量分配。
inline constexpr quint32 kMaxSnapshotItems = 100000u;

enum class MessageType : quint16 {
    Hello = 1,        // C→S：协议版本、客户端版本
    Welcome = 2,      // S→C：会话 id、tick rate、是否接受加入
    StartMatch = 3,   // 宿主→S（stdio/本地）：关卡 id / 自定义台本（类型用数据 key）
    Input = 4,        // C→S（仅主控会话）/ stdio：按键、鼠标、冲刺、布雷
    Snapshot = 5,     // S→C：世界快照
    Event = 6,        // S→C：音效、横幅、震屏/闪屏等表现事件
    MatchEnd = 7,     // S→C：胜负、统计、可重试
    RoomList = 8,     // S→C：没有对局时，服务端展示房间而不是对局
    JoinRoom = 9,     // C→S：请求加入房间
};

struct MessageHeader {
    quint32 length = 0;  // payload 字节数
    MessageType type = MessageType::Hello;
    quint16 version = kProtocolVersion;
};

// 把 payload 包成一条完整消息。payload 过大时返回空数组。
QByteArray frameMessage(MessageType type, const QByteArray &payload,
                        quint16 version = kProtocolVersion);

// 校验并拆开一条完整消息。bytes 必须正好是一帧（阶段 3 由读循环负责切帧）。
// 失败时返回 false，并把原因写进 error（若提供）。
bool decodeFrame(const QByteArray &bytes, MessageHeader *header, QByteArray *payload,
                 QString *error = nullptr);

// TCP 粘包/拆包缓冲：append() 收到的字节，next() 取出一条完整帧。
// 协议头里的 length 只表示 payload 长度，所以一帧是 kFrameHeaderSize + length。
class FrameBuffer {
public:
    enum class Result {
        NeedMore,
        Ready,
        Error,
    };

    void append(const QByteArray &bytes) { buffer_.append(bytes); }
    void clear() { buffer_.clear(); }
    int bufferedBytes() const { return buffer_.size(); }

    // Ready 时把完整帧写进 frame；Error 时把原因写进 error（若提供）。
    Result next(QByteArray *frame, QString *error = nullptr);

private:
    QByteArray buffer_;
};

} // namespace net
