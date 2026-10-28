#pragma once

#include <QString>

namespace menu {

// 失败页的数据模型；UI 只由 src/qml/FailureOverlay.qml 负责。
struct FailureView {
    QString title;
    QString detail;
    QString endpoint;
    bool retryable = true;
};

} // namespace menu
