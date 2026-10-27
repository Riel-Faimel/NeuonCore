#include "Menu/ServerSetup/ServerSetup.h"

#include "Base/Base.h"
#include "GameConfig.h"

#include <QHostAddress>
#include <QLinearGradient>
#include <QNetworkInterface>
#include <QPainter>
#include <QPen>
#include <QStringList>
#include <Qt>

#include <algorithm>

namespace menu {

namespace {

QString modeLabel(int index)
{
    switch (index) {
    case 1:
        return QStringLiteral("局域网主机");
    case 2:
        return QStringLiteral("加入主机");
    default:
        return QStringLiteral("单机");
    }
}

QString modeHint(int index)
{
    switch (index) {
    case 1:
        return QStringLiteral("在本机开放端口，等待同一局域网的客户端加入");
    case 2:
        return QStringLiteral("连接已有主机，使用对方的地址和端口");
    default:
        return QStringLiteral("不开放端口，使用默认本地路径");
    }
}

} // namespace

ServerSetup::ServerSetup() = default;

void ServerSetup::open()
{
    focus_ = Focus::Mode;
}

QRectF ServerSetup::modeRect(int index) const
{
    constexpr float width = 198.0f;
    constexpr float gap = 18.0f;
    const float total = width * 3.0f + gap * 2.0f;
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + float(index) * (width + gap), 206.0f, width, 54.0f);
}

QRectF ServerSetup::portRect() const
{
    const float top = mode_ == Mode::Join ? 468.0f : 404.0f;
    return QRectF(GameConfig::ScreenWidth / 2.0f - 190.0f, top, 380.0f, 48.0f);
}

QRectF ServerSetup::hostRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 190.0f, 390.0f, 380.0f, 48.0f);
}

QRectF ServerSetup::startRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 210.0f, 566.0f, 200.0f, 46.0f);
}

QRectF ServerSetup::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f + 10.0f, 566.0f, 200.0f, 46.0f);
}

bool ServerSetup::fieldVisible(Focus focus) const
{
    switch (focus) {
    case Focus::Mode:
    case Focus::Start:
    case Focus::Back:
        return true;
    case Focus::Port:
        return mode_ != Mode::Single;
    case Focus::Host:
        return mode_ == Mode::Join;
    }
    return false;
}

void ServerSetup::cycleMode(int direction)
{
    const int current = int(mode_);
    const int next = (current + (direction >= 0 ? 1 : 2)) % 3;
    mode_ = Mode(next);
    if (!fieldVisible(focus_)) {
        focus_ = Focus::Mode;
    }
    BaseService.playSound(QStringLiteral("ui"));
}

void ServerSetup::cycleFocus(int direction)
{
    constexpr Focus order[] = {Focus::Mode, Focus::Port, Focus::Host,
                               Focus::Start, Focus::Back};
    int index = 0;
    for (int i = 0; i < 5; ++i) {
        if (order[i] == focus_) {
            index = i;
            break;
        }
    }
    for (int step = 0; step < 8; ++step) {
        index = (index + (direction >= 0 ? 1 : 4) + 5) % 5;
        if (fieldVisible(order[index])) {
            focus_ = order[index];
            BaseService.playSound(QStringLiteral("ui"));
            return;
        }
    }
}

void ServerSetup::editCharacter(QChar character)
{
    if (focus_ == Focus::Port) {
        if (character.isDigit() && portText_.size() < 5) {
            if (portText_ == QStringLiteral("7777")) {
                portText_.clear();
            }
            portText_.append(character);
        }
    } else if (focus_ == Focus::Host) {
        if ((character.isLetterOrNumber() || character == QLatin1Char('.')
             || character == QLatin1Char('-') || character == QLatin1Char(':'))
            && hostText_.size() < 64) {
            hostText_.append(character);
        }
    }
}

void ServerSetup::setMode(Mode mode)
{
    mode_ = mode;
    if (!fieldVisible(focus_)) {
        focus_ = Focus::Mode;
    }
}

void ServerSetup::setPort(quint16 port)
{
    portText_ = QString::number(std::max<quint16>(1, port));
}

