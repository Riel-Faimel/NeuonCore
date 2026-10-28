#include "Menu/Menu.h"

#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"
#include "Data/GameData.h"
#include "Net/Client.h"

#include <QPainter>
#include <Qt>

#include <utility>

Menu::Menu(bool enableAudio, bool offline, const QString &host, quint16 port)
    : settings_(enableAudio)
{
    // 绘制资源（字体/颜色）由 main() 在 QApplication 之后统一初始化。
    // Shop 也是进程级单例，由 main() 负责 load()/save()。
    audioRequested_ = enableAudio;
    offline_ = offline;
    defaultHost_ = host;
    defaultPort_ = port;
    match_.configure(offline ? menu::Match::Mode::Local : menu::Match::Mode::Network,
                     host, port);
    if (enableAudio) {
        BaseService.initAudio();
        settings_.load();
    }
    frame_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    levels_ = {
        {QStringLiteral("标准协议"),
         QStringLiteral("星区地图 · 四个区域"), 1, standardScript(0)},
        {QStringLiteral("自定义协议"),
         QStringLiteral("点击编辑台本"), 1, editor_.script()},
    };
    for (int i = 0; i < int(data::kCampaigns.size())
                    && i < data::kLevelNodes.count; ++i) {
        const data::LevelNode &node = data::kLevelNodes.items[size_t(i)];
        Games::Level level;
        level.name = QString::fromUtf8(node.name.data(), int(node.name.size()));
        level.subtitle = QString::fromUtf8(node.subtitle.data(),
                                           int(node.subtitle.size()));
        level.startWave = 1;
        level.enemyLevel = i + 1;
        level.script = standardScript(i);
        campaignLevels_.push_back(std::move(level));
    }
}

Menu::~Menu() = default;

// 台本是数据：src/json/waves/level-0N.json 的 campaign 在编译期展开成常量，
// 这里只是把它转成这一局要提交给 Games 的 games::Script。
games::Script Menu::standardScript(int index)
{
    games::Script script;
    if (index < 0 || index >= int(data::kCampaigns.size())) {
        return script;
    }
    const data::Campaign &campaign = data::kCampaigns[size_t(index)];
    script.reserve(size_t(campaign.count));
    for (int w = 0; w < campaign.count; ++w) {
        const data::WaveDef &definition = campaign.waves[size_t(w)];
        games::Wave wave;
        wave.enemies.reserve(size_t(definition.count));
        for (int i = 0; i < definition.count; ++i) {
            const data::WaveSpawn &spawn = definition.spawns[size_t(i)];
            const auto type = arena::Enemy::typeForKey(
                QString::fromUtf8(spawn.kind.data(), int(spawn.kind.size())));
            if (type) {
                wave.enemies.push_back({*type, spawn.count});
            }
        }
        script.push_back(wave);
    }
    return script;
}

std::vector<menu::Main::Card> Menu::cards() const
{
    std::vector<menu::Main::Card> result;
    result.reserve(levels_.size());
    for (size_t i = 0; i < levels_.size(); ++i) {
        menu::Main::Card card;
        card.name = levels_[i].name;
        card.custom = int(i) == CustomLevelIndex;
        card.subtitle = card.custom
                            ? QStringLiteral("%1 波 · %2 个目标")
                                  .arg(editor_.waveCount())
                                  .arg(editor_.totalEnemies())
                            : levels_[i].subtitle;
        result.push_back(card);
    }
    return result;
}

menu::ResultSummary Menu::summary() const
{
    menu::ResultSummary summary;
    summary.victory = match_.state() == Games::State::Victory;
    summary.levelName =
        campaignActive_ && activeCampaign_ >= 0
                && activeCampaign_ < int(campaignLevels_.size())
            ? campaignLevels_[size_t(activeCampaign_)].name
            : (level_ >= 0 && level_ < int(levels_.size())
                   ? levels_[size_t(level_)].name
                   : QString());
    summary.score = match_.runScore();
    summary.kills = match_.kills();
    summary.wave = match_.wave();
    summary.totalWaves = match_.totalWaves();
    summary.runTime = match_.runTime();
    return summary;
}

const QImage &Menu::frame() const
{
    if (screen_ == Screen::Match && match_.active()) {
        return match_.frame();
    }
    return frame_;
}

