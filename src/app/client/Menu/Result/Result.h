#pragma once

#include <QString>

namespace menu {

// 结算层的数据模型。真正的 UI 由 src/qml/ResultOverlay.qml 负责；
// C++ 不再维护第二套 Result 绘制/命中测试。
struct ResultSummary {
    bool victory = false;
    QString levelName;
    int score = 0;
    int kills = 0;
    int wave = 0;
    int totalWaves = 0;
    float runTime = 0.0f;
};

} // namespace menu
