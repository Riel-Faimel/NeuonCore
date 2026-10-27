#pragma once

#include <QString>
#include <QtGlobal>

#include <vector>

namespace server {

// 服务端房间：没有对局时客户端看到的是房间，而不是对局。
// 当前是单房间模型；id/容量/状态/房主会话已经是多房间需要的字段。
// 房间的开/关、对局状态和人数都只由服务端决定，客户端只能展示 RoomList。
struct Room {
    quint32 id = 1;
    QString name = QStringLiteral("默认房间");
    int players = 0;
    int maxPlayers = 4;
    bool playing = false;
    quint32 hostSessionId = 0;
};

class RoomManager {
public:
    RoomManager();

    // 生命周期：open() 总是建立一个新房间；reset() 关闭当前房间。
    // 房间关闭后 rooms() 为空，客户端会收到空列表。
    void open(const QString &name, quint32 hostSessionId = 0);
    void reset();
    bool isOpen() const { return !rooms_.empty(); }

    Room &defaultRoom() { return rooms_.front(); }
    const Room &defaultRoom() const { return rooms_.front(); }
    const std::vector<Room> &rooms() const { return rooms_; }

    void setPlayers(int players);
    void setPlaying(bool playing);
    void setHostSession(quint32 sessionId);

private:
    std::vector<Room> rooms_;
};

} // namespace server
