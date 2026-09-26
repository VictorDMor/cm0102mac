#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace cm::aot {
enum class AudioMode { device, offline };
struct AudioSamples {
    double rate;
    std::uint32_t channels, frames;
    std::vector<float> interleaved;
};
AudioSamples decode_sound_pcm(std::span<const std::uint8_t> bytes, std::uint32_t channels,
                              std::uint32_t bits, std::uint32_t frames, double rate);

// Native audio owns sample copies. Its render blocks fill native PCM and publish
// completion flags; original callbacks run through the main-thread bridge.
class NativeAudio {
public:
    explicit NativeAudio(AudioMode mode = AudioMode::device);
    ~NativeAudio();
    void create(std::uint32_t channel, std::uint32_t initialization);
    void dispose(std::uint32_t channel);
    void volume(std::uint32_t channel, std::uint16_t left, std::uint16_t right);
    void play(std::uint32_t channel, const AudioSamples& samples);
    void stop(std::uint32_t channel);
    bool finished(std::uint32_t channel) const;
    std::vector<float> render_offline(std::uint32_t frames);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
