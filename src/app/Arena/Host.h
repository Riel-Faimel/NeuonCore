#pragma once

#include <QColor>

namespace arena {

// 组件 → 模块的回调接口（虚函数注入）。
//
// 组件（Bullets / Combatants / Entities / Events / State）是 Arena 的私有基类，
// 它们不直接依赖 Arena 的实现，也不保存 std::function / 回指指针：
// Arena 继承 Host 并实现这些虚函数，调用组件时把 Host& 放进 arena::Context 注入。
class Host {
public:
    virtual ~Host() = default;

    // 请求模块级的画面反馈：闪屏（alpha > 0）与震屏（shake > 0）。
    virtual void addFeedback(const QColor &color, int alpha, float shake) = 0;
};

} // namespace arena
