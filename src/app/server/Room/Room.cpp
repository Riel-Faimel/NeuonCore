#include "Room/Room.h"

#include <algorithm>

namespace server {

RoomManager::RoomManager()
{
    reset();
}

void RoomManager::reset()
{
    rooms_.clear();
    rooms_.push_back(Room{});
}

void RoomManager::setPlayers(int players)
{
    defaultRoom().players = std::max(0, players);
}

void RoomManager::setPlaying(bool playing)
{
    defaultRoom().playing = playing;
}

} // namespace server
