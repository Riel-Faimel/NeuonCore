#include "Audio.h"

#include <QAudioDevice>
#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMediaDevices>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QTime>
#include <QUrl>
#include <QVector>
#include <QtAlgorithms>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace base {
struct Audio::Impl {
    static constexpr float Tau = 6.28318530717958647692f;
    static constexpr int SampleRate = 44100;
    static constexpr int Channels = 2;
    static constexpr int PoolSize = 4;

    bool audioEnabled = false;
    bool muted = false;
    bool audioInitialized = false;
    float volume = 0.68f;
    QHash<QString, QVector<QSoundEffect *>> pools;
    QHash<QString, int> next;
    QElapsedTimer retry;   // 设备后到时的初始化重试节流
    QAudioDevice outputDevice;

    ~Impl()
    {
        clear();
    }

    // 排查用：设置 NEON_AUDIO_LOG=1 时把音频/音乐事件写到 %TEMP%\neon_core_audio.log。
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

    void clear()
    {
        for (const auto &pool : pools) {
            qDeleteAll(pool);
        }
        pools.clear();
        next.clear();
        audioEnabled = false;
    }

    QByteArray makeWav(float duration, float freqStart, float freqEnd,
                       const QString &wave, float volume, float noiseMix)
    {
        const int count = std::max(1, int(SampleRate * duration));
        QByteArray pcm;
        pcm.reserve(count * Channels * 2);
        QDataStream pcmStream(&pcm, QIODevice::WriteOnly);
        pcmStream.setByteOrder(QDataStream::LittleEndian);

        float phase = 0.0f;
        std::mt19937 rng(7);
        std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

        for (int i = 0; i < count; ++i) {
            const float t = float(i) / float(count);
            const float freq = freqStart + (freqEnd - freqStart) * t;
            phase += Tau * freq / float(SampleRate);

            float value = 0.0f;
            if (wave == QStringLiteral("square")) {
                value = std::sin(phase) >= 0.0f ? 1.0f : -1.0f;
            } else if (wave == QStringLiteral("saw")) {
                value = 2.0f * std::fmod(phase / Tau, 1.0f) - 1.0f;
            } else if (wave == QStringLiteral("noise")) {
                value = noise(rng);
            } else {
                value = std::sin(phase);
            }
            if (wave != QStringLiteral("noise") && noiseMix > 0.0f) {
                value = value * (1.0f - noiseMix) + noise(rng) * noiseMix;
            }

            const float attack = std::min(1.0f, t / 0.015f);
            const float release = std::min(1.0f, (1.0f - t) / 0.22f);
            const float mixed = std::clamp(value * attack * release * volume,
                                           -1.0f, 1.0f);
            const qint16 sample = qint16(mixed * 32767.0f);
            for (int channel = 0; channel < Channels; ++channel) {
                pcmStream << sample;
            }
        }

        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << quint32(36 + pcm.size());
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << quint32(16);
        out << quint16(1);
        out << quint16(Channels);
        out << quint32(SampleRate);
        out << quint32(SampleRate * Channels * 2);
        out << quint16(Channels * 2);
        out << quint16(16);
        out.writeRawData("data", 4);
        out << quint32(pcm.size());
        out.writeRawData(pcm.constData(), pcm.size());
        return wav;
    }
};
void Audio::init()
{
    Impl &io = *impl_;
    if (io.audioInitialized) {
        return;
    }
    io.audioInitialized = true;
    io.clear();

    // 受限环境下可能拿不到“默认输出设备”，但设备列表里仍然有可用端点，退而取第一个。
    QAudioDevice output = QMediaDevices::defaultAudioOutput();
    if (output.isNull()) {
        const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
        if (outputs.isEmpty()) {
            // 设备可能在启动之后才就绪（插耳机 / 蓝牙重连），play() 会节流重试。
            return;
        }
        output = outputs.first();
    }
    io.outputDevice = output;
    io.logEvent(QStringLiteral("audio init: device=%1").arg(output.description()));

    // 每个进程写自己的子目录：同时开两个实例也不会覆盖对方正在加载的 WAV。
    const QString rootPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + QStringLiteral("/neon_core_cpp_audio");
    QDir root;
    if (!root.mkpath(rootPath)) {
        return;
    }
    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-1);
    for (const QFileInfo &entry :
         QDir(rootPath).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (entry.lastModified() < cutoff) {
            QDir(entry.absoluteFilePath()).removeRecursively();
        }
    }
    const QString dirPath = rootPath + QStringLiteral("/")
                            + QString::number(QCoreApplication::applicationPid());
    QDir dir;
    if (!dir.mkpath(dirPath)) {
        return;
    }

    struct SoundDef {
        QString name;
        float duration;
        float freqStart;
        float freqEnd;
        QString wave;
        float volume;
        float noiseMix;
    };

    const QVector<SoundDef> defs = {
        {QStringLiteral("shoot"), 0.065f, 980.0f, 560.0f, QStringLiteral("square"), 0.20f, 0.10f},
        {QStringLiteral("hit"), 0.045f, 320.0f, 150.0f, QStringLiteral("noise"), 0.16f, 0.65f},
        {QStringLiteral("kill"), 0.20f, 220.0f, 55.0f, QStringLiteral("noise"), 0.30f, 0.72f},
        {QStringLiteral("hurt"), 0.30f, 160.0f, 55.0f, QStringLiteral("saw"), 0.32f, 0.35f},
        {QStringLiteral("dash"), 0.16f, 260.0f, 900.0f, QStringLiteral("sine"), 0.22f, 0.15f},
        {QStringLiteral("shoot_enemy"), 0.09f, 480.0f, 220.0f, QStringLiteral("saw"), 0.10f, 0.18f},
        {QStringLiteral("pickup"), 0.25f, 620.0f, 1240.0f, QStringLiteral("sine"), 0.24f, 0.03f},
        {QStringLiteral("ui"), 0.08f, 700.0f, 980.0f, QStringLiteral("sine"), 0.20f, 0.00f},
        {QStringLiteral("upgrade"), 0.55f, 360.0f, 1120.0f, QStringLiteral("sine"), 0.26f, 0.03f},
        {QStringLiteral("boss"), 0.85f, 90.0f, 42.0f, QStringLiteral("saw"), 0.28f, 0.42f},
        {QStringLiteral("victory"), 1.50f, 260.0f, 980.0f, QStringLiteral("sine"), 0.34f, 0.02f},
        {QStringLiteral("defeat"), 1.20f, 210.0f, 48.0f, QStringLiteral("saw"), 0.34f, 0.24f},
        {QStringLiteral("explode"), 0.45f, 150.0f, 38.0f, QStringLiteral("noise"), 0.36f, 0.85f},
    };

    for (const SoundDef &def : defs) {
        const QString filename = dirPath + QStringLiteral("/") + def.name + QStringLiteral(".wav");
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            continue;
        }
        file.write(io.makeWav(def.duration, def.freqStart, def.freqEnd,
                              def.wave, def.volume, def.noiseMix));
        file.close();

        const QUrl url = QUrl::fromLocalFile(filename);
        QVector<QSoundEffect *> pool;
        for (int i = 0; i < Impl::PoolSize; ++i) {
            auto *effect = new QSoundEffect();
            effect->setSource(url);
            effect->setLoopCount(1);
            effect->setVolume(io.volume);
            if (!io.outputDevice.isNull()) {
                effect->setAudioDevice(io.outputDevice);
            }
            pool.append(effect);
        }
        io.pools.insert(def.name, pool);
        io.next.insert(def.name, 0);
    }
    io.audioEnabled = !io.pools.isEmpty();
}

