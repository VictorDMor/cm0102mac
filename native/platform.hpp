#pragma once
#include "audio_bridge.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot {
struct State;
enum class EventMode { interactive, isolated };
class NativePlatform {
public:
    NativePlatform(std::filesystem::path game_directory, std::filesystem::path writable_directory, AudioMode audio = AudioMode::device);
    NativePlatform(std::filesystem::path game_directory, std::filesystem::path writable_directory, AudioMode audio, EventMode events);
    ~NativePlatform();
    NativePlatform(const NativePlatform&) = delete;
    NativePlatform& operator=(const NativePlatform&) = delete;
    bool dispatch(State& state, std::string_view import);
    const std::filesystem::path& game_directory() const;
    const std::filesystem::path& writable_directory() const;
    std::vector<std::filesystem::path> save_window_images(const std::filesystem::path& directory) const;
    std::vector<float> render_audio_offline(std::uint32_t frames);
    void observe_drawing(State&, std::uint32_t pc);
    void debug_attribute_load_begin(State&);
    void debug_attribute_load_end(State&);
    bool debug_attribute_save_may_enter(const State&) const;
    bool debug_attribute_save_begin(State&);
    void debug_attribute_save_mode(State&);
    void debug_attribute_save_renamed(State&);
    void debug_attribute_save_end(State&);
    void debug_attribute_save_abort() noexcept;
    bool debug_attribute_change_available() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
