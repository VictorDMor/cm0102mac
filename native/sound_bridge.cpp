#include "sound_bridge.hpp"
#include "aot_runtime.hpp"
#include "callbacks.hpp"
#include "process_bridge.hpp"
#include <deque>
#include <map>
#include <optional>
#include <thread>
#include <cstdio>
#include <cstdlib>

namespace cm::aot {
namespace {
struct Command { std::uint16_t code, parameter; std::uint32_t data; };
constexpr std::uint32_t channel_bytes = 1060;
constexpr std::int16_t bad_channel = -205, queue_full = -203;
}
struct NativeSound::Impl {
    struct Channel {
        bool owned;
        std::uint16_t capacity;
        std::deque<Command> pending;
        bool closing = false, drained = false;
    };
    AudioMode mode;
    std::unique_ptr<NativeAudio> audio;
    std::map<std::uint32_t, Channel> channels;
    bool polling = false;
    bool trace = std::getenv("CM_AOT_TRACE_AUDIO") != nullptr;
    std::optional<std::uint32_t> application_a5;
    std::uint32_t current_a5 = 0;
    explicit Impl(AudioMode selected) : mode(selected) {}
    NativeAudio& output() {
        if (!audio) audio = std::make_unique<NativeAudio>(mode);
        return *audio;
    }
    AudioSamples samples(State& s, std::uint32_t header) {
        if (s.memory.read<1>(header + 20) != 0xff)
            throw std::runtime_error("Original sound needs a standard/compressed header mapping");
        const auto channels = std::uint32_t(s.memory.read<4>(header + 4));
        const auto frames = std::uint32_t(s.memory.read<4>(header + 22));
        const auto bits = std::uint32_t(s.memory.read<2>(header + 48));
        if ((channels != 1 && channels != 2) || (bits != 8 && bits != 16))
            throw std::runtime_error("Original extended sound has an unsupported PCM layout");
        // bufferCmd ignores loop points and baseFrequency. It plays every frame
        // once; the game's callback explicitly resubmits looping sounds.
        const auto count = std::uint64_t(channels) * frames * (bits / 8);
        if (!count || count > 128 * 1024 * 1024)
            throw std::runtime_error("Original sound exceeds the PCM buffer limit");
        const auto pointer = std::uint32_t(s.memory.read<4>(header));
        const auto rate = s.memory.read<4>(header + 8) / 65536.0;
        return decode_sound_pcm(s.memory.copy_out(pointer ? pointer : header + 66, count), channels, bits, frames, rate);
    }
    void execute(State& s, std::uint32_t token, Command command) {
        switch (command.code) {
            case 0: break;
            case 3: output().stop(token); break;
            case 4: {
                auto& channel = channels.at(token);
                channel.pending.clear();
                if (channel.closing) channel.pending.push_back({0xffff, 0, 0});
                break;
            }
            case 46: output().volume(token, command.data & 0xffff, command.data >> 16); break;
            case 81: {
                if (!output().finished(token))
                    throw std::runtime_error("Immediate bufferCmd during active playback requires a verified Sound Manager scheduling policy");
                const auto buffer = samples(s, command.data);
                output().play(token, buffer);
                if (trace) std::fprintf(stderr, "Native audio scheduled: channel=%08x mode=%s frames=%u sourceChannels=%u rate=%.8g\n",
                    token, mode == AudioMode::device ? "device" : "offline", buffer.frames, buffer.channels, buffer.rate);
                break;
            }
            case 13: {
                const auto callback = std::uint32_t(s.memory.read<4>(token + 8));
                if (!callback) break;
                const auto record = allocate(s, 8, true);
                if (!record) throw std::runtime_error("Original sound callback command allocation failed");
                s.memory.write<2>(record, command.code); s.memory.write<2>(record + 2, command.parameter);
                s.memory.write<4>(record + 4, command.data);
                const std::array arguments{token, record};
                if (trace) std::fprintf(stderr, "Original sound callback: channel=%08x vector=%08x context=%08x\n", token, callback, command.data);
                try { (void)call_original(s, callback, arguments); }
                catch (...) { deallocate(s, record); throw; }
                deallocate(s, record);
                break;
            }
            default: throw std::runtime_error("Unimplemented original sound command " + std::to_string(command.code));
        }
    }
    void poll(State& s) {
        if (polling || !audio) return;
        struct Guard { bool& value; ~Guard() { value = false; } } guard{polling};
        polling = true;
        std::vector<std::uint32_t> tokens;
        for (const auto& [token, channel] : channels) { (void)channel; tokens.push_back(token); }
        for (const auto token : tokens) {
            // Original callbacks can alter queues or dispose/create channels.
            for (unsigned processed = 0; processed < 128; ++processed) {
                const auto found = channels.find(token);
                if (found == channels.end() || found->second.drained || found->second.pending.empty() || !audio->finished(token)) break;
                auto command = found->second.pending.front();
                found->second.pending.pop_front();
                if (command.code == 0xffff) { found->second.drained = true; break; }
                execute(s, token, command);
            }
        }
    }
};
NativeSound::NativeSound(AudioMode mode) : impl_(std::make_unique<Impl>(mode)) {}
NativeSound::~NativeSound() = default;
void NativeSound::poll(State& s) { impl_->poll(s); }
bool NativeSound::has_pending_commands() const {
    for (const auto& [token, channel] : impl_->channels) {
        (void)token;
        if (!channel.pending.empty()) return true;
    }
    return false;
}
std::vector<float> NativeSound::render_offline(std::uint32_t frames) { return impl_->output().render_offline(frames); }
bool NativeSound::dispatch(State& s, std::string_view name) {
    auto arg = [&](unsigned i) { return std::uint32_t(s.r<4>((i + 3) * 4)); };
    std::uint32_t result = 0;
    if (name == "CarbonLib::x80tod") {
        s.wr<8>(16400, float_bits<8>(native_extended80(s.memory.copy_out(arg(0), 10))));
        result = arg(0);
    } else if (name == "CarbonLib::X2Fix") {
        result = std::uint32_t(native_fixed(float_value<8>(s.r<8>(16400))));
    } else if (name == "CarbonLib::SetCurrentA5" || name == "CarbonLib::SetA5") {
        // These 68K context APIs have no ARM register equivalent. Retain a
        // logical original context keyed by the application's PowerPC TOC;
        // actual global access uses each original callback's transition vector.
        if (!impl_->application_a5) {
            impl_->application_a5 = std::uint32_t(s.r<4>(8));
            impl_->current_a5 = *impl_->application_a5;
        }
        result = impl_->current_a5;
        impl_->current_a5 = name == "CarbonLib::SetCurrentA5" ? *impl_->application_a5 : arg(0);
    } else if (name == "CarbonLib::NewSndCallBackUPP") {
        if (arg(0)) (void)s.memory.copy_out(arg(0), 8); // Original {entry, TOC}, never a host pointer.
        result = arg(0);
    } else if (name == "CarbonLib::SndNewChannel") {
        if (std::int16_t(arg(1)) != 5) throw std::runtime_error("Only the original sampled-sound synthesizer is mapped");
        const auto existing = std::uint32_t(s.memory.read<4>(arg(0)));
        const auto token = existing ? existing : allocate(s, channel_bytes, true);
        if (!token) result = std::uint32_t(-108);
        else {
            if (impl_->channels.contains(token)) throw std::runtime_error("Original sound channel initialized twice");
            const auto capacity = existing ? std::uint16_t(s.memory.read<2>(token + 30)) : std::uint16_t(128);
            if (!capacity || capacity > 128) throw std::runtime_error("Original custom sound queue capacity needs a verified layout");
            try { impl_->output().create(token, arg(2)); }
            catch (...) { if (!existing) deallocate(s, token); throw; }
            s.memory.write<4>(token + 8, arg(3));
            s.memory.write<2>(token + 30, capacity);
            impl_->channels.emplace(token, Impl::Channel{!existing, capacity, {}});
            s.memory.write<4>(arg(0), token);
        }
    } else if (name == "CarbonLib::SndDoImmediate" || name == "CarbonLib::SndDoCommand") {
        const auto found = impl_->channels.find(arg(0));
        if (found == impl_->channels.end()) result = std::uint32_t(bad_channel);
        else {
            const auto address = arg(1);
            const Command command{std::uint16_t(s.memory.read<2>(address)), std::uint16_t(s.memory.read<2>(address + 2)),
                                  std::uint32_t(s.memory.read<4>(address + 4))};
            if (command.code != 0 && command.code != 3 && command.code != 4 && command.code != 13 && command.code != 46 && command.code != 81)
                throw std::runtime_error("Unimplemented original sound command " + std::to_string(command.code));
            if (name == "CarbonLib::SndDoImmediate") impl_->execute(s, arg(0), command);
            else {
                if (found->second.pending.size() >= found->second.capacity) {
                    if (!arg(2)) throw std::runtime_error("Blocking insertion into a full original sound queue needs a verified wake policy");
                    result = std::uint32_t(queue_full);
                } else found->second.pending.push_back(command);
            }
        }
    } else if (name == "CarbonLib::SndDisposeChannel") {
        const auto token = arg(0);
        const auto found = impl_->channels.find(token);
        if (found == impl_->channels.end()) result = std::uint32_t(bad_channel);
        else {
            if (!arg(1)) {
                if (impl_->polling) throw std::runtime_error("Blocking sound disposal inside an original callback would deadlock");
                // A barrier follows the commands present at entry. Commands
                // appended by looping callbacks are discarded after it drains.
                found->second.closing = true;
                found->second.pending.push_back({0xffff, 0, 0});
                while (!impl_->channels.at(token).drained) {
                    impl_->poll(s);
                    if (impl_->channels.at(token).drained) break;
                    if (impl_->mode == AudioMode::offline) (void)impl_->output().render_offline(4096);
                    else std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            const bool owned = impl_->channels.at(token).owned;
            impl_->output().dispose(token);
            impl_->channels.erase(token);
            if (owned) deallocate(s, token);
        }
    } else return false;
    s.wr<4>(12, result);
    s.pc = std::uint32_t(s.r<4>(4128));
    return true;
}
}
