#include "Music.h"

#include <QAudioDevice>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QStandardPaths>
#include <QTime>
#include <QTimer>
#include <QUrl>

#include <algorithm>

namespace base {

struct Music::Impl {
    QMediaPlayer *player = nullptr;
    QAudioOutput *output = nullptr;
    QTimer timer;
    QStringList files;
    int index = -1;
    int failures = 0;
    bool muted = false;
    bool scanned = false;
    float volume = 0.45f;

    static void logEvent(const QString &message)
    {
        if (!qEnvironmentVariableIsSet("NEON_AUDIO_LOG")) {
            return;
        }
        QFile file(QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                   + QStringLiteral("/neon_core_audio.log"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            file.write(QStringLiteral("[%1] %2\n")
                           .arg(QTime::currentTime().toString(QStringLiteral("mm:ss.zzz")),
                                message)
                           .toUtf8());
        }
    }

    static QString directory()
    {
        const QString override = qEnvironmentVariable("NEON_MUSIC_DIR");
        if (!override.isEmpty()) {
            return override;
        }
        return QCoreApplication::applicationDirPath() + QStringLiteral("/music");
    }

    void scan()
    {
        if (scanned) {
            return;
        }
        scanned = true;
        const QStringList filters = {
            QStringLiteral("*.mp4"), QStringLiteral("*.m4a"), QStringLiteral("*.mp3"),
            QStringLiteral("*.flac"), QStringLiteral("*.ogg"), QStringLiteral("*.opus"),
            QStringLiteral("*.wav"), QStringLiteral("*.aac")};
        for (const QFileInfo &entry :
             QDir(directory()).entryInfoList(filters, QDir::Files, QDir::Name)) {
            files.append(entry.absoluteFilePath());
        }
        logEvent(QStringLiteral("music: %1 files in %2").arg(files.size()).arg(directory()));
    }

    void playNext()
    {
        if (player == nullptr || files.isEmpty()) {
            return;
        }
        index = (index + 1) % files.size();
        logEvent(QStringLiteral("music play [%1/%2] %3")
                     .arg(index + 1)
                     .arg(files.size())
                     .arg(QFileInfo(files.at(index)).fileName()));
        player->setSource(QUrl::fromLocalFile(files.at(index)));
        player->play();
    }

    void tick()
    {
        scan();
        if (files.isEmpty()) {
            return;
        }
        if (player == nullptr) {
            // 设备可能比进程晚就绪：定时器等它出现，出现后就只创建一次播放器。
            QAudioDevice device = QMediaDevices::defaultAudioOutput();
            if (device.isNull()) {
                const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
                if (outputs.isEmpty()) {
                    return;
                }
                device = outputs.first();
            }
            output = new QAudioOutput();
            output->setVolume(volume);
            output->setMuted(muted);
            output->setDevice(device);
            player = new QMediaPlayer();
            player->setAudioOutput(output);
            logEvent(QStringLiteral("music: player ready on %1").arg(device.description()));
            playNext();
            return;
        }
        const QMediaPlayer::MediaStatus status = player->mediaStatus();
        if (status == QMediaPlayer::EndOfMedia) {
            failures = 0;
            playNext();
        } else if (status == QMediaPlayer::InvalidMedia) {
            ++failures;
            if (failures < files.size()) {
                playNext();
            } else {
                player->stop();
            }
        }
    }
};

Music::Music() : impl_(std::make_unique<Impl>()) {}

Music::~Music()
{
    shutdown();
}

// 可在进程退出前显式调用；重复调用安全（指针置空）。
void Music::shutdown()
{
    impl_->timer.stop();
    delete impl_->player;
    impl_->player = nullptr;
    delete impl_->output;
    impl_->output = nullptr;
}

void Music::initialize()
{
    Impl &io = *impl_;
    io.timer.setInterval(500);
    QObject::connect(&io.timer, &QTimer::timeout, &io.timer, [&io] { io.tick(); });
    io.timer.start();
    io.tick();
}

void Music::setMuted(bool muted)
{
    impl_->muted = muted;
    if (impl_->output != nullptr) {
        impl_->output->setMuted(muted);
    }
}

void Music::setVolume(float volume)
{
    impl_->volume = std::clamp(volume, 0.0f, 1.0f);
    if (impl_->output != nullptr) {
        impl_->output->setVolume(impl_->volume);
    }
}

float Music::volume() const
{
    return impl_->volume;
}

bool Music::playing() const
{
    return impl_->player != nullptr
           && impl_->player->playbackState() == QMediaPlayer::PlayingState;
}

int Music::trackCount() const
{
    return int(impl_->files.size());
}

QString Music::currentTrack() const
{
    if (impl_->index < 0 || impl_->index >= impl_->files.size()) {
        return QString();
    }
    return QFileInfo(impl_->files.at(impl_->index)).fileName();
}

} // namespace base
