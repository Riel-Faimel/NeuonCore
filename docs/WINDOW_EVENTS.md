# 窗口事件与 QML 覆盖层流程

本文记录 `src/app/client/main.cpp` 中 `GameWindow` 的窗口事件、坐标换算、指针和 QML 覆盖层的实际数据流。它描述当前实现，不是迁移计划。

## 坐标与尺寸

| 对象 | 坐标系 |
| --- | --- |
| 设计画布 | 固定 `1280 × 720` |
| QWidget 窗口 | 逻辑像素，默认 `1024 × 576`，最小 `800 × 450`，可自由缩放 |
| C++ Menu 帧 | 按窗口物理像素渲染：`round(width * dpr) × round(height * dpr)`，再设置 `QImage::devicePixelRatio(dpr)` |
| QML 覆盖层 | `QQuickWidget` 尺寸等于窗口逻辑尺寸；内部 `stage` 以 `min(w/1280, h/720)` 等比缩放 |

C++ 侧鼠标坐标通过 `GameWindow::toGamePos()` 从窗口逻辑坐标换算回 1280×720 设计坐标。QML 侧坐标保持在 1280×720 `stage` 内，由 `stage.scale` 负责显示缩放。

## 事件入口

### `GameWindow::enterEvent`

1. `setCursor(Qt::BlankCursor)`；
2. 将事件位置换算成设计坐标，交给 `Menu::mouseMove()`；
3. `BaseService.pointerMoveTo()`；
4. `BaseService.pointerSetVisible(true)`；
5. `hideGameCursor(true)`：应用级 override 空白光标。

### `GameWindow::leaveEvent`

1. `setCursor(Qt::ArrowCursor)`；
2. `BaseService.pointerSetVisible(false)`；
3. `hideGameCursor(false)` 恢复系统光标。

### `GameWindow::mouseMoveEvent`

只处理窗口直接收到的鼠标移动（没有 QML 覆盖层时）：

```text
窗口 logical pos
  → toGamePos() 1280x720
  → Menu::mouseMove()
  → BaseService.pointerMoveTo()
```

### `GameWindow::mousePressEvent` / `mouseReleaseEvent`

- 左键：`Menu::mousePressLeft()`；
- 右键：`Menu::mousePressRight()`；
- 释放：`Menu::mouseReleaseLeft()`；
- `BaseService.pointerSetVisible(true)`；
- QML 屏幕由 QML `MouseArea` 自己接收点击，不进入这里。

### `GameWindow::keyPressEvent` / `keyReleaseEvent`

- 键盘始终由 `Menu` 处理，QML 覆盖层不抢键盘焦点；
- `QQuickWidget` 设置为 `Qt::NoFocus`；
- `focusOutEvent` 清空 Menu 输入，避免切窗后按键卡住。

### `GameWindow::showEvent`

- 首次显示时按可用屏幕居中；
- 把鼠标位置同步给 Menu/Base；
- 请求窗口焦点。

### `GameWindow::eventFilter`

`QQuickWidget` 会消费鼠标事件，因此它上面的移动不会触发 `GameWindow::mouseMoveEvent`。为避免准星停在 QML 边界：

```text
QQuickWidget MouseMove/HoverMove
  → widget->mapTo(GameWindow, local)
  → toGamePos()
  → BaseService.pointerMoveTo()
  → BaseService.pointerSetVisible(true)
```

`Enter` 设置指针可见，`Leave` 设置不可见。过滤器返回 `false`，事件继续交给 QML `MouseArea`，所以按钮 hover/click 不受影响。

### `GameWindow::paintEvent`

1. `paintEvent` 先绘制 `Menu::frame()`；
2. 如果当前不是 `Screen::Result` / `Screen::Failure`，再绘制 Widgets 准星；
3. QML 屏幕的准星由 QML `Reticle` 绘制，Widgets 准星被跳过，避免双层；
4. QML 覆盖层是 `GameWindow` 的子控件，绘制在父窗口内容之上。

### `GameWindow::tick`

```text
Menu::update(dt)
  → BaseService.pointerUpdate(dt)
  → Menu::render(window physical size, dpr)
  → syncResultOverlay()
  → syncFailureOverlay()
  → update()
```

`sync*Overlay()` 负责：

- 判断当前 `Menu::Screen`；
- 设置显示/隐藏；
- 同步 Result/Failure 数据；
- 同步指针 `pointerX/pointerY/pointerTime/pointerVisible`；
- 读取 QML 的 `retryRequested/backRequested`，转换回 `Menu::keyPress()`；
- 设置 `QQuickWidget` 几何为窗口 `rect()`。

## QML 覆盖层

### Result

- 文件：`src/qml/ResultOverlay.qml`；
- 显示条件：`Menu::Screen::Result`；
- 数据源：`Menu::resultSummary()`；
- 视觉：半透明暗层、强调线、六向 glow、四列统计、按钮；
- 指针：`src/qml/Reticle.qml`，最后声明并 `z: 1000`，画在按钮之上；
- 按钮：QML `MouseArea`，设置 `retryRequested/backRequested`，由 C++ 下帧转成 `Menu` 输入。

### Failure

- 文件：`src/qml/FailureOverlay.qml`；
- 显示条件：`Menu::Screen::Failure`；
- 数据源：`Menu::failureSummary()`；
- 指针和按钮流程与 Result 相同。

### 光标

系统箭头被两层隐藏：

1. `QQuickWidget::setCursor(Qt::BlankCursor)`；
2. 所有 QML `MouseArea` 的 `cursorShape: Qt.BlankCursor`；
3. 进入窗口时 `QGuiApplication::setOverrideCursor(Qt::BlankCursor)`，离开窗口时恢复。

因此准星进入 QML 按钮内部时仍是自定义准星，不会在按钮边界被系统箭头替换。

## 缩放流程

窗口没有固定尺寸。用户拖动窗口边缘时：

```text
QWidget resize
  → tick 中 Menu::render() 使用新的物理尺寸
  → QQuickWidget::setGeometry(rect())
  → QML stage.scale 更新
  → 下一帧绘制
```

C++ Menu 的 `QImage` 以物理像素重新栅格化，因此菜单文字不会先以 1280 渲染再缩小。QML 文本使用 `Text.NativeRendering`，减少缩放模糊。

## 已知边界

- 只有 Result/Failure 是 QML-only；Main/Map/Room/Editor/Depot/Armory/Settings/Match HUD 仍是 C++ Widgets。
- QML 和 QPainter 的字体栅格化不保证逐像素一致。
- QML 覆盖层的鼠标事件由 QML 处理；C++ 只镜像指针位置和按钮请求。
