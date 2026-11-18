#pragma once
#include <functional>
#include "audio_bridge.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "presentation.hpp"

namespace cm::aot {
struct State;
class DatabaseSeasons;
class CareerRules;
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
    DatabaseSeasons& database_seasons();
    // The running career's own rules (the save member native_rules.tmp).
    CareerRules& career_rules();
    // The guest's resolved temporary folder, where the original save code
    // collects and restores `*.tmp` members.
    void note_temporary_directory(std::string guest_directory);
    void inspect_database_season_names();
    // True between the saved-career loader's entry and its single exit.
    bool career_loading() const;
    // The loader is about to replace the tables of the running world.
    void career_database_replaced();
    // The loader's second pass extracted the save's `*.tmp` members into the
    // temporary folder (0x10346ee4, result in r3).
    void career_members_extracted(State&);
    void career_load_reset();
    std::vector<std::filesystem::path> save_window_images(const std::filesystem::path& directory) const;
    std::vector<float> render_audio_offline(std::uint32_t frames);
    void observe_drawing(State&, std::uint32_t pc);
    GuiCanvas gui_canvas_size() const;
    // Consumed by the local screen manager, never by an AppKit callback.
    bool request_adaptive_screen(std::uint32_t builder);
    void apply_adaptive_screen(State&, std::uint32_t display);
    bool resize_gui_canvas(State&, GuiCanvas);
    void quick_save_load_begin(State&);
    void quick_save_load_end(State&);
    bool quick_save_write_begin(State&);
    void quick_save_write_mode(State&);
    void quick_save_write_renamed(State&);
    void quick_save_write_end(State&);
    bool quick_save_busy() const;
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
