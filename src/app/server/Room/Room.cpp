#include "Room/Room.h"

#include <algorithm>

namespace server {

RoomManager::RoomManager() = default;

void RoomManager::open(const QString &name, quint32 hostSessionId)
{
    rooms_.clear();
    Room room;
    room.name = name.isEmpty() ? QStringLiteral("默认房间") : name;
    room.hostSessionId = hostSessionId;
    rooms_.push_back(std::move(room));
}

void RoomManager::reset()
{
    rooms_.clear();
}

void RoomManager::setPlayers(int players)
{
    if (!isOpen()) {
        return;
    }
    defaultRoom().players = std::clamp(players, 0, defaultRoom().maxPlayers);
}

void RoomManager::setPlaying(bool playing)
{
    if (!isOpen()) {
        return;
    }
    defaultRoom().playing = playing;
}

void RoomManager::setHostSession(quint32 sessionId)
{
    if (!isOpen()) {
        return;
    }
    defaultRoom().hostSessionId = sessionId;
}

} // namespace server