void ServerSetup::setHost(const QString &host)
{
    hostText_ = host;
}

ServerSetup::Config ServerSetup::config() const
{
    Config result;
    result.mode = mode_;
    result.host = hostText_.trimmed();
    const uint value = portText_.toUInt();
    result.port = value > 0 && value <= 65535 ? quint16(value) : 0;
    if (result.mode == Mode::Single) {
        result.host.clear();
        result.port = 0;
    } else if (result.mode == Mode::Host) {
        result.host.clear();
    }
    return result;
}

ServerSetup::Action ServerSetup::keyPress(int key)
{
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        cycleFocus(key == Qt::Key_Tab ? 1 : -1);
        return Action::None;
    }
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        cycleFocus(-1);
        return Action::None;
    }
    if (key == Qt::Key_Down || key == Qt::Key_S) {
        cycleFocus(1);
        return Action::None;
    }
    if (key == Qt::Key_Left || key == Qt::Key_A) {
        if (focus_ == Focus::Mode) {
            cycleMode(-1);
        }
        return Action::None;
    }
    if (key == Qt::Key_Right || key == Qt::Key_D) {
        if (focus_ == Focus::Mode) {
            cycleMode(1);
        }
        return Action::None;
    }
    if (key == Qt::Key_Backspace) {
        if (focus_ == Focus::Port && !portText_.isEmpty()) {
            portText_.chop(1);
        } else if (focus_ == Focus::Host && !hostText_.isEmpty()) {
            hostText_.chop(1);
        }
        return Action::None;
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        if (focus_ == Focus::Back) {
            return Action::Back;
        }
        const Config cfg = config();
        if (cfg.valid() && (cfg.mode == Mode::Single || cfg.port != 0)) {
            return Action::Start;
        }
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        editCharacter(QChar(QLatin1Char('0' + char(key - Qt::Key_0))));
    } else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        editCharacter(QChar(QLatin1Char('a' + char(key - Qt::Key_A))));
    } else if (key == Qt::Key_Period) {
        editCharacter(QLatin1Char('.'));
    } else if (key == Qt::Key_Minus) {
        editCharacter(QLatin1Char('-'));
    }
    return Action::None;
}

ServerSetup::Action ServerSetup::mousePress(const Math::Vec2 &position)
{
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    if (startRect().contains(point)) {
        const Config cfg = config();
        if (cfg.valid() && (cfg.mode == Mode::Single || cfg.port != 0)) {
            return Action::Start;
        }
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    for (int i = 0; i < 3; ++i) {
        if (modeRect(i).contains(point)) {
            setMode(Mode(i));
            focus_ = Focus::Mode;
            BaseService.playSound(QStringLiteral("ui"));
            return Action::None;
        }
    }
    if (fieldVisible(Focus::Port) && portRect().contains(point)) {
        focus_ = Focus::Port;
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    if (fieldVisible(Focus::Host) && hostRect().contains(point)) {
        focus_ = Focus::Host;
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    return Action::None;
}

QString ServerSetup::localAddress() const
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol
            && !address.isLoopback()) {
            return address.toString();
        }
    }
    return QStringLiteral("127.0.0.1");
}

