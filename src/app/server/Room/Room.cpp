#include "Room/Room.h"

namespace server {

RoomManager::RoomManager() = default;

void RoomManager::open(const QString &name, quint32 hostSessionId)
{
    room_ = Room{};
    room_.name = name.isEmpty() ? QStringLiteral("默认房间") : name;
    room_.hostSessionId = hostSessionId;
    open_ = true;
}

void RoomManager::reset()
{
    room_ = Room{};
    open_ = false;
}

void RoomManager::setPlaying(bool playing)
{
    if (!isOpen()) {
        return;
    }
    room_.playing = playing;
}

void RoomManager::setHostSession(quint32 sessionId)
{
    if (!isOpen()) {
        return;
    }
    room_.hostSessionId = sessionId;
}

} // namespace server
