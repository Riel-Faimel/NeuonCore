#pragma once

#include "Base/Base.h"
#include "Games/Games.h"

#include "Menu/Armory/Armory.h"
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

// Menu 模块：只组合 8 个界面组件和进程级资源（Shop、Loadout、关卡表），
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
        Armory,
        Settings,
    };

    // offline=true 保留阶段 2 的本地直连路径（自动化测试/离线回退）；
    // 默认走网络路径：host 为空时由 Match 启动 127.0.0.1 内嵌服务端。
    explicit Menu(bool enableAudio = true, bool offline = false,
                  const QString &host = QString(), quint16 port = 0);
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
    static games::Script standardScript(int index);
    std::vector<menu::Main::Card> cards() const;
    menu::Result::Summary summary() const;

    void startLevel(int index);
    void startCampaign(int index);
    void retry();
    void returnToMenu();
    void openEditor();

    std::vector<Games::Level> levels_;
    std::vector<Games::Level> campaignLevels_;

    menu::Main main_;
    menu::Map map_;
    menu::Match match_;
    menu::Result result_;
    menu::Editor editor_;
    menu::Depot depot_;
    menu::Armory armory_;
    menu::Settings settings_;

    Screen screen_ = Screen::Main;
    QImage frame_;
    Math::Vec2 mouse_;
    int level_ = 0;
    int activeCampaign_ = 0;
    bool campaignActive_ = true;
    bool mouseActive_ = false;
    bool audioRequested_ = false;
};
