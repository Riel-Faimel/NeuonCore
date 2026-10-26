#pragma once

#include "Base/Base.h"
#include "Games/Games.h"

#include "Menu/Depot/Depot.h"
#include "Menu/Editor/Editor.h"
#include "Menu/Main/Main.h"
#include "Menu/Map/Map.h"
#include "Menu/Match/Match.h"
#include "Menu/Result/Result.h"
#include "Menu/Settings/Settings.h"

#include <QImage>

#include <vector>

class SelfTest;

// Menu 模块：只组合 7 个界面组件和进程级资源（Shop、关卡表），
// 负责屏幕状态机、输入路由和组件之间的数据传递。
// 和 Arena 一样，组件在 namespace menu 里，Menu 是唯一的模块入口。
// Base 不再由 Menu 拥有：它是进程级单例 BaseService（见 Base/Base.h）。
class Menu {
public:
    enum class Screen {
        Main,
        Map,
        Match,
        Result,
        Editor,
        Shop,
        Settings,
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
    void mousePressRight(const Math::Vec2 &position);
    void mouseReleaseLeft();
    void clearInput();
    void toggleMute();
    bool muted() const { return BaseService.muted(); }
    bool audioEnabled() const { return BaseService.audioEnabled(); }
    Screen screen() const { return screen_; }

private:
    friend class SelfTest;

    static constexpr int CustomLevelIndex = 1;

    // 标准战役台本：运行时提交给 Games。
    static games::Script standardScript();
    std::vector<menu::Main::Card> cards() const;
    menu::Result::Summary summary() const;

    void startLevel(int index);
    void returnToMenu();
    void openEditor();

    std::vector<Games::Level> levels_;

    menu::Main main_;
    menu::Map map_;
    menu::Match match_;
    menu::Result result_;
    menu::Editor editor_;
    menu::Depot depot_;
    menu::Settings settings_;

    Screen screen_ = Screen::Main;
    QImage frame_;
    Math::Vec2 mouse_;
    int level_ = 0;
    bool mouseActive_ = false;
    bool audioRequested_ = false;
};
