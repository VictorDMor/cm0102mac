#include "speech_bridge.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <cstdio>
#include <cstdlib>
#include <string_view>

// The Speech Manager is deprecated in favour of AVSpeechSynthesizer, but it
// is the API the original calls, with the same channel and busy semantics.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace cm::aot {
struct NativeSpeech::Channel {
    SpeechChannel channel = nullptr;
    std::string text;  // SpeakText reads it while speaking
};
NativeSpeech::NativeSpeech() {
    if (const char* setting = std::getenv("CM_AOT_SPEECH")) {
        const std::string_view value{setting};
        mode_ = value == "on" ? Mode::on : value == "silent" ? Mode::silent : Mode::off;
    }
}
NativeSpeech::~NativeSpeech() {
    for (auto& [token, channel] : channels_) {
        (void)token;
        ::DisposeSpeechChannel(channel->channel);
    }
}
std::int16_t NativeSpeech::new_channel(std::uint32_t& token) {
    if (!available()) return -192;  // resNotFound: no synthesizer
    auto channel = std::make_unique<Channel>();
    if (const auto error = ::NewSpeechChannel(nullptr, &channel->channel)) return error;
    if (mode_ == Mode::silent) {
        const double zero = 0;
        const auto volume = CFNumberCreate(nullptr, kCFNumberDoubleType, &zero);
        ::SetSpeechProperty(channel->channel, kSpeechVolumeProperty, volume);
        CFRelease(volume);
    }
    token = next_token_++;
    channels_.emplace(token, std::move(channel));
    return 0;
}
std::int16_t NativeSpeech::speak(std::uint32_t token, std::string text) {
    const auto found = channels_.find(token);
    if (found == channels_.end()) return -50;  // paramErr
    auto& channel = *found->second;
    ::StopSpeech(channel.channel);
    channel.text = std::move(text);
    std::fprintf(stderr, "Speech: %zu bytes on channel %08x\n", channel.text.size(), unsigned(token));
    return ::SpeakText(channel.channel, channel.text.data(), channel.text.size());
}
std::int16_t NativeSpeech::busy() const { return ::SpeechBusy(); }
std::int16_t NativeSpeech::dispose(std::uint32_t token) {
    const auto found = channels_.find(token);
    if (found == channels_.end()) return -50;
    const auto error = ::DisposeSpeechChannel(found->second->channel);
    channels_.erase(found);
    return error;
}
}
