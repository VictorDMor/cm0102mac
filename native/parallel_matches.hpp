#pragma once
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace cm::aot {
struct State;
// Host preference "Parallel Matches" of Game Settings: whether background matches are played on several
// threads (MatchDayOrder::parallel, match_day_driver.hpp) or in the original's order. On by default
// (decided 2026-11-24, after the comparison of the two orders' results: TASKS.md, "Parallel play phase").
// Never stored in the original preferences or in career records. Off, the game is the original's.
// The preference exists only where the game's host initialises it: checkers and replays, which never do,
// stay in the original order. CM_MATCH_DAY_ORDER, when set, wins over the preference, and a session that
// records a capture or a season reference (CM_AOT_LAUNCH_CAPTURE, CM_AOT_SEASON_REFERENCE) runs in the
// original order, which is what replays reproduce; in those sessions the row is not shown.
// The day driver reads the order once when a day's matches start, so a change never splits a day.
inline constexpr bool default_parallel_matches = true;
inline constexpr unsigned parallel_matches_action = 30033;
bool parallel_matches_available();
bool parallel_matches_enabled();
void initialize_parallel_matches(const std::filesystem::path& preference);
void select_parallel_matches(bool enabled);
std::string_view parallel_matches_label(unsigned language);
void begin_parallel_matches_options();
void append_parallel_matches_option(State&);
bool dispatch_parallel_matches_option(State&, unsigned action);
void finish_parallel_matches_options(unsigned action, std::int32_t result);
}
