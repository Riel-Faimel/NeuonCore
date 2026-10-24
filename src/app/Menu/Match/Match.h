#pragma once

#include "Games/Games.h"

#include <QImage>

#include <memory>

class Base;
class SelfTest;

namespace menu {

// Match 组件：持有“当前这一局”，负责开局、推进、渲染和局内输入转发。
// 只活一局：Menu 开局时 start()，回主菜单时 stop()。
class Match {
public:
    void start(const Games::Level &level, Base &base);
    void stop();
    bool active() const { return games_ != nullptr; }

    void update(float dt);
    void render();
    const QImage &frame() const;
    Games::State state() const;
    bool finished() const;
    int score() const;
    int kills() const;
    int wave() const;
    int totalWaves() const;
    float runTime() const;

    void keyPress(int key, bool autoRepeat);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mouseReleaseLeft();

    // 测试 / 调试用；正常流程不需要直接拿 Games。
    Games *game() { return games_.get(); }

private:
    friend class ::SelfTest;

    std::unique_ptr<Games> games_;
};

} // namespace menu
