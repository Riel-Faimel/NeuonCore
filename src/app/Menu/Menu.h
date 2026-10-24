#pragma once

#include "Base/Base.h"
#include "Games/Games.h"

#include <QImage>
#include <QRectF>

#include <memory>
#include <vector>

class SelfTest;

// Menu 模块：主菜单、关卡选择、对局信息与结算界面。
// 它持有进程级的 Base 服务与关卡表，并在开局时构造一局 Games；
// Games 只活一局，返回主菜单即销毁，下一局重新构造。
class Menu {
public:
    enum class Screen {
        Main,
        Match,
        Result,
    };

    explicit Menu(bool enableAudio = true);
    ~Menu();

    Menu(const Menu &) = delete;
    Menu &operator=(const Menu &) = delete;

    void update(float dt);
    void render();
    const QImage &frame() const;

    void keyPress(int key, bool autoRepeat = false);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mouseReleaseLeft();
    void clearInput();
    void toggleMute();
    bool muted() const { return base_.muted(); }
    bool audioEnabled() const { return base_.audioEnabled(); }
    Screen screen() const { return screen_; }

private:
    friend class SelfTest;

    static constexpr int CustomLevelIndex = 2;

    void startLevel(int index);
    void returnToMenu();
    void drawMain();
    void drawResult();
    // 默认战役台本：运行时提交给 Games，Menu 只负责写数据和选起点。
    static games::Script standardScript();
    int maxCustomWave() const;
    void drawCustomPicker(QPainter &painter, const QRectF &rect, bool active);
    void adjustCustomWave(int delta);
    bool isCustomLevel(int index) const { return index == CustomLevelIndex; }
    QRectF levelRect(int index) const;
    QRectF customMinusRect() const;
    QRectF customPlusRect() const;
    QRectF retryRect() const;
    QRectF menuRect() const;

    Base base_;
    std::unique_ptr<Games> games_;
    std::vector<Games::Level> levels_;
    Screen screen_ = Screen::Main;
    QImage frame_;
    Math::Vec2 mouse_;
    int selected_ = 0;
    int customWave_ = 1;
    float menuTime_ = 0.0f;
    bool mouseActive_ = false;
};