menu::Room::View Menu::roomView() const
{
    menu::Room::View view;
    view.host = match_.roomHost();
    view.youAreHost = match_.roomYouAreHost();
    view.connected = match_.roomConnected();
    view.playing = match_.roomPlaying();
    view.canStart = view.host && view.connected && !view.playing;
    view.roomName = match_.roomName();
    view.endpoint = match_.roomEndpoint();
    view.players = match_.roomPlayers();
    view.maxPlayers = match_.roomMaxPlayers();
    if (const client::GameClient *gameClient = match_.gameClient()) {
        const auto &rooms = gameClient->rooms();
        if (!rooms.empty()) {
            for (const net::RoomPlayerInfo &member : rooms.front().members) {
                menu::Room::PlayerSlot slot;
                slot.host = member.host;
                slot.you = member.sessionId == gameClient->sessionId();
                slot.inMatch = member.inMatch;
                slot.name = QStringLiteral("P%1 / %2")
                                .arg(member.playerId)
                                .arg(member.host ? QStringLiteral("房主 HOST")
                                                 : QStringLiteral("玩家 PLAYER"));
                if (slot.you) {
                    slot.detail = member.inMatch
                                      ? QStringLiteral("本机 · 正在对局")
                                      : QStringLiteral("本机 · 等待房主开始");
                } else if (member.inMatch) {
                    slot.detail = QStringLiteral("对局中");
                } else {
                    slot.detail = QStringLiteral("等待房主开始对局");
                }
                view.playerSlots.push_back(std::move(slot));
            }
            // 人数以服务端成员列表为准（与 RoomList 的 players 字段一致）。
            if (int(view.playerSlots.size()) != view.players) {
                view.players = int(view.playerSlots.size());
            }
        }
    }
    if (!view.connected) {
        view.status = QStringLiteral("正在连接主机，等待房间状态…");
    } else if (view.playing) {
        view.status = QStringLiteral("房主已开始对局，正在同步战场…");
    } else if (view.host) {
        view.status = QStringLiteral("房间已建立，等待你按 ENTER 开始对局");
    } else {
        view.status = QStringLiteral("你已加入房间，等待房主开始对局");
    }
    if (view.host && !campaignActive_) {
        view.scriptSummary = QStringLiteral("台本 %1 波 · %2 个敌方目标")
                                 .arg(editor_.waveCount())
                                 .arg(editor_.totalEnemies());
    } else if (campaignActive_ && activeCampaign_ >= 0
               && activeCampaign_ < int(campaignLevels_.size())) {
        view.scriptSummary = QStringLiteral("%1 · 等待房主开始")
                                 .arg(campaignLevels_[size_t(activeCampaign_)].name);
    } else {
        view.scriptSummary = QStringLiteral("台本由主机同步 · 对局开始后进入战斗");
    }
    return view;
}

menu::FailureView Menu::failureView() const
{
    menu::FailureView view;
    view.title = match_.failureTitle();
    view.detail = match_.failureDetail();
    view.endpoint = match_.failureEndpoint();
    view.retryable = true;
    return view;
}

void Menu::update(float dt)
{
    if (screen_ == Screen::Room && match_.active()) {
        match_.update(dt);
        if (match_.failed()) {
            screen_ = Screen::Failure;
        } else if (!match_.inRoom()) {
            screen_ = Screen::Match;
        }
        return;
    }
    if (screen_ == Screen::Match && match_.active()) {
        match_.update(dt);
        if (match_.failed()) {
            // 连接/房间层失败是独立页面，不进入对局结算。
            screen_ = Screen::Failure;
        } else if (match_.inRoom()) {
            screen_ = Screen::Room;
        } else if (match_.finished()) {
            screen_ = Screen::Result;
        }
        return;
    }
    if (screen_ == Screen::Result && match_.active()) {
        match_.update(dt);
        if (match_.failed()) {
            screen_ = Screen::Failure;
        } else if (match_.inRoom()) {
            screen_ = Screen::Room;
        }
        return;
    }
    if (screen_ == Screen::Failure) {
        return;
    }
    if (screen_ == Screen::Main) {
        main_.update(dt, cards(), mouse_, mouseActive_);
    } else if (screen_ == Screen::Map) {
        map_.update(dt, mouse_, mouseActive_);
    } else if (screen_ == Screen::Editor) {
        editor_.update(dt);
    }
}

