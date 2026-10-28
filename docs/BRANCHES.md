# 分支维护：main（QML）与 widgets-only

项目现在维护两条长期线：

| 分支 | UI | 目的 |
| --- | --- | --- |
| `main` | QML（Main / Settings / Shop / Armory / Result / Failure）+ Widgets（Map / Room / Editor / Match HUD） | 主开发线 |
| `widgets-only` | 全部 Widgets | 稳定回退线，保留原 C++ Menu 绘制 |

## 分支关系

```text
97ee51a  refactor: gate test hooks behind DebugInfo
   │
   ├─ widgets-only
   │    └─ 97ee51a + gameplay/docs cherry-picks（当前 9e1265a）
   │
   └─ main
        └─ QML Result/Failure/Main/Settings/Shop/Armory
           + gameplay/docs
```

`widgets-only` 不是 `main` 的祖先状态备份，而是**独立维护的回退线**。不要在两条线之间直接 merge：

- `main` → `widgets-only`：会把 QML 覆盖层带进来，破坏 Widgets-only；
- `widgets-only` → `main`：会把旧的 C++ 绘制路径重新带回来。

共享玩法（Player / Arena / Upgrades / Net / 服务端 / 测试）在 `main` 提交后，用 cherry-pick 同步到 `widgets-only`。

## 当前差异

### main

- 新增 `src/qml/`：Main/Settings/Depot/Armory/Result/Failure 的 QML-only UI；
- 这些界面的 C++ `render()` 已删除，只保留数据、几何和输入逻辑；
- QML 覆盖层负责准星、半透明覆盖和 QML 按钮；
- 当前 smoke：**319**。

### widgets-only

- 没有 `src/qml/`；
- Main/Settings/Depot/Armory/Result/Failure 继续由 C++ QPainter 绘制；
- 保留旧 Widgets 窗口/准星路径；
- 当前 smoke：**317**；
- 已包含霰弹“三发一波、分裂加整波”的玩法修复。

## 日常流程

### 只改玩法（两条线都要）

```bash
git switch main
# 修改 Player / Arena / Upgrades / Net / Server / tests
mingw32-make -j12 test
./build/NeonCoreConsole.exe --smoke-test
git commit -m "..."

# 同步到 Widgets 回退线
git switch widgets-only
git cherry-pick <main-commit>
mingw32-make -j12 test
./build/NeonCoreConsole.exe --smoke-test
git switch main
```

冲突处理原则：

- 玩法文件按 `main` 的新实现解决；
- 不要为了通过编译把 QML 文件带进 `widgets-only`；
- `widgets-only` 的 UI 渲染文件保持旧版本。

### 只改 QML / 新界面

只提交到 `main`，不要 cherry-pick 到 `widgets-only`：

```text
src/qml/*
src/app/client/main.cpp 的 QML overlay 部分
Menu 的 QML bridge 方法
```

### 只改 Widgets 回退线

如果只是修复旧 C++ 绘制：

```bash
git switch widgets-only
# 修改 Main/Settings/Depot/Armory/Result/Failure 的 render
git commit
```

不要反向合并到 `main`；`main` 对应界面已经是 QML。

## 发布检查

### main

```bash
mingw32-make -j12 test
build\NeonCoreConsole.exe --smoke-test
mingw32-make -j12 deploy
```

检查 QML 文件已复制到 `dist/NeonCore/qml/`：

```text
MainOverlay.qml
SettingsOverlay.qml
DepotOverlay.qml
ArmoryOverlay.qml
ResultOverlay.qml
FailureOverlay.qml
NeonText.qml
MainEntry.qml
Reticle.qml
```

### widgets-only

```bash
git switch widgets-only
mingw32-make -j12 test
build\NeonCoreConsole.exe --smoke-test
mingw32-make -j12 deploy
```

该分支不依赖 QML 运行库，但仍沿用同一个 `Makefile` 的 Widgets 构建路径。

## 提交信息约定

- `fix: ...` / `feat: ...`：共享玩法，允许两条线 cherry-pick；
- `ui(qml): ...` / `style(qml): ...`：只进 `main`；
- `ui(widgets): ...`：只进 `widgets-only`；
- 文档更新可以两条线各自提交，也可以只更新 `main` 的 `docs/`。

## 远端

`widgets-only` 目前是本地分支。需要长期共享时：

```bash
git push -u origin widgets-only
```

之后两条线都从 `origin` 拉取，但不要互相 merge；用 cherry-pick 保持共享玩法同步。
