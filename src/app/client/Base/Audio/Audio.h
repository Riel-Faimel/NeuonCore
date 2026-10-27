#pragma once

#include <QString>

#include <memory>

namespace base {

// Audio 组件：程序化 WAV 音效池（背景音乐见 Music 组件）。
// QSoundEffect 的细节封装在 Impl。
class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio &) = delete;
    Audio &operator=(const Audio &) = delete;

    void init();
    void shutdown();
    void play(const QString &name);
    void setVolume(float volume);
    float volume() const;
    bool toggleMute();
    bool muted() const;
    bool enabled() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace base
