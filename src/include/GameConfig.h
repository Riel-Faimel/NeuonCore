#pragma once

// 游戏级常量。所有模块共享的屏幕/战场尺寸只在这里定义一次。
class GameConfig {
public:
    static constexpr int ScreenWidth = 1280;
    static constexpr int ScreenHeight = 720;

    static constexpr float FieldLeft = 26.0f;
    static constexpr float FieldRight = ScreenWidth - 26.0f;
    static constexpr float FieldTop = 114.0f;
    static constexpr float FieldBottom = ScreenHeight - 42.0f;

private:
    GameConfig() = delete;
};
