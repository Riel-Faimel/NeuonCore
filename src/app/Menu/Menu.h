#pragma once

#include "Games/Games.h"

#include <QImage>
#include <QRectF>

#include <vector>

class SelfTest;

// Menu 模块：主菜单、关卡选择、对局信息与结算界面。
// Games 只负责一局的局内逻辑；Renderer 只负责局内表现。
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
    bool muted() const { return games_.muted(); }
    bool audioEnabled() const { return games_.audioEnabled(); }
    bool inMatch() const { return screen_ == Screen::Match; }
    Screen screen() const { return screen_; }

private:
    friend class SelfTest;

    void startLevel(int index);
    void returnToMenu();
    void drawMain();
    void drawResult();
    QRectF levelRect(int index) const;
    QRectF retryRect() const;
    QRectF menuRect() const;

    Games games_;
    std::vector<Games::Level> levels_;
    Screen screen_ = Screen::Main;
    QImage frame_;
    Math::Vec2 mouse_;
    int selected_ = 0;
    float menuTime_ = 0.0f;
    bool mouseActive_ = false;
};
