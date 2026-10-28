#pragma once

#include "Debug.h"

#include "Base/Base.h"
#include "Games/Games.h"

#include "Menu/Armory/Armory.h"
#include "Menu/Depot/Depot.h"
#include "Menu/Editor/Editor.h"
#include "Menu/Failure/Failure.h"
#include "Menu/Main/Main.h"
#include "Menu/Map/Map.h"
#include "Menu/Match/Match.h"
#include "Menu/Result/Result.h"
#include "Menu/Room/Room.h"
#include "Menu/Settings/Settings.h"

#include <QImage>

#include <vector>

DebugInfo(class SelfTest;)

// Menu 模块：只组合 8 个界面组件和进程级资源（Shop、Loadout、关卡表），
// 负责屏幕状态机、输入路由和组件之间的数据传递。
// 和 Arena 一样，组件在 namespace menu 里，Menu 是唯一的模块入口。
// Base 不再由 Menu 拥有：它是进程级单例 BaseService（见 Base/Base.h）。
class Menu {
public:
    enum class Screen {
        Main,
        Map,
        // 房间/大厅：主机与加入者在对局开始前看到的同一块服务端权威界面。
        Room,
        Match,
        Result,
        // 无法连接服务器 / 房间已解散：独立的失败页，不套用对局失败结算。
        Failure,
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
    void render(const QSize &target = QSize(), qreal pixelRatio = 1.0);
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
    // 结算 UI 只在 QML ResultOverlay 中；C++ 只提供只读 Summary。
    menu::ResultSummary resultSummary() const { return summary(); }
    menu::FailureView failureSummary() const { return failureView(); }

private:
    DebugInfo(friend class SelfTest;)

    static constexpr int CustomLevelIndex = 1;

    // 标准战役台本：运行时提交给 Games。
    static games::Script standardScript(int index);
    std::vector<menu::Main::Card> cards() const;
    menu::ResultSummary summary() const;

    void startLevel(int index);
    void startCampaign(int index);
    void startCustom(const menu::Editor::Config &config);
    void retry();
    void retryFailure();
    void returnToMenu();
    void openEditor();
    void enterMatchScreen();

    menu::Room::View roomView() const;
    menu::FailureView failureView() const;

    std::vector<Games::Level> levels_;
    std::vector<Games::Level> campaignLevels_;

    menu::Main main_;
    menu::Map map_;
    menu::Match match_;
    menu::Room room_;
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
    bool offline_ = false;
    QString defaultHost_;
    quint16 defaultPort_ = 0;
    menu::Editor::Config customConfig_;
};
