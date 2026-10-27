#pragma once

#include <QString>
#include <QtGlobal>

namespace server {

// 服务端房间：没有对局时客户端看到的是房间，而不是对局。
// 当前是单房间模型；容量、等待/对局状态和房主会话由服务端权威决定，
// 客户端只能展示 RoomList。
struct Room {
    QString name = QStringLiteral("默认房间");
    int maxPlayers = 4;
    bool playing = false;
    quint32 hostSessionId = 0;
};

class RoomManager {
public:
    RoomManager();

    // 生命周期：open() 总是建立一个新房间；reset() 关闭当前房间。
    // 房间关闭后 isOpen()==false，客户端会收到空 RoomList。
    void open(const QString &name, quint32 hostSessionId = 0);
    void reset();
    bool isOpen() const { return open_; }

    Room &room() { return room_; }
    const Room &room() const { return room_; }

    void setPlaying(bool playing);
    void setHostSession(quint32 sessionId);

private:
    Room room_;
    bool open_ = false;
};

} // namespace server
