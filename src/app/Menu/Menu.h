#pragma once

#include "Base/Base.h"
#include "Games/Games.h"

#include <QImage>
#include <QRectF>

#include <memory>
#include <vector>

class SelfTest;

// Menu 模块：主菜单、关卡选择、台本编辑、对局信息与结算界面。
// 它持有进程级的 Base 服务与关卡表，并在开局时构造一局 Games；
// Games 只活一局，返回主菜单即销毁，下一局重新构造。
class Menu {
public:
    enum class Screen {
        Main,
        Match,
        Result,
        Editor,
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

    static constexpr int CustomLevelIndex = 1;
    static constexpr int MaxCustomWaves = 12;
    static constexpr int MaxEnemyCount = 20;
    static constexpr int EditorColumns = 5;   // 四种敌人 + BOSS

    static constexpr float EditorLeft = 90.0f;
    static constexpr float EditorRight = GameConfig::ScreenWidth - 90.0f;
    static constexpr float EditorWaveColumn = 88.0f;
    static constexpr float EditorHeaderY = 238.0f;
    static constexpr float EditorRowsTop = 256.0f;
    static constexpr float EditorRowHeight = 30.0f;

    void startLevel(int index);
    void returnToMenu();
    void drawMain();
    void drawResult();
    void drawEditor();

    // 默认战役台本：运行时提交给 Games，Menu 只负责写数据和选起点。
    static games::Script standardScript();
    // 自定义模式的初始台本：只有一波，波数由玩家在编辑器里定义。
    static games::Script defaultCustomScript();
    bool isCustomLevel(int index) const { return index == CustomLevelIndex; }
    QRectF levelRect(int index) const;
    QRectF retryRect() const;
    QRectF menuRect() const;

    // 台本编辑
    void openEditor();
    static QString enemyKind(int column);
    int customWaveCount() const { return int(customScript_.size()); }
    int enemyCount(int row, int column) const;
    void setEnemyCount(int row, int column, int count);
    bool waveHasBoss(int row) const;
    void setWaveBoss(int row, bool on);
    void adjustEditorCell(int delta);
    void addCustomWave();
    void removeCustomWave();
    int totalCustomEnemies() const;
    QRectF editorRowRect(int row) const;
    QRectF editorCellRect(int row, int column) const;
    QRectF editorMinusRect() const;
    QRectF editorPlusRect() const;
    QRectF editorAddWaveRect() const;
    QRectF editorRemoveWaveRect() const;
    QRectF editorDeployRect() const;
    QRectF editorBackRect() const;

    Base base_;
    std::unique_ptr<Games> games_;
    std::vector<Games::Level> levels_;
    games::Script customScript_;
    Screen screen_ = Screen::Main;
    QImage frame_;
    Math::Vec2 mouse_;
    int selected_ = 0;
    int editorRow_ = 0;
    int editorColumn_ = 0;
    float menuTime_ = 0.0f;
    bool mouseActive_ = false;
    bool audioRequested_ = false;
};