void ServerSetup::render(QPainter &painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9));
    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.6, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);
    painter.save();
    painter.setPen(QPen(QColor(23, 50, 70), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("CUSTOM PROTOCOL  //  NETWORK SETUP"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 72}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("联机设置"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 122},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("标准协议不需要配置；自定义协议可选择单机、开房或加入"),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 160},
               QStringLiteral("center"), 230);

    for (int i = 0; i < 3; ++i) {
        const QRectF rect = modeRect(i);
        const bool selected = int(mode_) == i;
        const QColor accent = i == 0 ? Base::Cyan
                                     : (i == 1 ? Base::Green : Base::Amber);
        Base::panel(painter, rect, selected ? accent : QColor(48, 91, 112),
                    selected ? 240 : 180, 10,
                    selected ? QColor(9, 24, 38) : QColor(6, 14, 25));
        Base::text(painter, Base::mediumFont(), modeLabel(i),
                   selected ? Base::White : Base::Text,
                   {float(rect.center().x()), float(rect.center().y())},
                   QStringLiteral("center"), 220);
    }

    const float infoTop = 300.0f;
    Base::panel(painter, QRectF(240, infoTop, 800, 250), QColor(43, 83, 103),
                180, 12, QColor(5, 13, 23));
    Base::text(painter, Base::techTinyFont(), QStringLiteral("MODE"),
               Base::Muted, {262, infoTop + 26}, QStringLiteral("midleft"), 180);
    Base::text(painter, Base::smallFont(), modeHint(int(mode_)), Base::Text,
               {262, infoTop + 44}, QStringLiteral("midleft"), 210);

    if (mode_ == Mode::Single) {
        Base::text(painter, Base::smallFont(),
                   QStringLiteral("默认路径：不监听端口，不开服务器"),
                   Base::Cyan, {262, infoTop + 110},
                   QStringLiteral("midleft"), 210);
    } else {
        const QRectF field = mode_ == Mode::Join ? hostRect() : portRect();
        if (mode_ == Mode::Join) {
            Base::text(painter, Base::techTinyFont(), QStringLiteral("HOST ADDRESS"),
                       Base::Muted, {float(field.left()), float(field.top()) - 18.0f},
                       QStringLiteral("midleft"), 170);
            Base::panel(painter, field,
                        focus_ == Focus::Host ? Base::Amber : QColor(60, 104, 126),
                        focus_ == Focus::Host ? 240 : 190, 8, QColor(3, 10, 18));
            Base::text(painter, Base::mediumFont(),
                       hostText_.isEmpty() ? QStringLiteral("输入主机地址") : hostText_,
                       hostText_.isEmpty() ? Base::Muted : Base::White,
                       {float(field.left()) + 16.0f, float(field.center().y())},
                       QStringLiteral("midleft"), 220);
        }
        const QRectF port = portRect();
        Base::text(painter, Base::techTinyFont(), QStringLiteral("PORT"),
                   Base::Muted, {float(port.left()), float(port.top()) - 18.0f},
                   QStringLiteral("midleft"), 170);
        Base::panel(painter, port,
                    focus_ == Focus::Port ? Base::Cyan : QColor(60, 104, 126),
                    focus_ == Focus::Port ? 240 : 190, 8, QColor(3, 10, 18));
        Base::text(painter, Base::mediumFont(), portText_,
                   portText_.isEmpty() ? Base::Muted : Base::White,
                   {float(port.left()) + 16.0f, float(port.center().y())},
                   QStringLiteral("midleft"), 220);

        if (mode_ == Mode::Host) {
            const QString ip = localAddress();
            Base::text(painter, Base::smallFont(),
                       QStringLiteral("同一局域网加入：%1:%2").arg(ip, portText_),
                       Base::Cyan, {262, infoTop + 172},
                       QStringLiteral("midleft"), 210);
            Base::text(painter, Base::tinyFont(),
                       QStringLiteral("端口会监听所有网卡；防火墙需要放行"),
                       Base::Muted, {262, infoTop + 198},
                       QStringLiteral("midleft"), 200);
        } else {
            Base::text(painter, Base::tinyFont(),
                       QStringLiteral("连接已有主机；端口必须与主机一致"),
                       Base::Muted, {262, infoTop + 226},
                       QStringLiteral("midleft"), 200);
        }
    }

    const Config cfg = config();
    const bool canStart = cfg.valid() && (cfg.mode == Mode::Single || cfg.port != 0);
    Base::panel(painter, startRect(),
                canStart ? Base::Green : QColor(60, 82, 94), 210, 9,
                QColor(6, 16, 26));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ENTER  开始"),
               canStart ? Base::Text : Base::Muted,
               {float(startRect().center().x()), float(startRect().center().y())},
               QStringLiteral("center"));
    Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 9,
                QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"),
               Base::Text,
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("↑↓ 选择   ←→ 切换模式   数字编辑端口   ENTER 开始   ESC 返回"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 28.0f},
               QStringLiteral("center"), 160);
}

} // namespace menu
