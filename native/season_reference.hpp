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
// A bounded operation (a captured world pass): pure imports such as pow and
// BlockMoveData run live through the bridges and their records are skipped;
// OS imports replay from the transcript. Instruction and native-call counts
// are diagnostics here. The end state is checked by the caller.
void open_operation_replay(const std::filesystem::path& directory);
// For timing: skip the recorded RNG state writes instead of checking each
// one through a memory observer (an observer also switches C++ owners to
// their observed paths), and skip the heap check before each OS import.
// The caller's end-state check still applies.
void open_timing_replay(const std::filesystem::path& directory);
void close_season_replay();
// Direct dependency fixtures can end after the last import, without a day
// boundary. Require physical EOF/checksum before closing those short fixtures.
void require_season_replay_eof();
// Empty means exact agreement. Reports named game/RNG fields first, then the
// first architectural or allocated-memory byte. No volatile fields are masked.
std::string compare_season_states(const State& expected, const State& actual, bool semantic_calendar = false);
// For an operation replay: as above, but instruction and native-call counts
// are diagnostics, and only registers the caller can read are compared.
std::string compare_operation_states(const State& expected, const State& actual);
std::string season_state_summary(const State& state);
}
