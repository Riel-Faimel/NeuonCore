#include "Menu/Main/Main.h"

#include "Base/Base.h"
#include "GameConfig.h"

#include <QLinearGradient>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <cmath>

namespace menu {

void Main::update(float dt, const std::vector<Card> &cards,
                  const Math::Vec2 &mouse, bool mouseActive)
{
    time_ += dt;
    if (!mouseActive || cards.empty()) {
        return;
    }
    for (int i = 0; i < int(cards.size()); ++i) {
        if (levelRect(i, int(cards.size())).contains(QPointF(mouse.x, mouse.y))) {
            selected_ = i;
        }
    }
}

QRectF Main::levelRect(int index, int cardCount) const
{
    constexpr float width = 360.0f;
    constexpr float height = 156.0f;
    constexpr float gap = 26.0f;
    const int count = std::max(1, cardCount);
    const float total = width * float(count) + gap * float(count - 1);
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + index * (width + gap), 402.0f, width, height);
}

QRectF Main::settingsButtonRect() const
{
    return QRectF(GameConfig::ScreenWidth - 152.0f, 26.0f, 122.0f, 38.0f);
}

QRectF Main::shopButtonRect() const
{
    return QRectF(GameConfig::ScreenWidth - 286.0f, 26.0f, 122.0f, 38.0f);
}

QRectF Main::armoryButtonRect() const
{
    return QRectF(GameConfig::ScreenWidth - 420.0f, 26.0f, 122.0f, 38.0f);
}

Main::Command Main::keyPress(int key, const std::vector<Card> &cards)
{
    Command command;
    const int count = int(cards.size());
    if (count == 0) {
        return command;
    }
    if (key == Qt::Key_Up || key == Qt::Key_W || key == Qt::Key_Left) {
        selected_ = std::max(0, selected_ - 1);
    } else if (key == Qt::Key_Down || key == Qt::Key_S || key == Qt::Key_Right) {
        selected_ = std::min(count - 1, selected_ + 1);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        if (cards[size_t(selected_)].custom) {
            command.action = Action::OpenEditor;
        } else {
            command.action = Action::OpenMap;
        }
    } else if (key == Qt::Key_1) {
        command.action = Action::OpenMap;
    } else if (key == Qt::Key_2) {
        command.action = Action::OpenEditor;
    } else if (key == Qt::Key_3) {
        command.action = Action::OpenArmory;
    }
    return command;
}

Main::Command Main::mousePress(const Math::Vec2 &position,
                               const std::vector<Card> &cards)
{
    Command command;
    const QPointF point(position.x, position.y);
    if (armoryButtonRect().contains(point)) {
        command.action = Action::OpenArmory;
        return command;
    }
    if (shopButtonRect().contains(point)) {
        command.action = Action::OpenShop;
        return command;
    }
    if (settingsButtonRect().contains(point)) {
        command.action = Action::OpenSettings;
        return command;
    }
    const int count = int(cards.size());
    for (int i = 0; i < count; ++i) {
        if (!levelRect(i, count).contains(point)) {
            continue;
        }
        selected_ = i;
        command.level = i;
        command.action = cards[size_t(i)].custom ? Action::OpenEditor
                                                 : Action::OpenMap;
        return command;
    }
    return command;
}

} // namespace menu
