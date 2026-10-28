#pragma once

#include "Math.h"

#include <QRectF>
#include <QString>

#include <vector>

namespace menu {

// Main 组件：标题页。绘制标题、关卡卡、商店和设置入口，只做导航与命中测试，
// 不启动对局、不持有 Games —— 它把玩家意图作为 Command 交回 Menu。
class Main {
public:
    struct Card {
        QString name;
        QString subtitle;
        bool custom = false;
    };

    enum class Action {
        None,
        OpenMap,
        OpenEditor,
        OpenShop,
        OpenArmory,
        OpenSettings,
    };

    struct Command {
        Action action = Action::None;
        int level = 0;
    };

    void update(float dt, const std::vector<Card> &cards,
                const Math::Vec2 &mouse, bool mouseActive);

    Command keyPress(int key, const std::vector<Card> &cards);
    Command mousePress(const Math::Vec2 &position, const std::vector<Card> &cards);

    QRectF levelRect(int index, int cardCount) const;
    QRectF settingsButtonRect() const;
    QRectF shopButtonRect() const;
    QRectF armoryButtonRect() const;
    int selected() const { return selected_; }
    void select(int index) { selected_ = index; }

private:
    int selected_ = 0;
    float time_ = 0.0f;
};

} // namespace menu
