#pragma once

#include "Math.h"

#include <QRectF>

class Base;
class Inventory;
class QPainter;
class Score;

namespace menu {

// Shop 组件：局外补给站。目前只卖“震荡地雷”（一次性消耗品），
// 价格/伤害来自 src/json/items.json，购买从全局积分扣款并写入库存。
class Shop {
public:
    enum class Action {
        None,
        Back,
    };

    void reset();
    Action keyPress(int key, Score &score, Inventory &inventory, Base &base);
    Action mousePress(const Math::Vec2 &position, Score &score, Inventory &inventory,
                      Base &base);
    void render(QPainter &painter, const Score &score, const Inventory &inventory) const;

    QRectF itemRect() const;
    QRectF buyRect() const;
    QRectF backRect() const;

private:
    bool buy(Score &score, Inventory &inventory, Base &base);

    int purchases_ = 0;
};

} // namespace menu
