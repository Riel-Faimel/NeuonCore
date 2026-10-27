#include "Net/Server.h"
#include "Loadout/Loadout.h"
#include "Shop/Shop.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QString>
#include <QStringList>

#include <cstdio>
#include <thread>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("NeonCore"));
    QCoreApplication::setOrganizationName(QStringLiteral("NeonCore"));
    // 权威钱包/库存/发射器由服务端进程持有并持久化。
    ShopService.load();
    LoadoutService.load();

    quint16 port = 7777;
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QStringLiteral("--port") && i + 1 < args.size()) {
            bool ok = false;
            const uint parsed = args[++i].toUInt(&ok);
            if (!ok || parsed == 0 || parsed > 65535) {
                std::fprintf(stderr, "SERVER ERROR: invalid port\n");
                return 2;
            }
            port = quint16(parsed);
        } else if (args[i] == QStringLiteral("--help")
                   || args[i] == QStringLiteral("-h")) {
            std::printf("NeonCore server\n"
                        "  --port <1-65535>   listen port (default 7777)\n");
            return 0;
        } else {
            std::fprintf(stderr, "SERVER ERROR: unknown argument %s\n",
                         args[i].toUtf8().constData());
            return 2;
        }
    }

    server::GameServer gameServer;
    QString error;
    if (!gameServer.listen(port, false, &error)) {
        std::fprintf(stderr, "SERVER ERROR: %s\n", error.toUtf8().constData());
        return 1;
    }
    std::printf("NEON CORE server listening on port %u (tick %d Hz)\n",
                unsigned(gameServer.port()), gameServer.tickRate());
    std::fflush(stdout);

    // 控制面：stdin 上读 [u32 length][payload]，与 TCP 数据面完全分离。
    // 长度是小端 payload 字节数；payload 内含 u16 type + u16 version + body。
    std::thread controlReader([&gameServer] {
        for (;;) {
            unsigned char lengthBytes[4] = {};
            if (std::fread(lengthBytes, 1, 4, stdin) != 4) {
                return;
            }
            const quint32 length = quint32(lengthBytes[0])
                                   | (quint32(lengthBytes[1]) << 8)
                                   | (quint32(lengthBytes[2]) << 16)
                                   | (quint32(lengthBytes[3]) << 24);
            if (length == 0 || length > net::kMaxPayloadBytes) {
                std::fprintf(stderr, "CONTROL ERROR: invalid frame length\n");
                return;
            }
            QByteArray payload;
            payload.resize(int(length));
            if (std::fread(payload.data(), 1, size_t(length), stdin)
                != size_t(length)) {
                return;
            }
            QByteArray frame;
            frame.reserve(int(length) + 4);
            frame.append(char(length & 0xff));
            frame.append(char((length >> 8) & 0xff));
            frame.append(char((length >> 16) & 0xff));
            frame.append(char((length >> 24) & 0xff));
            frame.append(payload);
            QMetaObject::invokeMethod(
                &gameServer,
                [&gameServer, frame] {
                    QString error;
                    if (!gameServer.handleControlFrame(frame, &error)) {
                        std::fprintf(stderr, "CONTROL ERROR: %s\n",
                                     error.toUtf8().constData());
                    }
                },
                Qt::QueuedConnection);
        }
    });
    controlReader.detach();

    const int code = app.exec();
    ShopService.save();
    LoadoutService.save();
    return code;
}