void Audio::shutdown()
{
    Impl &io = *impl_;
    if (!io.audioInitialized && io.pools.isEmpty()) {
        return;
    }
    io.logEvent(QStringLiteral("audio shutdown"));
    io.clear();
    io.audioInitialized = false;
}

void Audio::play(const QString &name)
{
    Impl &io = *impl_;
    if (io.muted) {
        return;
    }
    if (!io.audioEnabled) {
        if (!io.audioInitialized) {
            return;
        }
        // 启动时设备还没就绪：最多每 2 秒重试一次初始化，设备出现后声音自动恢复。
        if (io.retry.isValid()) {
            if (io.retry.elapsed() < 2000) {
                return;
            }
            io.retry.restart();
        } else {
            io.retry.start();
        }
        io.audioInitialized = false;
        io.logEvent(QStringLiteral("audio retry: enabled=%1").arg(io.audioEnabled));
        init();
        if (!io.audioEnabled) {
            return;
        }
    }
    const auto it = io.pools.find(name);
    if (it == io.pools.end() || it.value().isEmpty()) {
        return;
    }
    const QVector<QSoundEffect *> &pool = it.value();
    QSoundEffect *selected = nullptr;
    for (QSoundEffect *effect : pool) {
        if (!effect->isPlaying()) {
            selected = effect;
            break;
        }
    }
    if (!selected) {
        int &index = io.next[name];
        selected = pool.at(index % pool.size());
        index = (index + 1) % pool.size();
        selected->stop();
    }
    selected->play();
}

void Audio::setVolume(float volume)
{
    Impl &io = *impl_;
    io.volume = std::clamp(volume, 0.0f, 1.0f);
    for (const auto &pool : io.pools) {
        for (QSoundEffect *effect : pool) {
            effect->setVolume(io.volume);
        }
    }
}

float Audio::volume() const
{
    return impl_->volume;
}

bool Audio::toggleMute()
{
    Impl &io = *impl_;
    io.muted = !io.muted;
    if (io.muted) {
        for (const auto &pool : io.pools) {
            for (QSoundEffect *effect : pool) {
                effect->stop();
            }
        }
    }
    return io.muted;
}

bool Audio::muted() const
{
    return impl_->muted;
}

bool Audio::enabled() const
{
    return impl_->audioEnabled;
}

Audio::Audio() : impl_(std::make_unique<Impl>()) {}

Audio::~Audio() = default;

} // namespace base
