#pragma once

class Arena;

namespace arena {

class Enemy;

// 事件组件：一个 class 同时负责碰撞事件与击杀事件。
class Events {
public:
    void resolvePlayerBullets(Arena &arena, float dt);
    void resolveEnemyBullets(Arena &arena, float dt);
    void separateEnemies(Arena &arena);
    void contactDamage(Arena &arena);
    void handleKill(Arena &arena, Enemy &enemy);
};

} // namespace arena
