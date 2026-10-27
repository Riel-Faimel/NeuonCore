#pragma once

#include "Math.h"

#include <QColor>
#include <QRectF>
#include <QString>

#include <vector>

class QPainter;

namespace menu {

// Room 组件：对局开始前的房间/大厅界面。
// 主机与加入者看到同一份服务端权威房间数据（名字、人数、等待状态），
// 只有主机能开始对局或解散房间；加入者只能等待或离开。
class Room {
public:
    enum class Action {
        None,
        Start,
        Dissolve,
        Back,
    };

    // RoomList 里一个会话槽位的展示数据（人数与列表都来自服务端）。
    struct PlayerSlot {
        QString name;
        QString detail;
        bool host = false;
        bool you = false;
        bool inMatch = false;
    };

    struct View {
        bool host = false;         // 本机是房主（本地内嵌 GameServer）
        bool youAreHost = false;   // 服务端房间的 hostSessionId 指向本会话
        bool connected = false;    // 客户端已完成 Hello/Welcome
        bool playing = false;      // 房主已经 startMatch，正在等第一份快照
        bool canStart = false;
        QString roomName;
        QString endpoint;
        QString scriptSummary;
        QString status;
        int players = 0;
        int maxPlayers = 4;
        // 服务端 RoomList 的会话槽位，与 players 计数保持一致。
        std::vector<PlayerSlot> playerSlots;
    };

    void update(float dt, const Math::Vec2 &mouse, bool mouseActive);
    void render(QPainter &painter, const View &view) const;

    Action keyPress(int key, bool autoRepeat, const View &view) const;
    Action mousePress(const Math::Vec2 &position, const View &view) const;
    void mouseMove(const Math::Vec2 &position);

    QRectF startRect() const;
    QRectF dissolveRect() const;
    QRectF backRect() const;

private:
    bool pointerHovering(const QRectF &rect) const;
    float pulse() const;
    void drawButton(QPainter &painter, const QRectF &rect, const QString &key,
                    const QString &label, const QColor &tint, bool primary,
                    bool enabled) const;

    float time_ = 0.0f;
    Math::Vec2 pointer_;
    bool pointerActive_ = false;
};

} // namespace menu
