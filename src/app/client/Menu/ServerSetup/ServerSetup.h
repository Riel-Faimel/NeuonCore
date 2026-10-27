#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QString>

namespace menu {

// 自定义协议的联机设置：单机 / 局域网主机 / 加入主机。
// 标准协议不进入这个界面，继续走默认的内嵌服务端回环路径。
class ServerSetup {
public:
    enum class Mode {
        Single,
        Host,
        Join,
    };

    struct Config {
        Mode mode = Mode::Single;
        QString host = QStringLiteral("127.0.0.1");
        quint16 port = 7777;

        bool valid() const { return mode == Mode::Single || !host.isEmpty(); }
    };

    enum class Action {
        None,
        Start,
        Back,
    };

    ServerSetup();

    void open();
    Action keyPress(int key);
    Action mousePress(const Math::Vec2 &position);
    void render(QPainter &painter) const;

    Config config() const;
    QString portText() const { return portText_; }
    QString hostText() const { return hostText_; }
    void setMode(Mode mode);
    void setPort(quint16 port);
    void setHost(const QString &host);

private:
    enum class Focus {
        Mode,
        Port,
        Host,
        Start,
        Back,
    };

    QRectF modeRect(int index) const;
    QRectF portRect() const;
    QRectF hostRect() const;
    QRectF startRect() const;
    QRectF backRect() const;
    bool fieldVisible(Focus focus) const;
    void cycleMode(int direction);
    void cycleFocus(int direction);
    void editCharacter(QChar character);
    QString localAddress() const;

    Mode mode_ = Mode::Single;
    Focus focus_ = Focus::Mode;
    QString portText_ = QStringLiteral("7777");
    QString hostText_ = QStringLiteral("127.0.0.1");
};

} // namespace menu
