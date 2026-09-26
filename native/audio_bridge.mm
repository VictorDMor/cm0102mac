#include "audio_bridge.hpp"
#import <AVFAudio/AVFAudio.h>
#include <atomic>
#include <array>
#include <cmath>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <thread>

namespace cm::aot {
namespace {
template<class F> decltype(auto) audio_call(F&& function) {
    @try { return function(); }
    @catch (NSException* error) {
        throw std::runtime_error("Native audio exception: " + std::string((error.reason ? error.reason : error.name).UTF8String));
    }
}
void require_audio(bool result, NSError* error, const char* operation) {
    if (!result) throw std::runtime_error(std::string(operation) + ": " +
        (error ? error.localizedDescription.UTF8String : "native audio operation failed"));
}
struct Playback {
    std::vector<float> stereo;
    std::uint32_t frames = 0, cursor = 0; // Cursor belongs to the render thread.
    std::atomic<bool> completed{false};
};
struct RenderState {
    std::atomic<Playback*> published{nullptr}, hazard{nullptr};
    std::atomic<std::uint32_t> gain{0x01000100};
};
static_assert(std::atomic<Playback*>::is_always_lock_free && std::atomic<std::uint32_t>::is_always_lock_free);
OSStatus render(RenderState& state, BOOL* silence, AVAudioFrameCount frames, AudioBufferList* output) {
    if (output->mNumberBuffers != 2) return kAudioUnitErr_FormatNotSupported;
    for (unsigned side = 0; side < 2; ++side) {
        if (!output->mBuffers[side].mData || output->mBuffers[side].mDataByteSize < std::uint64_t(frames) * sizeof(float))
            return kAudioUnitErr_FormatNotSupported;
        std::memset(output->mBuffers[side].mData, 0, std::size_t(frames) * sizeof(float));
    }
    // Protect samples across cancellation without locks, allocation, reference-
    // count destruction, or original game code on the real-time render thread.
    Playback* clip;
    do {
        clip = state.published.load();
        state.hazard.store(clip);
    } while (clip != state.published.load());
    struct Guard { RenderState& state; ~Guard() { state.hazard.store(nullptr); } } guard{state};
    *silence = YES;
    if (!clip) return noErr;
    const auto count = std::min(frames, clip->frames - clip->cursor);
    const auto gain = state.gain.load(std::memory_order_relaxed);
    const std::array<float, 2> scale{(gain & 0xffff) / 256.0f, (gain >> 16) / 256.0f};
    for (unsigned side = 0; side < 2; ++side) {
        auto* destination = static_cast<float*>(output->mBuffers[side].mData);
        for (std::uint32_t i = 0; i < count; ++i)
            destination[i] = clip->stereo[std::size_t(clip->cursor + i) * 2 + side] * scale[side];
    }
    clip->cursor += count;
    *silence = count == 0;
    if (clip->cursor == clip->frames) clip->completed.store(true, std::memory_order_release);
    return noErr;
}
}
struct NativeAudio::Impl {
    struct Channel {
        AVAudioSourceNode* source;
        std::shared_ptr<RenderState> state;
        std::uint32_t initialization;
        std::unique_ptr<Playback> current;
        std::vector<std::unique_ptr<Playback>> retired;
        void reclaim() {
            const auto active = state->hazard.load();
            std::erase_if(retired, [active](const auto& clip) { return clip.get() != active; });
        }
        void wait_for_render() const {
            while (state->hazard.load()) std::this_thread::yield();
        }
    };
    AVAudioEngine* engine;
    AVAudioFormat* stereo;
    AudioMode mode;
    double rate;
    std::map<std::uint32_t, Channel> channels;
    explicit Impl(AudioMode selected) : mode(selected) {
        engine = [AVAudioEngine new];
        rate = mode == AudioMode::offline ? 48000 : [engine.outputNode outputFormatForBus:0].sampleRate;
        if (!std::isfinite(rate) || rate <= 0) throw std::runtime_error("Native audio output has no usable sample rate");
        stereo = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:rate channels:2];
        if (!stereo) throw std::runtime_error("Native audio rejected its PCM output format");
        [engine connect:engine.mainMixerNode to:engine.outputNode format:stereo];
        if (mode == AudioMode::offline) {
            NSError* error = nil;
            const auto enabled = [engine enableManualRenderingMode:AVAudioEngineManualRenderingModeOffline
                format:stereo maximumFrameCount:4096 error:&error];
            require_audio(enabled, error, "Cannot enable native offline audio rendering");
        }
        engine.mainMixerNode.outputVolume = 1;
    }
    ~Impl() {
        for (auto& [token, channel] : channels) { (void)token; channel.state->published.store(nullptr); }
        [engine stop];
        for (auto& [token, channel] : channels) { (void)token; channel.wait_for_render(); }
    }
};
NativeAudio::NativeAudio(AudioMode mode) : impl_(audio_call([&] { return std::make_unique<Impl>(mode); })) {}
NativeAudio::~NativeAudio() = default;
void NativeAudio::create(std::uint32_t token, std::uint32_t initialization) {
    audio_call([&] {
        if (impl_->channels.contains(token)) throw std::runtime_error("Duplicate original sound channel");
        if ((initialization & ~0xc7u) || (initialization & 0xc0u) == 0x40u)
            throw std::runtime_error("Unsupported original sampled-channel initialization");
        auto state = std::make_shared<RenderState>();
        const auto pan = initialization & 3;
        state->gain = pan == 2 ? 0x00000100 : pan == 3 ? 0x01000000 : 0x01000100;
        AVAudioSourceNode* source = [[AVAudioSourceNode alloc] initWithFormat:impl_->stereo
            renderBlock:^OSStatus(BOOL* silence, const AudioTimeStamp*, AVAudioFrameCount frames, AudioBufferList* output) {
                return render(*state, silence, frames, output);
            }];
        if (!source) throw std::runtime_error("Cannot create native PCM source");
        [impl_->engine attachNode:source];
        [impl_->engine connect:source to:impl_->engine.mainMixerNode format:impl_->stereo];
        impl_->channels.emplace(token, Impl::Channel{source, state, initialization, {}, {}});
    });
}
void NativeAudio::stop(std::uint32_t token) {
    auto& channel = impl_->channels.at(token);
    channel.state->published.store(nullptr);
    if (channel.current) channel.retired.push_back(std::move(channel.current));
    channel.reclaim();
}
void NativeAudio::dispose(std::uint32_t token) {
    stop(token);
    audio_call([&] {
        auto& channel = impl_->channels.at(token);
        [impl_->engine detachNode:channel.source];
        channel.wait_for_render();
        impl_->channels.erase(token);
    });
}
void NativeAudio::volume(std::uint32_t token, std::uint16_t left, std::uint16_t right) {
    if (left > 256 || right > 256) throw std::runtime_error("Amplified original sound volume needs a verified gain mapping");
    impl_->channels.at(token).state->gain.store(std::uint32_t(left) | (std::uint32_t(right) << 16), std::memory_order_relaxed);
}
bool NativeAudio::finished(std::uint32_t token) const {
    const auto& clip = impl_->channels.at(token).current;
    return !clip || clip->completed.load(std::memory_order_acquire);
}
void NativeAudio::play(std::uint32_t token, const AudioSamples& samples) {
    audio_call([&] {
        if ((samples.channels != 1 && samples.channels != 2) || samples.interleaved.size() != std::uint64_t(samples.frames) * samples.channels ||
            !samples.frames || !std::isfinite(samples.rate) || samples.rate <= 0)
            throw std::runtime_error("Invalid native sound buffer");
        const auto count = std::ceil(samples.frames * impl_->rate / samples.rate);
        if (!std::isfinite(count) || count < 1 || count > 64 * 1024 * 1024)
            throw std::runtime_error("Resampled original sound exceeds the native buffer limit");
        auto& channel = impl_->channels.at(token);
        auto clip = std::make_unique<Playback>();
        clip->frames = std::uint32_t(count);
        clip->stereo.resize(std::size_t(clip->frames) * 2);
        const bool mono = (channel.initialization & 0xc0) != 0xc0;
        const bool interpolate = (channel.initialization & 4) == 0;
        for (std::uint32_t frame = 0; frame < clip->frames; ++frame) {
            const double source = frame * samples.rate / impl_->rate;
            const auto first = std::min(std::uint32_t(source), samples.frames - 1), next = std::min(first + 1, samples.frames - 1);
            std::array<float, 2> values{};
            for (unsigned side = 0; side < 2; ++side) {
                const auto component = samples.channels == 1 ? 0 : side;
                const auto a = samples.interleaved[std::size_t(first) * samples.channels + component];
                const auto b = samples.interleaved[std::size_t(next) * samples.channels + component];
                values[side] = interpolate ? a + float(source - first) * (b - a) : a;
            }
            if (mono && samples.channels == 2) values.fill((values[0] + values[1]) * 0.5f);
            for (unsigned side = 0; side < 2; ++side) clip->stereo[std::size_t(frame) * 2 + side] = values[side];
        }
        stop(token);
        if (!impl_->engine.running) {
            NSError* error = nil;
            const auto started = [impl_->engine startAndReturnError:&error];
            require_audio(started, error, "Cannot start native audio output");
        }
        channel.current = std::move(clip);
        channel.state->published.store(channel.current.get());
    });
}
std::vector<float> NativeAudio::render_offline(std::uint32_t frames) {
    return audio_call([&] {
        if (impl_->mode != AudioMode::offline || !frames || frames > 4096)
            throw std::runtime_error("Offline audio rendering requires the explicit test mode and 1..4096 frames");
        AVAudioPCMBuffer* buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:impl_->stereo frameCapacity:frames];
        NSError* error = nil;
        const auto status = [impl_->engine renderOffline:frames toBuffer:buffer error:&error];
        require_audio(status == AVAudioEngineManualRenderingStatusSuccess, error, "Native offline rendering failed");
        std::vector<float> result(std::size_t(buffer.frameLength) * 2);
        for (std::size_t i = 0; i < buffer.frameLength; ++i)
            for (unsigned side = 0; side < 2; ++side) result[i * 2 + side] = buffer.floatChannelData[side][i];
        return result;
    });
}
}
