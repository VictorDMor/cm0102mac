#pragma once
#include "aot_runtime.hpp"
#include <span>

namespace cm::aot {
// Private, versioned diagnostic fixtures; never part of the game's save format.
struct SeasonReplayComplete {};
void season_day_boundary(State& state, std::uint32_t pc);
void season_record_import_begin(State& state);
void season_record_import_end(State& state);
void season_record_callback_begin(State& state, std::uint32_t vector,
                                  std::span<const std::uint32_t> arguments);
void season_record_callback_end(State& state, std::uint32_t result);
bool season_replay_import(State& state, const char* name);
void open_season_replay(const std::filesystem::path& directory, bool semantic_calendar = false);
void close_season_replay();
// Empty means exact agreement. Reports named game/RNG fields first, then the
// first architectural or allocated-memory byte. No volatile fields are masked.
std::string compare_season_states(const State& expected, const State& actual, bool semantic_calendar = false);
std::string season_state_summary(const State& state);
}
