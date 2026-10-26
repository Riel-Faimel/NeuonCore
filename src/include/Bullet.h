#pragma once

#include "Math.h"

#include <QPainter>

class Arena;

namespace arena {
class Fighter;
}

// 子弹的全局接口。arena::Bullets 池只持有 Bullet*，具体弹种实现推进、绘制和命中语义；
// 池不关心弹种数据，只通过这个接口驱动和回收。
class Bullet {
public:
    enum class Standpoint {
        Player,
        Enemy,
    };

    virtual ~Bullet() = default;

    // 位置推进：记录上一帧位置后按 dt 前进（含寿命、出屏判死）。
    virtual void advance(float dt) = 0;
    // 样式绘制。
    virtual void draw(QPainter &world, QPainter &glow) const = 0;
    // 碰撞事件：命中目标后由子弹自己结算伤害/穿透等，返回 false 表示这颗弹到此结束。
    virtual bool hit(Arena &arena, arena::Fighter &target) = 0;

    // 池用它检查是否回收（子弹通过 hit() 的返回值或自身寿命结束自己）。
    virtual bool alive() const = 0;

    // 几何参数：扫掠碰撞用“上一帧位置 → 当前位置”和碰撞半径。
    virtual const Math::Vec2 &prev() const = 0;
    virtual const Math::Vec2 &position() const = 0;
    virtual float radius() const = 0;

    // 阵营 ID：池据此分流，也决定和谁碰撞。
    virtual Standpoint standpoint() const = 0;
};
