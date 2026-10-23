#pragma once

class Arena;

namespace arena {

// 碰撞事件组件：只负责“谁撞到谁”，不负责死亡后的奖励。
class Collision {
public:
    void resolvePlayerBullets(Arena &arena, float dt);
    void resolveEnemyBullets(Arena &arena, float dt);
    void separateEnemies(Arena &arena);
    void contactDamage(Arena &arena);
};

} // namespace arena
