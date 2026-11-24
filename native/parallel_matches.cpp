#include "parallel_matches.hpp"
#include "aot_runtime.hpp"
#include "match_day_driver.hpp"
#include "match_debugging.hpp"
#include "new_screens.hpp"
#include "settings_toggle.hpp"

#include <array>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <string>

namespace cm::aot {
namespace {
SettingsToggle toggle{parallel_matches_action};
std::atomic<bool> available{false}, enabled{false}, forced{false};
std::filesystem::path file;

void apply(bool on) {
    enabled.store(on);
    if (!forced.load()) set_match_day_order(on ? MatchDayOrder::parallel : MatchDayOrder::original);
}
}
bool parallel_matches_available() { return available.load(std::memory_order_relaxed); }
bool parallel_matches_enabled() { return enabled.load(std::memory_order_relaxed); }
void initialize_parallel_matches(const std::filesystem::path& preference) {
    file = preference;
    // The order named in the environment, or a session being recorded, is not the preference's to change.
    const char* order = std::getenv("CM_MATCH_DAY_ORDER");
    const char* capture = std::getenv("CM_AOT_LAUNCH_CAPTURE");
    const char* reference = std::getenv("CM_AOT_SEASON_REFERENCE");
    forced.store((order && *order) || (capture && *capture) || reference);
    // Then the row is not shown either: a choice with no effect on this session must not be saved for the next.
    if (forced.load()) return;
    bool on = default_parallel_matches;
    if (std::ifstream in(file); in) {
        std::string word;
        if (in >> word && (word == "on" || word == "off")) on = word == "on";
    }
    available.store(true);
    apply(on);
}
void select_parallel_matches(bool on) {
    apply(on);
    if (file.empty()) return;
    std::error_code ignored;
    std::filesystem::create_directories(file.parent_path(), ignored);
    const auto temporary = std::filesystem::path(file).concat(".tmp");
    bool written = false;
    { std::ofstream out(temporary, std::ios::trunc); out << (on ? "on\n" : "off\n"); out.flush(); written = bool(out); }
    if (written) std::filesystem::rename(temporary, file, ignored);
    // Not saved (a full disk, a read-only folder): the choice holds for this session only.
    if (!written || ignored) std::filesystem::remove(temporary, ignored);
}
std::string_view parallel_matches_label(unsigned language) {
    // The original font renderer consumes single-byte game text, not UTF-8; the languages in the game's order.
    static constexpr std::array<std::string_view, 11> labels{
        "Parallel Matches", "Parallel Matches", "Matchs en parall\350le", "Parallele Spiele",
        "Partite in parallelo", "Partidos en paralelo", "Parallelle kampe", "Parallelle wedstrijden",
        "Parallelle kamper", "Jogos em Paralelo", "Parallella matcher"};
    return labels[language < labels.size() ? language : 0];
}
void begin_parallel_matches_options() { toggle.begin(parallel_matches_enabled()); }
void append_parallel_matches_option(State& s) {
    if (!parallel_matches_available()) return;
    if (!toggle.pending) begin_parallel_matches_options();
    // The third host row: after Debugging and New Screens, in the order preferences_screen.cpp appends them.
    toggle.append(s, 14 + unsigned(native_debugging_available()) + unsigned(new_screens_available()),
                  parallel_matches_label(s.memory.read<1>(s.r<4>(8) - 0x35ac)));
}
bool dispatch_parallel_matches_option(State& s, unsigned action) {
    return parallel_matches_available() && toggle.dispatch(s, action, default_parallel_matches);
}
void finish_parallel_matches_options(unsigned action, std::int32_t result) {
    if (!parallel_matches_available()) return;
    if (const auto commit = toggle.finish(action, result)) select_parallel_matches(*commit);
}
}
