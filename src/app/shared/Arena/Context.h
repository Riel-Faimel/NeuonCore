#pragma once

namespace arena {

class Host;
class Bullets;
class Combatants;
class Entities;
class Events;
class State;

// 组件的运行上下文：Arena 在调用组件时构造，把宿主接口和兄弟组件的引用显式交给它。
// 这是模块内部的注入点——外部拿不到 Context，所以组件接口不会泄漏到 Arena 的公共 API。
// 组件 → 模块的反馈走 Host 的虚函数（Host 由 Arena 实现）。
struct Context {
    Host &host;
    Bullets &bullets;
    Combatants &combatants;
    Entities &entities;
    Events &events;
    State &state;
};

} // namespace arena
