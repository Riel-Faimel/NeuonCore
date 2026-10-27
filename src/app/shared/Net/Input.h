#pragma once

#include "Net/Protocol.h"

#include <QByteArray>
#include <QString>

// 客户端 → 服务端的输入命令（阶段 3）。
//
// 客户端只发送“意图”，不发送世界状态：服务端把它翻译成 arena::InputState 并
// 调用 GameSimulation::setInput()。鼠标位置用于服务端计算玩家朝向，布雷/冲刺/
// 暂停/强化选择都是一次性动作，由服务端权威执行验证。
namespace net {

struct InputCommand {
    // 移动轴（-1..1）；鼠标世界坐标；是否正在瞄准/射击。
    float moveX = 0.0f;
    float moveY = 0.0f;
    bool mouseActive = false;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool shooting = false;

    // 一次性动作：客户端在事件发生时置位，发送后清零。
    bool dashRequested = false;
    bool pauseToggle = false;
    bool mineRequested = false;
    float mineX = 0.0f;
    float mineY = 0.0f;
    // -1 表示没有选择；0..2 对应三张强化卡。
    int upgradeChoice = -1;

    // 目标玩家（1..4）。TCP 客户端必须留 0：服务端只按会话自己的 playerId
    // 应用输入，客户端不能指定别的飞机。stdio 宿主用它选择控制哪一架；
    // 0 表示服务端默认（会话自己 / 对局 leader）。
    int playerId = 0;

    bool operator==(const InputCommand &) const = default;
};

QByteArray serializeInputCommand(const InputCommand &command);
bool deserializeInputCommand(const QByteArray &payload, InputCommand *command,
                             QString *error = nullptr);

QByteArray encodeInputMessage(const InputCommand &command,
                              quint16 version = kProtocolVersion);
bool decodeInputMessage(const QByteArray &frame, InputCommand *command,
                        QString *error = nullptr);

} // namespace net
