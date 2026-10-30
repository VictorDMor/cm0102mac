#pragma once
#include <functional>
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
    void quick_save_load_begin(State&);
    void quick_save_load_end(State&);
    bool quick_save_write_begin(State&);
    void quick_save_write_mode(State&);
    void quick_save_write_renamed(State&);
    void quick_save_write_end(State&);
    bool quick_save_busy() const;
    bool debug_attribute_change_available() const;
    bool quick_save_available() const;
    bool quick_save_request(State&);
    void quick_save_note_boundary(const State&);
    bool quick_save_pending(State&);
    bool quick_save_prepare_path(State&);
    bool quick_save_call_dispatcher_writer(State&);
    // The caller owns a writable 256-byte guest buffer until the writer ends.
    bool quick_save_prepare_path_at(State&, std::uint32_t buffer, bool quiet_progress = false);
    bool quick_save_quiet_progress() const;
    void quick_save_at_screen_boundary(State&);
    // Receives the messages quick save would otherwise show in an alert.
    void set_quick_save_alert_observer(std::function<void(const std::string&)>);
private:
    void quick_save_abandon() noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
