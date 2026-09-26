#pragma once
#include "audio_bridge.hpp"
#include <memory>
#include <string>

namespace cm::aot {
struct State;
class NativeSound {
public:
    explicit NativeSound(AudioMode mode = AudioMode::device);
    ~NativeSound();
    bool dispatch(State&, const std::string& import);
    void poll(State&);
    bool has_pending_commands() const;
    std::vector<float> render_offline(std::uint32_t frames);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