void Menu::render(const QSize &target, qreal pixelRatio)
{
    const QSize size = target.isValid() ? target
                                         : QSize(GameConfig::ScreenWidth,
                                                 GameConfig::ScreenHeight);
    const qreal sx = qreal(size.width()) / GameConfig::ScreenWidth;
    const qreal sy = qreal(size.height()) / GameConfig::ScreenHeight;
    auto prepare = [&]() {
        if (frame_.size() != size) {
            frame_ = QImage(size, QImage::Format_ARGB32_Premultiplied);
        }
        // 绘制期间用 1:1 像素坐标，画完再交给 Qt 按 DPR 显示。
        frame_.setDevicePixelRatio(1.0);
        frame_.fill(Qt::transparent);
    };
    auto finish = [&]() { frame_.setDevicePixelRatio(pixelRatio); };

    if (screen_ == Screen::Room && match_.active()) {
        prepare();
        {
            QPainter painter(&frame_);
            painter.scale(sx, sy);
            room_.render(painter, roomView());
        }
        finish();
        return;
    }
    if (screen_ == Screen::Match && match_.active()) {
        match_.render();
        return;
    }
    if (screen_ == Screen::Failure) {
        // 失败页 UI 由 GameWindow 的 QML FailureOverlay 负责。
        return;
    }
    if (screen_ == Screen::Result && match_.active()) {
        // 结算 UI 由 GameWindow 的 QML ResultOverlay 负责；Widgets 只保留底层对局帧。
        match_.render();
        return;
    }
    prepare();
    {
        QPainter painter(&frame_);
        painter.scale(sx, sy);
        if (screen_ == Screen::Map) {
            map_.render(painter);
        } else if (screen_ == Screen::Editor) {
            editor_.render(painter, BaseService.muted());
        } else if (screen_ == Screen::Shop) {
            depot_.render(painter);
        } else if (screen_ == Screen::Armory) {
            armory_.render(painter);
        } else if (screen_ == Screen::Settings) {
            settings_.render(painter);
        } else {
            main_.render(painter, cards(), mouse_, mouseActive_, BaseService.muted(),
                         audioRequested_ && !BaseService.audioEnabled());
        }
    }
    finish();
}

void Menu::startCampaign(int index)
{
    if (index < 0 || index >= int(campaignLevels_.size())) {
        return;
    }
    // 标准协议永远走默认路径：不读自定义服务器配置。
    match_.configure(offline_ ? menu::Match::Mode::Local : menu::Match::Mode::Network,
                     defaultHost_, defaultPort_);
    activeCampaign_ = index;
    campaignActive_ = true;
    match_.start(campaignLevels_[size_t(index)]);
    enterMatchScreen();
    BaseService.keyRelease(Qt::Key_Escape);
}

void Menu::startCustom(const menu::Editor::Config &config)
{
    customConfig_ = config;
    campaignActive_ = false;
    level_ = CustomLevelIndex;
    Games::Level level = levels_[size_t(CustomLevelIndex)];
    level.script = editor_.script();
    level.startWave = 1;

    if (config.mode == menu::Editor::Mode::Single) {
        // 单机自定义：不开放端口，使用本地直连路径，保持直接开局。
        match_.configure(menu::Match::Mode::Local);
    } else if (config.mode == menu::Editor::Mode::Host) {
        // 本机开房：启动内嵌服务端并进入房间，不立即 startMatch。
        match_.configure(menu::Match::Mode::Host, QString(), config.port);
    } else {
        // 加入已有主机：只连接并进入房间，开局权在房主。
        match_.configure(menu::Match::Mode::Network, config.host, config.port);
    }

    match_.start(level);
    enterMatchScreen();
    BaseService.keyRelease(Qt::Key_Escape);
}

void Menu::enterMatchScreen()
{
    if (!match_.active()) {
        screen_ = Screen::Main;
        return;
    }
    if (match_.failed()) {
        screen_ = Screen::Failure;
        return;
    }
    screen_ = match_.inRoom() ? Screen::Room : Screen::Match;
}

