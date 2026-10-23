#pragma once

class Arena;

namespace arena {

class Enemy;

// 击杀事件组件：只处理“目标死亡之后”的奖励、掉落、分裂和 Boss 结算。
class Kill {
public:
    void handle(Arena &arena, Enemy &enemy);
};

} // namespace arena
