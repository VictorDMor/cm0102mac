#pragma once
// The Speech Manager, for the original's speech option (MAIN.CPP speaks news
// items). Off unless CM_AOT_SPEECH is "on" (spoken) or "silent" (spoken at
// volume 0, for validation runs): without it Gestalt('ttsc') reports no
// Speech Manager and the original never speaks, as before.
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace cm::aot {
class NativeSpeech {
public:
    NativeSpeech();
    ~NativeSpeech();
    NativeSpeech(const NativeSpeech&) = delete;
    NativeSpeech& operator=(const NativeSpeech&) = delete;
    bool available() const { return mode_ != Mode::off; }
    // Each returns the Speech Manager's OSErr.
    std::int16_t new_channel(std::uint32_t& token);
    std::int16_t speak(std::uint32_t token, std::string text);  // MacRoman bytes
    std::int16_t busy() const;                                   // channels speaking
    std::int16_t dispose(std::uint32_t token);

private:
    enum class Mode { off, on, silent } mode_ = Mode::off;
    struct Channel;
    std::map<std::uint32_t, std::unique_ptr<Channel>> channels_;
    std::uint32_t next_token_ = 0x53500000;
};
}
