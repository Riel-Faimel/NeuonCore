#pragma once

#include "Math.h"

#include <QRectF>
#include <QString>

class QPainter;

namespace menu {

// Failure 组件：连接/房间层的独立失败页。
// 与 Result 的区别：这不是“对局失败”，不显示战绩、不冒充空对局；
// 只解释无法连接服务器或房间已解散，并提供重试/返回。
class Failure {
public:
    struct View {
        QString title;
        QString detail;
        QString endpoint;
        bool retryable = true;
    };

    enum class Action {
        None,
        Retry,
        Back,
    };

    void render(QPainter &painter, const View &view) const;
    Action keyPress(int key) const;
    Action mousePress(const Math::Vec2 &position) const;

    QRectF retryRect() const;
    QRectF backRect() const;
};

} // namespace menu
