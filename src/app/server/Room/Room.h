#pragma once

#include <QString>
#include <QtGlobal>

#include <vector>

namespace server {

// 服务端房间：没有对局时客户端看到的是房间，而不是对局。
// 当前是单房间模型；id/容量/状态已经是多房间需要的字段。
struct Room {
    quint32 id = 1;
    QString name = QStringLiteral("默认房间");
    int players = 0;
    int maxPlayers = 4;
    bool playing = false;
};

class RoomManager {
public:
    RoomManager();

    Room &defaultRoom() { return rooms_.front(); }
    const Room &defaultRoom() const { return rooms_.front(); }
    const std::vector<Room> &rooms() const { return rooms_; }

    void reset();
    void setPlayers(int players);
    void setPlaying(bool playing);

private:
    std::vector<Room> rooms_;
};

} // namespace server
