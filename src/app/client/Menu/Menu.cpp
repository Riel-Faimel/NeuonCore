#include "Menu/Menu.h"

#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"
#include "Data/GameData.h"

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

menu::Result::Summary Menu::summary() const
{
    menu::Result::Summary summary;
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
    return screen_ == Screen::Match && match_.active() ? match_.frame() : frame_;
}

void Menu::update(float dt)
{
    if (screen_ == Screen::Match && match_.active()) {
        match_.update(dt);
        if (match_.finished()) {
            screen_ = Screen::Result;
        }
        return;
    }
    if (screen_ == Screen::Result && match_.active()) {
        match_.update(dt);
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

void Menu::render()
{
    if (screen_ == Screen::Match && match_.active()) {
        match_.render();
        return;
    }
    if (screen_ == Screen::Result && match_.active()) {
        match_.render();
        frame_ = match_.frame().copy();
        QPainter painter(&frame_);
        result_.render(painter, summary());
        return;
    }
    QPainter painter(&frame_);
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
    screen_ = Screen::Match;
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
        // 单机自定义：不开放端口，使用本地直连路径。
        match_.configure(menu::Match::Mode::Local);
    } else if (config.mode == menu::Editor::Mode::Host) {
        // 本机开房：port 非 0 让 Match 在 0.0.0.0 上启动内嵌服务端。
        match_.configure(menu::Match::Mode::Network, QString(), config.port);
    } else {
        // 加入已有主机。
        match_.configure(menu::Match::Mode::Network, config.host, config.port);
    }

    match_.start(level);
    screen_ = Screen::Match;
    BaseService.keyRelease(Qt::Key_Escape);
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
    if (campaignActive_) {
        startCampaign(activeCampaign_);
    } else if (level_ == CustomLevelIndex) {
        startCustom(customConfig_);
    } else {
        startLevel(level_);
    }
}

void Menu::returnToMenu()
{
    screen_ = Screen::Main;
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
    case Screen::Match:
        if (!match_.active()) {
            break;
        }
        match_.keyPress(key, autoRepeat);
        if (key == Qt::Key_Q && match_.state() == Games::State::Paused) {
            returnToMenu();
        }
        break;
    case Screen::Result: {
        const menu::Result::Action action = result_.keyPress(key);
        if (action == menu::Result::Action::Retry) {
            retry();
        } else if (action == menu::Result::Action::Back) {
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
    if (screen_ == Screen::Match) {
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
    case Screen::Match:
        match_.mousePressLeft(position);
        break;
    case Screen::Result: {
        const menu::Result::Action action = result_.mousePress(position);
        if (action == menu::Result::Action::Retry) {
            retry();
        } else if (action == menu::Result::Action::Back) {
            returnToMenu();
        }
        break;
    }
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
