#include "audio_bridge.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <exception>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void near(float actual, float expected, const char* message) {
    if (std::abs(actual - expected) > 0.0001f)
        throw std::runtime_error(std::string(message) + ": " + std::to_string(actual) + " != " + std::to_string(expected));
}
void completed(NativeAudio& audio, std::uint32_t token) {
    require(audio.finished(token), "Native rendered-buffer completion was not delivered");
}
}
int main() {
    try {
        const std::array<std::uint8_t, 8> words{0x80, 0, 0xff, 0xff, 0, 0, 0x7f, 0xff};
        auto pcm = decode_sound_pcm(words, 2, 16, 2, 22050);
        require(pcm.interleaved == std::vector<float>({-1, -1.0f / 32768, 0, 32767.0f / 32768}), "Signed big-endian 16-bit PCM decoding");
        const std::array<std::uint8_t, 3> bytes{0, 128, 255};
        require(decode_sound_pcm(bytes, 1, 8, 3, 22050).interleaved == std::vector<float>({-1, 0, 127.0f / 128}), "Unsigned 8-bit PCM decoding");
        bool rejected = false;
        try { (void)decode_sound_pcm(words, 2, 16, 3, 22050); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Truncated PCM accepted");
        NativeAudio audio(AudioMode::offline);
        audio.create(1, 0xc5); // Stereo, no interpolation.
        audio.volume(1, 128, 64);
        AudioSamples stereo{24000, 2, 1024, std::vector<float>(2048)};
        for (unsigned i = 0; i < 1024; ++i) {
            stereo.interleaved[i * 2] = i % 2 ? 0.5f : -0.5f;
            stereo.interleaved[i * 2 + 1] = 0.25f;
        }
        audio.play(1, stereo);
        require(!audio.finished(1), "Playback reported completion before rendering");
        auto rendered = audio.render_offline(3072);
        for (unsigned i = 0; i < 2048; ++i) {
            near(rendered[i * 2], (i / 2) % 2 ? 0.25f : -0.25f, "Stereo left gain or nearest resampling");
            near(rendered[i * 2 + 1], 0.0625f, "Stereo right gain/routing");
        }
        for (unsigned i = 4096; i < rendered.size(); ++i) near(rendered[i], 0, "Native buffer played beyond its last frame");
        completed(audio, 1);
        audio.dispose(1);

        audio.create(2, 0x85); // Original game's mono and no-interpolation flags.
        audio.play(2, AudioSamples{48000, 2, 1024, std::vector<float>(2048, 0.5f)});
        audio.stop(2);
        audio.play(2, AudioSamples{48000, 1, 2048, std::vector<float>(2048, -0.25f)});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        require(!audio.finished(2), "Canceled playback's completion leaked into its replacement");
        rendered = audio.render_offline(3072);
        for (unsigned i = 0; i < 4096; ++i) near(rendered[i], -0.25f, "Mono routing or canceled-buffer leakage");
        completed(audio, 2);
        stereo.rate = 48000;
        audio.play(2, stereo);
        rendered = audio.render_offline(2048);
        for (unsigned i = 0; i < 1024; ++i) {
            if (std::abs(rendered[i * 2] - (i % 2 ? 0.375f : -0.125f)) > 0.0001f) {
                std::cerr << "Mono replay mismatch at frame " << i << "; initial L/R:";
                for (unsigned j = 0; j < 8; ++j) std::cerr << ' ' << rendered[j];
                std::cerr << "; nonzero frames:";
                for (unsigned j = 0; j < 2048; j += 128) if (rendered[j * 2] != 0) std::cerr << ' ' << j;
                std::cerr << '\n';
            }
            near(rendered[i * 2], i % 2 ? 0.375f : -0.125f, "Stereo-to-mono fold");
            near(rendered[i * 2 + 1], rendered[i * 2], "Mono sides differ");
        }
        completed(audio, 2);
        audio.dispose(2);
        audio.create(3, 0xc5);
        const AudioSamples concurrent{48000, 2, 96, std::vector<float>(192, 0.5f)};
        audio.play(3, concurrent);
        std::atomic<unsigned> blocks{0};
        std::exception_ptr render_error;
        std::jthread renderer([&](std::stop_token stop) {
            try {
                while (!stop.stop_requested()) {
                    const auto block = audio.render_offline(128);
                    for (const auto sample : block)
                        require(std::isfinite(sample) && sample >= 0 && sample <= 0.5f,
                                "Concurrent native cancellation corrupted PCM samples");
                    blocks.fetch_add(1);
                }
            } catch (...) { render_error = std::current_exception(); blocks.store(UINT32_MAX); }
        });
        while (blocks.load() < 8) std::this_thread::yield();
        for (unsigned i = 0; i < 2000; ++i) {
            audio.volume(3, i % 2 ? 128 : 256, 64);
            audio.stop(3);
            audio.play(3, concurrent);
            std::this_thread::yield();
        }
        renderer.request_stop(); renderer.join();
        if (render_error) std::rethrow_exception(render_error);
        audio.dispose(3);
        std::cout << "Native offline audio: PCM byte order, stereo gains, resampling, mono routing, cancellation, and completion passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