void Menu::startLevel(int index)
{
    if (index == 0) {
        startCampaign(0);
        return;
    }
    if (index < 0 || index >= int(levels_.size())) {
        return;
    }
    if (index == CustomLevelIndex) {
        // 直接进入自定义关卡时等价于“单机自定义”；编辑器部署会先走联机设置。
        startCustom(menu::Editor::Config{});
        return;
    }
    campaignActive_ = false;
    level_ = index;
    match_.start(levels_[size_t(index)]);
    screen_ = Screen::Match;
    BaseService.keyRelease(Qt::Key_Escape);
}

void Menu::retry()
{
    // 房主在结果页“重新部署”先回到房间，由房主再次 ENTER 开局；
    // 客户端收到 enterRoom 的 RoomList 后从结算页回到房间。
    if (match_.roomHost() && match_.returnToRoom()) {
        screen_ = Screen::Room;
        BaseService.keyRelease(Qt::Key_Escape);
        return;
    }
    if (match_.restartInPlace()) {
        screen_ = Screen::Match;
        BaseService.keyRelease(Qt::Key_Escape);
        return;
    }
    if (campaignActive_) {
        startCampaign(activeCampaign_);
    } else if (level_ == CustomLevelIndex) {
        startCustom(customConfig_);
    } else {
        startLevel(level_);
    }
}

void Menu::retryFailure()
{
    if (!match_.active()) {
        returnToMenu();
        return;
    }
    match_.restart();
    enterMatchScreen();
    BaseService.keyRelease(Qt::Key_Escape);
}

void Menu::returnToMenu()
{
    screen_ = Screen::Main;
    if (match_.active() && match_.roomHost()) {
        // 房主离开标题必须显式解散：通知已连接客户端，而不是直接析构让它们看到空房间。
        match_.dissolveRoom();
    }
    match_.stop();
    BaseService.clear();
}

void Menu::openEditor()
{
    editor_.prepare();
    screen_ = Screen::Editor;
    BaseService.playSound(QStringLiteral("ui"));
}

