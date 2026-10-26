#include "Menu/Menu.h"

#include "Base/Base.h"
#include "Data/GameData.h"

#include <QPainter>
#include <Qt>

#include <utility>

Menu::Menu(bool enableAudio) : settings_(enableAudio)
{
    Base::initialize();
    audioRequested_ = enableAudio;
    score_.load();
    inventory_.load();
    if (enableAudio) {
        base_.initAudio();
        settings_.load(base_);
    }
    frame_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    levels_ = {
        {QStringLiteral("standard"), QStringLiteral("标准协议"),
         QStringLiteral("八波战役 · 守住星核"), 1, standardScript()},
        {QStringLiteral("custom"), QStringLiteral("自定义协议"),
         QStringLiteral("点击编辑台本"), 1, editor_.script()},
    };
}

Menu::~Menu()
{
    score_.save();
    inventory_.save();
}

// 台本是数据：src/json/waves.json 的 campaign 在编译期展开成常量，
// 这里只是把它转成这一局要提交给 Games 的 games::Script。
games::Script Menu::standardScript()
{
    games::Script script;
    script.reserve(size_t(data::kCampaign.count));
    for (int w = 0; w < data::kCampaign.count; ++w) {
        const data::WaveDef &definition = data::kCampaign.waves[size_t(w)];
        games::Wave wave;
        wave.enemies.reserve(size_t(definition.count));
        for (int i = 0; i < definition.count; ++i) {
            const data::WaveSpawn &spawn = definition.spawns[size_t(i)];
            wave.enemies.push_back({QString::fromUtf8(spawn.kind.data(), int(spawn.kind.size())),
                                    spawn.count});
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
    summary.levelName = level_ >= 0 && level_ < int(levels_.size())
                            ? levels_[size_t(level_)].name
                            : QString();
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
    if (screen_ == Screen::Editor) {
        editor_.render(painter, base_.muted());
    } else if (screen_ == Screen::Shop) {
        shop_.render(painter, score_, inventory_);
    } else if (screen_ == Screen::Settings) {
        settings_.render(painter, base_);
    } else {
        main_.render(painter, cards(), mouse_, mouseActive_, base_.muted(),
                     audioRequested_ && !base_.audioEnabled());
    }
}

void Menu::startLevel(int index)
{
    if (index < 0 || index >= int(levels_.size())) {
        return;
    }
    level_ = index;
    Games::Level level = levels_[size_t(index)];
    if (index == CustomLevelIndex) {
        // 自定义模式：台本整体由玩家定义，永远从第 1 波开始。
        level.script = editor_.script();
        level.startWave = 1;
    }
    match_.start(level, base_, score_, inventory_);
    screen_ = Screen::Match;
    base_.keyRelease(Qt::Key_Escape);
}

void Menu::returnToMenu()
{
    screen_ = Screen::Main;
    match_.stop();
    score_.save();
    inventory_.save();
    base_.clear();
}

void Menu::openEditor()
{
    editor_.prepare();
    screen_ = Screen::Editor;
    base_.playSound(QStringLiteral("ui"));
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
        case menu::Main::Action::StartLevel:
            startLevel(command.level);
            break;
        case menu::Main::Action::OpenEditor:
            openEditor();
            break;
        case menu::Main::Action::OpenShop:
            shop_.reset();
            screen_ = Screen::Shop;
            base_.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenSettings:
            settings_.reset();
            screen_ = Screen::Settings;
            base_.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::None:
            break;
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
            startLevel(level_);
        } else if (action == menu::Result::Action::Back) {
            returnToMenu();
        }
        break;
    }
    case Screen::Editor: {
        const menu::Editor::Action action = editor_.keyPress(key, base_);
        if (action == menu::Editor::Action::Deploy) {
            startLevel(CustomLevelIndex);
        } else if (action == menu::Editor::Action::Back) {
            returnToMenu();
        }
        break;
    }
    case Screen::Shop: {
        const menu::Shop::Action action = shop_.keyPress(key, score_, inventory_, base_);
        if (action == menu::Shop::Action::Back) {
            screen_ = Screen::Main;
            base_.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Settings: {
        const menu::Settings::Action action = settings_.keyPress(key, base_);
        if (action == menu::Settings::Action::Back) {
            screen_ = Screen::Main;
            base_.playSound(QStringLiteral("ui"));
        }
        break;
    }
    }
}

void Menu::keyRelease(int key)
{
    if (screen_ == Screen::Match) {
        match_.keyRelease(key);
    }
}

void Menu::mouseMove(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    if (screen_ == Screen::Match) {
        match_.mouseMove(position);
    } else if (screen_ == Screen::Settings) {
        settings_.mouseMove(position, base_);
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
        case menu::Main::Action::StartLevel:
            startLevel(command.level);
            break;
        case menu::Main::Action::OpenEditor:
            openEditor();
            break;
        case menu::Main::Action::OpenShop:
            shop_.reset();
            screen_ = Screen::Shop;
            base_.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::OpenSettings:
            settings_.reset();
            screen_ = Screen::Settings;
            base_.playSound(QStringLiteral("ui"));
            break;
        case menu::Main::Action::None:
            break;
        }
        break;
    }
    case Screen::Match:
        match_.mousePressLeft(position);
        break;
    case Screen::Result: {
        const menu::Result::Action action = result_.mousePress(position);
        if (action == menu::Result::Action::Retry) {
            startLevel(level_);
        } else if (action == menu::Result::Action::Back) {
            returnToMenu();
        }
        break;
    }
    case Screen::Editor: {
        const menu::Editor::Action action = editor_.mousePress(position, base_);
        if (action == menu::Editor::Action::Deploy) {
            startLevel(CustomLevelIndex);
        } else if (action == menu::Editor::Action::Back) {
            returnToMenu();
        }
        break;
    }
    case Screen::Shop: {
        const menu::Shop::Action action =
            shop_.mousePress(position, score_, inventory_, base_);
        if (action == menu::Shop::Action::Back) {
            screen_ = Screen::Main;
            base_.playSound(QStringLiteral("ui"));
        }
        break;
    }
    case Screen::Settings: {
        const menu::Settings::Action action = settings_.mousePress(position, base_);
        if (action == menu::Settings::Action::Back) {
            screen_ = Screen::Main;
            base_.playSound(QStringLiteral("ui"));
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
    }
}

void Menu::clearInput()
{
    base_.clear();
}

void Menu::toggleMute()
{
    if (!base_.toggleMute()) {
        base_.playSound(QStringLiteral("ui"));
    }
}
