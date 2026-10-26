#pragma once

class Arena;

namespace arena {

class Enemy;
struct Mine;

// 事件组件：一个 class 同时负责碰撞事件与击杀事件。
class Events {
public:
    void resolvePlayerBullets(Arena &arena);
    void resolveEnemyBullets(Arena &arena);
    void resolveMines(Arena &arena);
    void separateEnemies(Arena &arena);
    void contactDamage(Arena &arena);
    void handleKill(Arena &arena, Enemy &enemy);

private:
    void detonateMine(Arena &arena, Mine &mine);
};

} // namespace arena
