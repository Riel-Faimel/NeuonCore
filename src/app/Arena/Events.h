#pragma once

namespace arena {

struct Context;
class Enemy;
struct Mine;

// 事件组件：一个 class 同时负责碰撞事件与击杀事件。
class Events {
public:
    void resolvePlayerBullets(const Context &ctx);
    void resolveEnemyBullets(const Context &ctx);
    void resolveMines(const Context &ctx);
    void separateEnemies(const Context &ctx);
    void contactDamage(const Context &ctx);
    void handleKill(const Context &ctx, Enemy &enemy);

private:
    void detonateMine(const Context &ctx, Mine &mine);
};

} // namespace arena