void Menu::keyPress(int key, bool autoRepeat)
{
    if (key == Qt::Key_M && !autoRepeat) {
        toggleMute();
        return;
    }
    switch (screen_) {
    case Screen::Main: {
        const menu::Main::Command command = main_.keyPress(key, cards());
        switch (command.action) {
        case menu::Main::Action::OpenMap:
            map_.reset();
            screen_ = Screen::Map;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenEditor:
            openEditor();
            break;
        case menu::Main::Action::OpenShop:
            screen_ = Screen::Shop;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenArmory:
            armory_.open();
            screen_ = Screen::Armory;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenSettings:
            settings_.reset();
            screen_ = Screen::Settings;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::None:
            break;
        }
        break;
    }
    case Screen::Map: {
        const menu::Map::Command command = map_.keyPress(key);
        if (command.action == menu::Map::Action::Start) {
            if (command.level >= 0 && command.level < int(campaignLevels_.size())) {
                startCampaign(command.level);
            }
        } else if (command.action == menu::Map::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Room: {
        const menu::Room::Action action =
            room_.keyPress(key, autoRepeat, roomView());
        if (action == menu::Room::Action::Start) {
            match_.startRoomMatch();
            BaseService.playSound(QStringLiteral("ui"));
        } else if (action == menu::Room::Action::Dissolve) {
            // 房主解散：通知已连接客户端，然后关闭内嵌服务端。
            match_.dissolveRoom();
            returnToMenu();
        } else if (action == menu::Room::Action::Back) {
            if (match_.roomHost()) {
                match_.dissolveRoom();
            } else {
                match_.leaveRoom();
            }
            returnToMenu();
        }
        break;
    }
    case Screen::Failure: {
        if ((key == Qt::Key_R || key == Qt::Key_Return || key == Qt::Key_Enter)
            && failureView().retryable) {
            retryFailure();
        } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
            returnToMenu();
        }
        break;
    }
    case Screen::Match:
        if (!match_.active()) {
            break;
        }
        if (match_.lobby()) {
            screen_ = Screen::Room;
            break;
        }
        match_.keyPress(key, autoRepeat);
        if (key == Qt::Key_Q && match_.state() == Games::State::Paused) {
            returnToMenu();
        }
        break;
    case Screen::Result: {
        if (key == Qt::Key_R || key == Qt::Key_Return || key == Qt::Key_Enter) {
            retry();
        } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
            returnToMenu();
        }
        break;
    }
    case Screen::Editor: {
        const menu::Editor::Action action = editor_.keyPress(key, autoRepeat);
        if (action == menu::Editor::Action::Deploy) {
            startCustom(editor_.config());
        } else if (action == menu::Editor::Action::Back) {
            returnToMenu();
        }
        break;
    }
    case Screen::Shop: {
        const menu::Depot::Action action = depot_.keyPress(key);
        if (action == menu::Depot::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Armory: {
        if (armory_.keyPress(key) == menu::Armory::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Settings: {
        const menu::Settings::Action action = settings_.keyPress(key);
        if (action == menu::Settings::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    }
}

void Menu::keyRelease(int key)
{
    if (screen_ == Screen::Match) {
        match_.keyRelease(key);
    } else if (screen_ == Screen::Editor) {
        editor_.keyRelease(key);
    }
}

void Menu::mouseMove(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    if (screen_ == Screen::Room) {
        room_.mouseMove(position);
    } else if (screen_ == Screen::Match) {
        match_.mouseMove(position);
    } else if (screen_ == Screen::Settings) {
        settings_.mouseMove(position);
    } else if (screen_ == Screen::Editor) {
        editor_.mouseMove(position);
    }
}

void Menu::mousePressLeft(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    switch (screen_) {
    case Screen::Main: {
        const menu::Main::Command command = main_.mousePress(position, cards());
        switch (command.action) {
        case menu::Main::Action::OpenMap:
            map_.reset();
            screen_ = Screen::Map;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenEditor:
            openEditor();
            break;
        case menu::Main::Action::OpenShop:
            screen_ = Screen::Shop;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenArmory:
            armory_.open();
            screen_ = Screen::Armory;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenSettings:
            settings_.reset();
            screen_ = Screen::Settings;
            BaseService.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::None:
            break;
        }
        break;
    }
    case Screen::Map: {
        const menu::Map::Command command = map_.mousePress(position);
        if (command.action == menu::Map::Action::Start) {
            if (command.level >= 0 && command.level < int(campaignLevels_.size())) {
                startCampaign(command.level);
            }
        } else if (command.action == menu::Map::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Room: {
        const menu::Room::Action action = room_.mousePress(position, roomView());
        if (action == menu::Room::Action::Start) {
            match_.startRoomMatch();
            BaseService.playSound(QStringLiteral("ui"));
        } else if (action == menu::Room::Action::Dissolve) {
            match_.dissolveRoom();
            returnToMenu();
        } else if (action == menu::Room::Action::Back) {
            if (match_.roomHost()) {
                match_.dissolveRoom();
            } else {
                match_.leaveRoom();
            }
            returnToMenu();
        }
        break;
    }
    case Screen::Failure:
        // 失败页按钮只在 QML FailureOverlay 中。
        break;
    case Screen::Match:
        if (match_.lobby()) {
            screen_ = Screen::Room;
            break;
        }
        match_.mousePressLeft(position);
        break;
    case Screen::Result:
        // 结算按钮只在 QML ResultOverlay 中；Widgets 不再维护第二套命中区域。
        break;
    case Screen::Editor: {
        const menu::Editor::Action action = editor_.mousePress(position);
        if (action == menu::Editor::Action::Deploy) {
            startCustom(editor_.config());
        } else if (action == menu::Editor::Action::Back) {
            returnToMenu();
        }
        break;
    }
    case Screen::Shop: {
        const menu::Depot::Action action =
            depot_.mousePress(position);
        if (action == menu::Depot::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Armory: {
        if (armory_.mousePress(position) == menu::Armory::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Settings: {
        const menu::Settings::Action action = settings_.mousePress(position);
        if (action == menu::Settings::Action::Back) {
            screen_ = Screen::Main;
            BaseService.playSound(QStringLiteral("ui"));
        }
        break;
    }
    }
}

void Menu::mousePressRight(const Math::Vec2 &position)
{
    if (screen_ == Screen::Match && match_.active()) {
        match_.mousePressRight(position);
    }
}

void Menu::mouseReleaseLeft()
{
    settings_.mouseRelease();
    if (screen_ == Screen::Match) {
        match_.mouseReleaseLeft();
    } else if (screen_ == Screen::Editor) {
        editor_.mouseRelease();
    }
}

void Menu::clearInput()
{
    BaseService.clear();
}

void Menu::toggleMute()
{
    if (!BaseService.toggleMute()) {
        BaseService.playSound(QStringLiteral("ui"));
    }
}
