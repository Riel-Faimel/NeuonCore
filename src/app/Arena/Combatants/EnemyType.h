#pragma once

#include <typeindex>
#include <typeinfo>

namespace arena {

// 敌人类型标识：直接用 C++ RTTI（typeid）组织，不再用分类字符串。
// 字符串只在 JSON/台本/编辑器等数据边界出现，进游戏逻辑前解析成 EnemyType。
using EnemyType = std::type_index;

} // namespace arena
