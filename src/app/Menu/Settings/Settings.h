#pragma once

#include "Math.h"

#include <QRectF>

class QPainter;

namespace menu {

// Settings 组件：音效 / 背景音乐音量。只改 Base 暴露的音量接口，
// 自己负责命中测试和 settings.ini 的读写。
class Settings {
public:
    enum class Action {
        None,
        Back,
    };

    explicit Settings(bool persistent = true);

    void reset();
    void load();
    Action keyPress(int key);
    Action mousePress(const Math::Vec2 &position);
    void mouseMove(const Math::Vec2 &position);
    void mouseRelease();
    void render(QPainter &painter) const;

    QRectF rowRect(int row) const;
    QRectF trackRect(int row) const;
    QRectF backRect() const;

private:
    static QString path();
    void adjust(int row, float delta);
    void setFromPointer(const Math::Vec2 &position);
    void save() const;

    bool persistent_ = true;
    int row_ = 0;
    bool dragging_ = false;
};

} // namespace menu
