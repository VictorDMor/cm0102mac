#include "audio_bridge.hpp"
#include <cmath>
#include <stdexcept>

namespace cm::aot {
AudioSamples decode_sound_pcm(std::span<const std::uint8_t> bytes, std::uint32_t channels,
                              std::uint32_t bits, std::uint32_t frames, double rate) {
    if ((channels != 1 && channels != 2) || (bits != 8 && bits != 16) || !frames ||
        !std::isfinite(rate) || rate <= 0 || rate > 192000)
        throw std::runtime_error("Original sound requires an additional PCM format mapping");
    if (bytes.size() != std::uint64_t(channels) * frames * (bits / 8))
        throw std::runtime_error("Original PCM byte count does not match its frame layout");
    AudioSamples result{rate, channels, frames, std::vector<float>(std::size_t(frames) * channels)};
    for (std::size_t i = 0; i < result.interleaved.size(); ++i) {
        if (bits == 8) result.interleaved[i] = (int(bytes[i]) - 128) / 128.0f;
        else {
            const auto word = (std::uint32_t(bytes[i * 2]) << 8) | bytes[i * 2 + 1];
            const auto signed_sample = word < 32768 ? std::int32_t(word) : std::int32_t(word) - 65536;
            result.interleaved[i] = signed_sample / 32768.0f;
        }
    }
    return result;
}
}
