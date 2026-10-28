#include "Menu/Armory/Armory.h"

#include "Base/Base.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Loadout/Loadout.h"

#include <Qt>

#include <algorithm>

namespace menu {

namespace {

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), int(text.size()));
}

} // namespace

void Armory::open()
{
    selected_ = LoadoutService.selected();
}

QRectF Armory::cardRect(int index) const
{
    constexpr float width = 290.0f;
    constexpr float gap = 28.0f;
    const float total = width * float(data::kLaunchers.count)
                        + gap * float(std::max(0, data::kLaunchers.count - 1));
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + float(index) * (width + gap), 214.0f, width, 314.0f);
}

QRectF Armory::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 90.0f, 600.0f, 180.0f, 42.0f);
}

Armory::Action Armory::keyPress(int key)
{
    if (data::kLaunchers.count <= 0) {
        return key == Qt::Key_Escape || key == Qt::Key_Q ? Action::Back
                                                         : Action::None;
    }
    if (key == Qt::Key_Left || key == Qt::Key_A) {
        selected_ = std::max(0, selected_ - 1);
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        selected_ = std::min(data::kLaunchers.count - 1, selected_ + 1);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter
               || key == Qt::Key_Space) {
        LoadoutService.select(selected_);
        BaseService.playSound(QStringLiteral("upgrade"));
    } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    return Action::None;
}

Armory::Action Armory::mousePress(const Math::Vec2 &position)
{
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    for (int i = 0; i < data::kLaunchers.count; ++i) {
        if (cardRect(i).contains(point)) {
            selected_ = i;
            LoadoutService.select(i);
            BaseService.playSound(QStringLiteral("upgrade"));
            break;
        }
    }
    return Action::None;
}

} // namespace menu
